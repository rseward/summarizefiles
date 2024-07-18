/*
 ** Copyright (c) 2024 Bluestone Consulting Group, LLC
 ** Copyright (c) 2024 Rob Seward <rseward@bluestone-consulting.com>
 **
 ** This program is free software; please use, redistribute and modify it under the terms of
 ** the BSD license. (Chosen to be compatible with the libcfu library).
 **
 ** If you modify it for a specific purpose please consider making a pull request here:
 **
 ** Create an issue at the project for consideration to merge the pull request.
 */

/* We want POSIX.1-2008 + XSI, i.e. SuSv4, features */
#define _XOPEN_SOURCE 700

/* Added on 2017-06-25:
   If the C library can support 64-bit file sizes
   and offsets, using the standard names,
   these defines tell the C library to do so. */
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <unistd.h>
#include <ftw.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <assert.h>
#include "summarizefiles.h"

/* POSIX.1 says each process has at least 20 file descriptors.
 * Three of those belong to the standard streams.
 * Here, we use a conservative estimate of 15 available;
 * assuming we use at most two for other uses in this program,
 * we should never run into any problems.
 * Most trees are shallower than that, so it is efficient.
 * Deeper trees are traversed fine, just a bit slower.
 * (Linux allows typically hundreds to thousands of open files,
 *  so you'll probably never see any issues even if you used
 *  a much higher value, say a couple of hundred, but
 *  15 is a safe, reasonable value.)
*/
#ifndef USE_FDS
#define USE_FDS 15
#endif

void sf_show(sumfiles_t *self);
int sf_addentry(sumfiles_t *self, const char *fullpath, const char *basefile, const struct stat *info);
sumfiles_t *sfstate;
int sf_getconsolesize(sumfiles_t *self);

/**********************************************************************************************
 * print_entry: nftw example for using the dir traversal function. Left in code for debugging
 *    purposes.
 **********************************************************************************************/

int print_entry(const char *filepath, const struct stat *info,
                const int typeflag, struct FTW *pathinfo)
{
    /* const char *const filename = filepath + pathinfo->base; */
    const double bytes = (double)info->st_size; /* Not exact if large! */
    struct tm mtime;

    localtime_r(&(info->st_mtime), &mtime);

    printf("%04d-%02d-%02d %02d:%02d:%02d",
           mtime.tm_year+1900, mtime.tm_mon+1, mtime.tm_mday,
           mtime.tm_hour, mtime.tm_min, mtime.tm_sec);

    char sbuf[256];

    printf( show_size(sbuf, bytes) );

    if (typeflag == FTW_SL)
        {
            char   *target;
            size_t  maxlen = 1023;
            ssize_t len;

            while (1)
                {

                    target = malloc(maxlen + 1);
                    if (target == NULL)
                        return ENOMEM;

                    len = readlink(filepath, target, maxlen);
                    if (len == (ssize_t)-1)
                        {
                            const int saved_errno = errno;
                            free(target);
                            return saved_errno;
                        }
                    if (len >= (ssize_t)maxlen)
                        {
                            free(target);
                            maxlen += 1024;
                            continue;
                        }

                    target[len] = '\0';
                    break;
                }

            printf(" %s -> %s\n", filepath, target);
            free(target);

        }
    else if (typeflag == FTW_SLN)
        printf(" %s (dangling symlink)\n", filepath);
    else if (typeflag == FTW_F)
        printf(" %s\n", filepath);
    else if (typeflag == FTW_D || typeflag == FTW_DP)
        printf(" %s/\n", filepath);
    else if (typeflag == FTW_DNR)
        printf(" %s/ (unreadable)\n", filepath);
    else
        printf(" %s (unknown)\n", filepath);

    return 0;
}

char* get_file_extension(const char* filepath)
{
    char* dot = strrchr(filepath, '.');
    if (!dot || dot == filepath)
        {
            return "";  // No extension or filename is just a dot
        }
    //printf("%p\n", filepath);
    //printf("%p\n", dot+1);
    return dot + 1;
}

