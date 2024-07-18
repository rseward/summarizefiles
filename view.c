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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "models.h"

/**
 * view orientated code for the project. Display the results to the user.
 */

void sf_sortentries(sumfiles_t *self, sumentry_t *results, size_t result_size);
int sf_compare_size_desc(const void *a, const void *b);
int sf_compare_group(const void *a, const void *b);
int sf_compare_lines_desc(const void *a, const void *b);

char *show_size(char *strbuf, size_t bytes)
{
    if (bytes >= 1099511627776.0)
        sprintf(strbuf, " %9.3f TB", bytes / 1099511627776.0);
    else if (bytes >= 1073741824.0)
        sprintf(strbuf, " %9.3f GB", bytes / 1073741824.0);
    else if (bytes >= 1048576.0)
        sprintf(strbuf, " %9.3f MB", bytes / 1048576.0);
    else if (bytes >= 1024.0)
        sprintf(strbuf, " %9.3f KB", bytes / 1024.0);
    else
        sprintf(strbuf, " %9.0f B  ", bytes);

    return strbuf;
}

char *se_show(sumfiles_t *self, sumentry_t *entry, char *sbufentry)
{
    char sbufbytes[256];
    memset(sbufbytes, 0, sizeof(sbufbytes));
    char group[self->colsize];
    strcpy(group, entry->group);
    group[10]=0;


    char *dval=group;
    if ((self->popts == SF_TIME)!=0)
        {
            dval=entry->label;
        }

    if ((self->popts == SF_LINES)!=0)
        {
            sprintf(sbufbytes, "%d lines", entry->line_count);
        }
    else
        {
            show_size(sbufbytes, entry->total_bytes);
        }
    sprintf(sbufentry, "|%10s: %10s in %d files", dval, sbufbytes, entry->file_count);
    sbufentry[self->colsize]=0;

    return sbufentry;
}

void sf_renderline(sumfiles_t *self, sumentry_t *results, size_t result_size, int row)
{
    char sbufentry[self->console_cols + 1];
    char outbuf[self->console_cols+2];
    char colbuf[self->colsize+1];
    char *outbufp = outbuf;

    //printf("Before outbuf zero out rsize=%d.\n", result_size);
    memset(outbuf, ' ', sizeof(outbuf));
    outbuf[sizeof(outbuf)]=0;
    //printf("After outbuf zero out. row=%d\n", row);

    // identify the entries that should appear in the rendered display
    int drows = self->console_rows - 2;
    int idx = row;
    int colidx=0;
    if (drows<1)
        {
            printf("drows=%d\n", drows);
            return;
        }
    while (idx<result_size)
        {
            //printf("idx=%d, col=%d ncols=%d res_size=%d\n",idx, colidx, self->entries_per_line, result_size);
            sprintf(colbuf, "%-*s", self->colsize, se_show(self, &results[idx], sbufentry) );
            colbuf[self->colsize]=0;
            memcpy(outbufp+(colidx*self->colsize), colbuf, strlen(colbuf));
            if (drows<1)
                {
                    break;
                }
            idx = idx + drows;
            colidx++;
            if (colidx>self->entries_per_line)
                {
                    if ( (self->popts & SF_DEBUG) != 0 )
                        {
                            //printf("idx=%d, col=%d ncols=%d res_size=%d drows=%d crows=%d colbuf.len=%d\n",idx, colidx, self->entries_per_line, result_size, drows, self->console_rows, strlen(colbuf) );
                            //printf("Exceeded number of columns!\n");
                        }
                    break;
                }
        }
    outbuf[self->console_cols-2]=0;
    printf("%-*s\n", self->console_cols-2, outbuf);
}

char *now(char *sbuf, int bufsize)
{
    time_t tnow = time(NULL);
    struct tm *tm = localtime(&tnow);
    size_t ret = strftime(sbuf, bufsize, SF_DATETIMEFMT, tm);
    assert(ret);
    return sbuf;
}

char *formatmtime(time_t fmtime, char *sbuf, int bufsize)
{
    struct tm mtime;
    localtime_r(&fmtime, &mtime);
    strftime(sbuf, bufsize, SF_DATEFMT,  &mtime);

    return sbuf;
}

void sf_showresults(sumfiles_t *self, sumentry_t *results, size_t result_size)
{
    char sbufentry[1024];

    self->entries_per_line = self->console_cols / self->colsize;
    self->dentries = (self->entries_per_line) * (self->console_rows - 2);

    if ((self->popts & SF_TIME)!=0)
        {
            qsort( results, result_size, sizeof(sumentry_t), sf_compare_group );
        }
    else
        {
            if ((self->popts & SF_LINES)!=0)
                {
                    qsort( results, result_size, sizeof(sumentry_t), sf_compare_lines_desc );
                }
            else
                {
                    qsort( results, result_size, sizeof(sumentry_t), sf_compare_size_desc );
                }
        }

    /*
    int idx = 0;
    for (idx=0; idx<result_size; idx++) {
      //sumentry_t *entry = results[idx];
      //printf( results[idx].group );
      printf( se_show(&results[idx], sbufentry ) );
      }*/

    if ( (self->popts & SF_DEBUG) != 0 )
        {
            printf("res_size=%d\n", result_size);
            printf("view_entries=%d\n", self->dentries);

            printf("Buckle up, it's go time!\n");
        }
    else
        {
            printf("\e[1;1H\e[2J"); // Clear the console
            char mindatebuf[64];
            char maxdatebuf[64];
            now(sbufentry, 64);
            formatmtime(self->min_mod_time, mindatebuf, 64);
            formatmtime(self->max_mod_time, maxdatebuf, 64);

            if (self->console_cols>82)
                {
                    printf("%s %s  min mdate: %s   max mdate: %s exceptions: %d\n\n", sbufentry, self->rootpathdisp, mindatebuf, maxdatebuf, self->exceptions);
                }
            else
                {
                    // short status line for small terminals
                    printf("%s %s\n\n", sbufentry, self->rootpathdisp);
                }
        }
    int ridx;
    for (ridx=0; ridx<self->console_rows-3; ridx++)
        {
            sf_renderline(self, results, result_size, ridx);
        }
}

int sf_compare_size_desc(const void *a, const void *b)
{
    const sumentry_t *e1 = (const sumentry_t *)a;
    const sumentry_t *e2 = (const sumentry_t *)b;
    //printf("compare: %s.%d == %s.%d\n", e1->group, e1->total_bytes, e2->group, e2->total_bytes );
    return e2->total_bytes - e1->total_bytes;
}

int sf_compare_lines_desc(const void *a, const void *b)
{
    const sumentry_t *e1 = (const sumentry_t *)a;
    const sumentry_t *e2 = (const sumentry_t *)b;
    return e2->line_count - e1->line_count;
}

int sf_compare_group(const void *a, const void *b)
{
    const sumentry_t *e1 = (const sumentry_t *)a;
    const sumentry_t *e2 = (const sumentry_t *)b;

    return strcmp( e2->group, e1->group);
}

/*
void sf_sortentries(sumfiles_t *self, sumentry_t *results, size_t result_size)
{
  qsort( results, result_size, sizeof(sumentry_t), sf_compare_size_desc );

  return 0;
  }*/

