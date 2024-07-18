#include <time.h>
#include <limits.h>
#include <cfuhash.h>
#include <magic.h>

#define SF_LOG    1
#define SF_EXT    2
#define SF_TIME   4
#define SF_DEBUG  8
#define SF_LINES 16

struct sumfiles
{
    char rootpath[1024];
    char rootpathdisp[256];
    short popts;
    int console_cols;
    int console_rows;
    int dentries;
    int entries_per_line;
    int colsize;
    int exceptions;

    time_t min_mod_time;
    time_t max_mod_time;
    time_t last_refresh;

    cfuhash_table_t *entries;
    magic_t magic_session;
};
typedef struct sumfiles sumfiles_t;

#define SF_STRING_LIMIT 50

struct sumentry
{
    char group[SF_STRING_LIMIT];
    char label[SF_STRING_LIMIT];
    long total_bytes;
    int line_count;
    int file_count;
    time_t min_mod_time;
    time_t max_mod_time;
    char *display;
};
typedef struct sumentry sumentry_t;

#define SF_DATEFMT "%Y-%m-%d"
#define SF_DATETIMEFMT "%Y-%m-%d %H:%m"