/**********************************************************************************************
 * summarize_entry: An entry point from nftw, this function is called everytime a file
 *    or directory is discovered for analysis.
 **********************************************************************************************/

int summarize_entry(const char *filepath, const struct stat *info,
                    const int typeflag, struct FTW *pathinfo)
{
    int ret = 0;
    char tmpfp[strlen(filepath)];
    int before = sfstate->exceptions;
    strcpy(tmpfp, filepath); // Make a copy for the potentially destructive basename func

    if ((sfstate->popts & SF_DEBUG) != 0)
        {
            print_entry(filepath, info, typeflag, pathinfo);
        }
    sf_addentry(sfstate, filepath, basename(tmpfp), info);
    if (sfstate->exceptions>before)
        {
            print_entry(filepath, info, typeflag, pathinfo);
        }

    return ret;
}

/**********************************************************************************************
 * summarize_files: Execute a directory tree traversal using the nftw library. Begin an
 *    analysis of files within the tree.
 **********************************************************************************************/

int summarize_files(const char *const dirpath)
{
    //db = sqlite_open();
    int result;

    /* Invalid directory path? */
    if (dirpath == NULL || *dirpath == '\0')
        return errno = EINVAL;

    result = nftw(dirpath, summarize_entry, USE_FDS, FTW_PHYS);
    if (result >= 0)
        errno = result;

    return errno;
}

/**********************************************************************************************
 * sf_new: Init the structure containing state info used by the summarizefiles set of
 *    functions.
 **********************************************************************************************/

sumfiles_t *sf_new(int popts)
{
    sumfiles_t *self = malloc(sizeof(sumfiles_t)); // freed

    self->entries = cfuhash_new_with_initial_size(10000);
    //self->popts = SF_EXT; // + SF_DEBUG;
    //self->popts = SF_TIME;
    // self->popts = SF_LINES;
    self->popts = popts;
    self->console_cols = 80;
    self->console_rows = 30;
    self->last_refresh = 0;
    self->colsize = 40;
    self->min_mod_time = INT_MAX;
    self->max_mod_time = 0;
    self->exceptions = 0;
    strcpy(self->rootpath, "");
    strcpy(self->rootpathdisp, "");

    if ( (self->popts & SF_DEBUG) != 0)
        {
            printf("Debug Mode!\n");
        }

    if ( (self->popts & SF_TIME) != 0)
        {
            // TODO choose a colsize based on console width?
            self->colsize = 50;
            printf("Time Mode: colsize=%d\n", self->colsize);
        }
    if ( (self->popts & SF_LINES) != 0)
        {
            self->magic_session = magic_open(MAGIC_MIME|MAGIC_CHECK);
            if (self->magic_session == NULL)
                {
                    perror("Unable to initialize libmagic");
                    free(self);
                    return NULL;
                }

            // Load the magic database
            if (magic_load(self->magic_session, NULL)!=0)
                {
                    perror("Unable to load libmagic database");
                    free(self);
                    return NULL;
                }
        }


    sf_getconsolesize(self);

    return self;
}


/**********************************************************************************************
 * sf_refreshview: As info accumulates in the hashmap, decide if it is an auspicious time
 *    to update the view of the progress.
 **********************************************************************************************/

