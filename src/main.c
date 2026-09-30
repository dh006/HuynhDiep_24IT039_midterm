#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "options.h"
#include "listing.h"
#include "display.h"


/*
 * qsort() comparator needs access
 * to current options.
 */
static const Options *operand_options = NULL;


/*
 * Get the time selected by:
 *
 * default -> mtime
 * -c      -> ctime
 * -u      -> atime
 */
static time_t selected_time(
    const struct stat *st)
{
    if (operand_options->time_mode ==
        TIME_CTIME) {

        return st->st_ctime;
    }

    if (operand_options->time_mode ==
        TIME_ATIME) {

        return st->st_atime;
    }

    return st->st_mtime;
}


/*
 * Get metadata used to sort an operand.
 *
 * Normally a symbolic link to a directory
 * is followed.
 *
 * With -d, the symbolic link itself is used.
 */
static int get_operand_stat(
    const char *path,
    const Options *options,
    struct stat *result)
{
    struct stat lst;

    if (lstat(path, &lst) != 0)
        return -1;

    *result = lst;

    if (options->directory_mode ==
        DIR_AS_FILE) {

        return 0;
    }

    if (S_ISLNK(lst.st_mode)) {

        struct stat target;

        if (stat(path, &target) == 0 &&
            S_ISDIR(target.st_mode)) {

            *result = target;
        }
    }

    return 0;
}


/*
 * Sort top-level operands.
 *
 * Supports:
 *
 * default -> name
 * -S      -> size
 * -t      -> time
 */
static int compare_operands(
    const void *a,
    const void *b)
{
    const char *name1;
    const char *name2;

    struct stat st1;
    struct stat st2;

    name1 =
        *(const char **)a;

    name2 =
        *(const char **)b;


    /*
     * Normal lexicographical sorting.
     */
    if (operand_options->sort_mode ==
        SORT_NAME) {

        return strcmp(
            name1,
            name2
        );
    }


    if (get_operand_stat(
            name1,
            operand_options,
            &st1) != 0 ||

        get_operand_stat(
            name2,
            operand_options,
            &st2) != 0) {

        return strcmp(
            name1,
            name2
        );
    }


    /*
     * -S:
     * largest first.
     */
    if (operand_options->sort_mode ==
        SORT_SIZE) {

        if (st1.st_size >
            st2.st_size) {

            return -1;
        }

        if (st1.st_size <
            st2.st_size) {

            return 1;
        }

        return strcmp(
            name1,
            name2
        );
    }


    /*
     * -t:
     * newest first.
     */
    if (operand_options->sort_mode ==
        SORT_TIME) {

        time_t time1;
        time_t time2;

        time1 =
            selected_time(&st1);

        time2 =
            selected_time(&st2);

        if (time1 > time2)
            return -1;

        if (time1 < time2)
            return 1;

        return strcmp(
            name1,
            name2
        );
    }


    return strcmp(
        name1,
        name2
    );
}


/*
 * Reverse an operand array.
 */
static void reverse_operands(
    char **items,
    size_t count)
{
    size_t i;

    for (i = 0;
         i < count / 2;
         i++) {

        char *temp;

        temp =
            items[i];

        items[i] =
            items[count - 1 - i];

        items[count - 1 - i] =
            temp;
    }
}


/*
 * Determine whether an operand is
 * treated as a directory.
 */
static int is_directory_operand(
    const char *path,
    const Options *options)
{
    struct stat lst;

    if (options->directory_mode ==
        DIR_AS_FILE) {

        return 0;
    }

    if (lstat(path, &lst) != 0)
        return 0;


    if (S_ISDIR(lst.st_mode))
        return 1;


    /*
     * Follow a symbolic link argument
     * if it points to a directory.
     */
    if (S_ISLNK(lst.st_mode)) {

        struct stat target;

        if (stat(path, &target) == 0 &&
            S_ISDIR(target.st_mode)) {

            return 1;
        }
    }

    return 0;
}


int main(int argc, char *argv[])
{
    Options options;

    char **files;
    char **directories;

    size_t file_count;
    size_t directory_count;

    int status;
    int i;


    /*
     * Enable locale for -q.
     */
    setlocale(
        LC_CTYPE,
        ""
    );


    status = 0;

    file_count = 0;
    directory_count = 0;


    init_options(
        &options
    );


    if (parse_options(
            argc,
            argv,
            &options) != 0) {

        return 1;
    }


    /*
     * No operands.
     */
    if (optind >= argc) {

        if (options.directory_mode ==
            DIR_AS_FILE) {

            return display_file(
                ".",
                ".",
                &options
            );
        }

        return list_directory(
            ".",
            &options
        );
    }


    files = malloc(
        (size_t)(argc - optind) *
        sizeof(char *)
    );

    directories = malloc(
        (size_t)(argc - optind) *
        sizeof(char *)
    );


    if (files == NULL ||
        directories == NULL) {

        perror("malloc");

        free(files);
        free(directories);

        return 1;
    }


    /*
     * Separate non-directory and
     * directory operands.
     */
    for (i = optind;
         i < argc;
         i++) {

        struct stat st;


        if (lstat(
                argv[i],
                &st) != 0) {

            perror(argv[i]);

            status = 1;

            continue;
        }


        if (is_directory_operand(
                argv[i],
                &options)) {

            directories[
                directory_count
            ] = argv[i];

            directory_count++;
        }

        else {

            files[
                file_count
            ] = argv[i];

            file_count++;
        }
    }


    /*
     * Sort both operand groups separately.
     *
     * -f disables sorting.
     */
    if (!options.no_sort) {

        operand_options =
            &options;


        qsort(
            files,
            file_count,
            sizeof(char *),
            compare_operands
        );


        qsort(
            directories,
            directory_count,
            sizeof(char *),
            compare_operands
        );


        operand_options = NULL;


        /*
         * -r reverses whichever
         * ordering was selected.
         */
        if (options.reverse) {

            reverse_operands(
                files,
                file_count
            );

            reverse_operands(
                directories,
                directory_count
            );
        }
    }


    /*
     * Non-directory operands first.
     */
    for (i = 0;
         i < (int)file_count;
         i++) {

        if (display_file(
                files[i],
                files[i],
                &options) != 0) {

            status = 1;
        }
    }


    /*
     * Then directory operands.
     */
    {
        size_t valid_count;

        valid_count =
            file_count +
            directory_count;


        for (i = 0;
             i < (int)directory_count;
             i++) {

            /*
             * Separate output sections.
             */
            if (file_count > 0 ||
                i > 0) {

                putchar('\n');
            }


            /*
             * -R prints its own header.
             */
            if (options.directory_mode !=
                    DIR_RECURSIVE &&
                valid_count > 1) {

                printf(
                    "%s:\n",
                    directories[i]
                );
            }


            if (list_directory(
                    directories[i],
                    &options) != 0) {

                status = 1;
            }
        }
    }


    free(files);
    free(directories);


    return status;
}
