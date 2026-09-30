#ifndef OPTIONS_H
#define OPTIONS_H

typedef enum {
    SORT_NAME,
    SORT_SIZE,
    SORT_TIME
} SortMode;

typedef enum {
    TIME_MTIME,
    TIME_CTIME,
    TIME_ATIME
} TimeMode;

typedef enum {
    BLOCK_DEFAULT,
    BLOCK_KILOBYTES,
    BLOCK_HUMAN
} BlockMode;

typedef enum {
    DIR_NORMAL,
    DIR_AS_FILE,
    DIR_RECURSIVE
} DirectoryMode;

typedef enum {
    PRINT_AUTO,
    PRINT_QUESTION,
    PRINT_RAW
} PrintMode;

typedef struct {
    int show_all;           /* -a */
    int almost_all;         /* -A */

    int no_sort;            /* -f */
    int reverse;            /* -r */

    int show_inode;         /* -i */
    int classify;           /* -F */

    int long_format;        /* -l or -n */
    int numeric_ids;        /* -n */

    int show_blocks;        /* -s */

    SortMode sort_mode;     /* name, -S, -t */
    TimeMode time_mode;     /* mtime, -c, -u */
    BlockMode block_mode;   /* default, -k, -h */

    DirectoryMode directory_mode; /* normal, -d, -R */
    PrintMode print_mode;          /* auto, -q, -w */

    long long block_size;   /* BLOCKSIZE environment variable */

} Options;

void init_options(Options *options);
int parse_options(int argc, char *argv[], Options *options);

#endif