int sf_refreshview(sumfiles_t *self)
{
    if ( strlen(self->rootpathdisp)==0 && strlen(self->rootpath)>0 )
        {
            // Compute the chars allocated to displaying the root path on the status line
            //   and truncate the value as necessary
            char sbuf[256];
            int rootpathlen = strlen(self->rootpath);

            if (self->console_cols<=83 || rootpathlen + 78 < self->console_cols)
                {
                    // plenty of room to display the full length
                    strcpy(self->rootpathdisp, self->rootpath);
                }
            else
                {
                    // rootpath needs to be shortened
                    rootpathlen = self->console_cols - 78 -2;
                    printf("rootpathlen=%d\n", rootpathlen);
                    if (strlen(self->rootpath)<rootpathlen)
                        {
                            // unreachable
                            // rootpathlen=strlen(self-rootpath);
                            strcpy( self->rootpathdisp, self->rootpath );
                        }
                    else
                        {
                            substr(self->rootpath, -1, rootpathlen, sbuf );
                            sprintf(self->rootpathdisp, "..%s", sbuf);
                        }
                }
            printf("rootpathdisp=%s\n", self->rootpathdisp);
        }

    if ( (self->popts & SF_DEBUG) == 0 )
        {
            time_t now = time(NULL);
            if (self->last_refresh == 0 || (now - self->last_refresh) > 1)
                {
                    sf_show(self);
                    self->last_refresh=now;
                }
        }
}

/**********************************************************************************************
 * sf_addmapentry: Adapter function to perform the work necessary to add the information
 *   to the libcfu hashmap. Look at the hashtable entry by key, add the info for the entry
 *   if found, otherwise return a new entry.
 **********************************************************************************************/

sumentry_t *sf_addmapentry(
    sumfiles_t *self,
    char *key,
    char *label,
    size_t fbytes,
    int flines,
    time_t fmtime)
{
    sumentry_t *entry = cfuhash_get( self->entries, key );

    if (entry)
        {
            entry->total_bytes = entry->total_bytes + fbytes;
            entry->line_count = entry->line_count + flines;
            entry->file_count = entry->file_count + 1;
            if (entry->min_mod_time > fmtime)
                {
                    entry->min_mod_time = fmtime;
                }
            if (entry->max_mod_time < fmtime)
                {
                    entry->max_mod_time = fmtime;
                }
        }
    else
        {
            char *tlabel = label;
            if (tlabel == NULL)
                {
                    tlabel="";
                }

            entry = malloc(sizeof(sumentry_t)); // freed
            strncpy(entry->group, key, SF_STRING_LIMIT);
            strcpy(entry->label, tlabel);
            entry->total_bytes = fbytes;
            entry->line_count = flines;
            entry->file_count = 1;
            entry->min_mod_time = fmtime;
            entry->min_mod_time = fmtime;
        }

    cfuhash_put( self->entries, key, entry);

    return entry;
}


int count_lines(const char *filepath)
{
    FILE *inf = fopen(filepath, "r");
    if (inf == NULL)
        {
            printf("Failed to open: %s\n", filepath);
            return -1;
        }

    int line_count=0;
    char ch;
    while ((ch = fgetc(inf)) != EOF)
        {
            if (ch == '\n')
                {
                    line_count++;
                }
        }

    fclose(inf);

    return line_count;
}

/**********************************************************************************************
 * sf_addentry_byext: Add an entry to the hashmap performing any tasks related to a
 *   summary by extension.
 **********************************************************************************************/

int sf_addentry_byext(sumfiles_t *self, const char *fullpath, const char *basefile, const struct stat *info)
{

    char *ext = get_file_extension(basefile);
    if ((sfstate->popts & SF_DEBUG) != 0)
        {
            printf("ext=%s\n", ext);
        }
    const double bytes = (double)info->st_size; /* Not exact if large! */
    int lines = 0;

    if ((self->popts & SF_LINES) != 0)
        {
            // using magic determine if the file is a text file and count the lines if so.
            const char* ftype = magic_file(self->magic_session, fullpath);
            int istext=0;
            if (ftype != NULL)
                {
                    if (strstr(ftype,"text")!=0)
                        {
                            istext=1;
                        }
                }

            if (istext)
                {
                    lines=count_lines(fullpath);
                    if (lines<0)
                        {
                            self->exceptions++;
                            lines=0;
                        }
                }
        }

    sf_addmapentry(self, ext, "", bytes, lines, info->st_mtime);
} //|

/**********************************************************************************************
 * sf_addentry_bytime: Add an entry to the hashmap performing any tasks related to a
 *   summary by time. E.g. group files by their temporal proximity to each other.
 **********************************************************************************************/

