#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "options.h"


/*
 * Parse BLOCKSIZE.
 *
 * Supported examples:
 *
 * BLOCKSIZE=512
 * BLOCKSIZE=1024
 * BLOCKSIZE=1K
 * BLOCKSIZE=2K
 * BLOCKSIZE=1M
 */
static long long parse_block_size(const char *value)
{
    char *end;
    unsigned long long number;
    unsigned long long multiplier;

    if (value == NULL || *value == '\0')
        return 512;

    errno = 0;

    number = strtoull(
        value,
        &end,
        10
    );

    if (errno != 0 ||
        end == value ||
        number == 0) {

        return 512;
    }

    multiplier = 1;

    if (*end != '\0') {

        char suffix;

        suffix = *end;
        end++;

        switch (suffix) {

        case 'b':
        case 'B':
            multiplier = 1;
            break;

        case 'k':
        case 'K':
            multiplier = 1024ULL;
            break;

        case 'm':
        case 'M':
            multiplier =
                1024ULL * 1024ULL;
            break;

        case 'g':
        case 'G':
            multiplier =
                1024ULL *
                1024ULL *
                1024ULL;
            break;

        case 't':
        case 'T':
            multiplier =
                1024ULL *
                1024ULL *
                1024ULL *
                1024ULL;
            break;

        default:
            return 512;
        }

        /*
         * Also accept forms such as:
         *
         * 1KB
         * 1MB
         */
        if ((*end == 'B' ||
             *end == 'b') &&
            end[1] == '\0') {

            end++;
        }

        if (*end != '\0')
            return 512;
    }

    if (number >
        (unsigned long long)LLONG_MAX /
        multiplier) {

        return 512;
    }

    return (long long)
        (number * multiplier);
}


void init_options(Options *options)
{
    /*
     * NetBSD manual:
     * -A is always enabled for super-user.
     */
    options->almost_all =
        (geteuid() == 0) ? 1 : 0;

    options->show_all = 0;

    options->no_sort = 0;
    options->reverse = 0;

    options->show_inode = 0;
    options->classify = 0;

    options->long_format = 0;
    options->numeric_ids = 0;

    options->show_blocks = 0;

    options->sort_mode = SORT_NAME;
    options->time_mode = TIME_MTIME;
    options->block_mode = BLOCK_DEFAULT;

    options->directory_mode = DIR_NORMAL;
    options->print_mode = PRINT_AUTO;

    options->block_size =
        parse_block_size(
            getenv("BLOCKSIZE")
        );
}


int parse_options(int argc,
                  char *argv[],
                  Options *options)
{
    int opt;

    while ((opt = getopt(
                argc,
                argv,
                "AacdFfhiklnqRrSstuw")) != -1) {

        switch (opt) {

        case 'A':
            options->almost_all = 1;
            break;

        case 'a':
            options->show_all = 1;
            break;

        case 'c':
            options->time_mode =
                TIME_CTIME;
            break;

        case 'd':
            options->directory_mode =
                DIR_AS_FILE;
            break;

        case 'F':
            options->classify = 1;
            break;

        case 'f':
            options->no_sort = 1;
            break;

        /*
         * -h and -k override each other.
         */
        case 'h':
            options->block_mode =
                BLOCK_HUMAN;
            break;

        case 'i':
            options->show_inode = 1;
            break;

        case 'k':
            options->block_mode =
                BLOCK_KILOBYTES;
            break;

        /*
         * -l and -n override each other.
         */
        case 'l':
            options->long_format = 1;
            options->numeric_ids = 0;
            break;

        case 'n':
            options->long_format = 1;
            options->numeric_ids = 1;
            break;

        /*
         * -q and -w override each other.
         */
        case 'q':
            options->print_mode =
                PRINT_QUESTION;
            break;

        case 'R':
            options->directory_mode =
                DIR_RECURSIVE;
            break;

        case 'r':
            options->reverse = 1;
            break;

        case 'S':
            options->sort_mode =
                SORT_SIZE;
            break;

        case 's':
            options->show_blocks = 1;
            break;

        case 't':
            options->sort_mode =
                SORT_TIME;
            break;

        case 'u':
            options->time_mode =
                TIME_ATIME;
            break;

        case 'w':
            options->print_mode =
                PRINT_RAW;
            break;

        default:
            return -1;
        }
    }

    return 0;
}