int sf_addentry_bytime(sumfiles_t *self, const char *fullpath, const char *basefile, const struct stat *info)
{
    //printf("[bytime] %s\n", filepath);
    char key[64];
    time_t tnow = time(NULL);
    struct tm *tm_tmp;
    const double bytes = (double)info->st_size; // Not exact if large!
    long oneday = 24 * 60 * 60;
    long onemonth = tnow - 30 * oneday;
    long oneyear = tnow - 365 * oneday;

    tm_tmp = localtime( &tnow );

    char *month_g = "03month";
    char *year_g = "02year";
    char *old_g = "01old";
    char day[16];

    //printf("Select the group: %d onemonth=%d oneyear=%d\n", info->st_mtime,
    //	     onemonth - info->st_mtime, oneyear - info->st_mtime
    //	 );
    char *group = year_g;
    char *dayfmt = "%Y%m";
    if (info->st_mtime<oneyear)
        {
            group = old_g;
            dayfmt = "%Y";
        }
    else
        {
            if (info->st_mtime>onemonth)
                {
                    group = month_g;
                    dayfmt ="%Y%m%d";
                }
        }

    //printf("group=%s, dayfmt=%s\n", group, dayfmt);
    struct tm mtime;
    localtime_r(&(info->st_mtime), &mtime);
    strftime(day, 16, dayfmt,  &mtime);
    //printf("group=%s, day=%s\n", group, day);

    sprintf(key, "%s.%s", group, day);

    sf_addmapentry(self, key, day, bytes, 0, info->st_mtime);
    return 0;
}

/**********************************************************************************************
 * sf_addentry: Add an entry to the hashmap. Perform any other checks and tasks required for adding
 *   the file entry.
 **********************************************************************************************/

int sf_addentry(sumfiles_t *self, const char *fullpath, const char *basefile, const struct stat *info)
{
    if (basefile[0] == '.')
        {
            // hidden file, move on
            return 0;
        }

    if ( (info->st_mode & S_IFREG) == 0)
        {
            // Not a file, move on
            return 0;
        }

    if (info->st_mtime < self->min_mod_time)
        {
            self->min_mod_time = info->st_mtime;
        }
    if (info->st_mtime > self->max_mod_time)
        {
            self->max_mod_time = info->st_mtime;
        }

    sf_refreshview(self);

    if ( (self->popts & SF_EXT) != 0 || (self->popts & SF_LINES) != 0)
        {
            return sf_addentry_byext(self, fullpath, basefile, info);
        }

    if ( (self->popts & SF_TIME) != 0 )
        {
            return sf_addentry_bytime(self, fullpath, basefile, info);
        }

}

/**********************************************************************************************
 * sf_show: Iterate thru the file groups discovered so far and prepare them into a list for
 *  display by the view functions.
 **********************************************************************************************/

void sf_show(sumfiles_t *self)
{
    char **keys = NULL;
    size_t *key_sizes = NULL;
    size_t key_count = 0;
    int idx = 0;
    char sbufentry[1024];

    keys = (char **)cfuhash_keys_data(self->entries, &key_count, &key_sizes, 0);
    //printf("sf_show keys=%d\n", key_count);

    sumentry_t results[key_count];
    int residx=0;
    for (idx=0; idx<key_count; idx++)
        {
            //printf("[%s]\n", keys[idx]);

            sumentry_t *entry = cfuhash_get(self->entries, keys[idx]);
            int includeentry=0;

            if (entry->total_bytes>1024)
                {
                    if ((self->popts & SF_LINES)!=0)
                        {
                            if (entry->line_count>0)
                                {
                                    includeentry=1;
                                }
                        }
                    else
                        {
                            includeentry=1;
                        }
                }

            if (includeentry)
                {
                    memcpy( &results[residx], entry, sizeof(sumentry_t) );
                    residx++;
                }
            //printf( se_show(entry, sbufentry ) );
            free(keys[idx]);
        }

    sf_showresults(self, (sumentry_t *) results, residx);

    free(key_sizes);
    free(keys);

}

int sf_getconsolesize(sumfiles_t *self)
{
    char strbuf[256];
    int status = mysystem(strbuf, "stty size", 256);
    strtrim(strbuf);
    printf("'%s'\n", strbuf);

    char *rest = strbuf;
    char *token = strtok_r(strbuf, " ", &rest);
    self->console_rows = atoi(token);
    if (token != NULL)
        {
            token = strtok_r(NULL, " ", &rest);
            self->console_cols = atoi(token);
        }

    printf("console size cols=%d rows=%d\n", self->console_cols, self->console_rows );
}

/**********************************************************************************************
 * Destroy the state information and all it's dependent objects.
 **********************************************************************************************/

void sf_destroy(sumfiles_t *self)
{
    // Clean up after the run

    // iterate over the map entries and free them
    char **keys = NULL;
    size_t *key_sizes = NULL;
    size_t key_count = 0;
    int idx = 0;
    char sbufentry[1024];

    keys = (char **)cfuhash_keys_data(self->entries, &key_count, &key_sizes, 0);
    //printf("sf_show keys=%d\n", key_count);

    sumentry_t results[key_count];
    int residx=0;
    for (idx=0; idx<key_count; idx++)
        {
            sumentry_t *entry = cfuhash_get(self->entries, keys[idx]);
            free(entry);
            free(keys[idx]);
        }
    free(key_sizes);
    free(keys);

    cfuhash_destroy(self->entries);
    free(self);
}

void help()
{
    printf( "usage: summarizefiles.py [-h] [--time] [--debug] [--lines] N [N ...]\n"
            "\n"
            "positional arguments:\n"
            "  N            Directories to summarize\n"
            "\n"
            "options:\n"
            "  -h, --help   show this help message and exit\n"
            "  --time, -t   Summarize files by date modified. Most sophiscated time summary. Try it!\n"
            "  --debug, -v  Something don't work, time to debug!\n"
            "  --lines, -L  Summarize text files by their line count\n\n");
}

/* ################################################################################################
 *  Main method for parsing args and initiating a file summary scan.
 * ################################################################################################ */


int main(int argc, char *argv[])
{
    int arg;

    const struct option long_options[] =
    {
        { "help", no_argument, NULL, 'h' },
        { "debug", no_argument, NULL, 'd' },
        { "lines", no_argument, NULL, 'L' },
        { "time", no_argument, NULL, 't' },
        {0, 0, 0, 0}
    };

    static const char *short_options = "hdLt";

    int popts = 0;
    int option;
    char c;
    while ((c = getopt_long(argc, argv, short_options, long_options, &option)) != -1)
        {
            switch(c)
                {
                case 'h':
                    help();
                    exit(0);
                    break;
                case 'd':
                    popts = popts + SF_DEBUG;
                    break;
                case 'L':
                    popts = popts + SF_LINES;
                    break;
                case 't':
                    popts = popts + SF_TIME;
                    break;
                }
        }

    if ( (popts & SF_TIME) == 0 && (popts & SF_LINES) == 0)
        {
            // default to a summary by extension
            popts = popts + SF_EXT;
        }

    printf("popts=%d\n", popts);
    sfstate = sf_new(popts);

    assert(sfstate!=NULL);

    if (argc < optind)
        {
            strcpy(sfstate->rootpath, ".");
            if (summarize_files("."))
                {
                    fprintf(stderr, "%s.\n", strerror(errno));
                    return EXIT_FAILURE;
                }

        }
    else
        {
            for (arg = optind; arg < argc; arg++)
                {
                    strcpy(sfstate->rootpath, argv[arg]);
                    if (summarize_files(argv[arg]))
                        {
                            fprintf(stderr, "%s.\n", strerror(errno));
                            return EXIT_FAILURE;
                        }
                }
        }

    sf_show(sfstate);
    sf_destroy(sfstate);

    return EXIT_SUCCESS;
}