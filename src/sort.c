#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sort.h"

/*
 * qsort() comparator cannot directly receive
 * our directory/options, so these are temporarily
 * stored while sorting.
 *
 * This program is single-threaded, so this is
 * acceptable for this project.
 */
static const char *current_directory = NULL;
static const Options *current_options = NULL;


/*
 * Build:
 *
 * directory + "/" + name
 */
static char *join_path(const char *directory,
                       const char *name)
{
    size_t length;
    char *path;

    length =
        strlen(directory) +
        strlen(name) +
        2;

    path = malloc(length);

    if (path == NULL)
        return NULL;

    snprintf(
        path,
        length,
        "%s/%s",
        directory,
        name
    );

    return path;
}


/*
 * Get the time selected by:
 *
 * default -> mtime
 * -c      -> ctime
 * -u      -> atime
 */
static time_t selected_time(const struct stat *st)
{
    if (current_options->time_mode == TIME_CTIME)
        return st->st_ctime;

    if (current_options->time_mode == TIME_ATIME)
        return st->st_atime;

    return st->st_mtime;
}


/*
 * Compare two file names.
 */
static int compare_entries(const void *a,
                           const void *b)
{
    const char *name1;
    const char *name2;

    char *path1;
    char *path2;

    struct stat st1;
    struct stat st2;

    name1 = *(const char **)a;
    name2 = *(const char **)b;

    /*
     * Normal lexicographical sorting.
     */
    if (current_options->sort_mode == SORT_NAME) {
        return strcmp(name1, name2);
    }

    path1 = join_path(
        current_directory,
        name1
    );

    path2 = join_path(
        current_directory,
        name2
    );

    if (path1 == NULL || path2 == NULL) {
        free(path1);
        free(path2);

        return strcmp(name1, name2);
    }

    if (lstat(path1, &st1) != 0 ||
        lstat(path2, &st2) != 0) {

        free(path1);
        free(path2);

        return strcmp(name1, name2);
    }

    free(path1);
    free(path2);

    /*
     * -S:
     * largest first.
     */
    if (current_options->sort_mode == SORT_SIZE) {

        if (st1.st_size > st2.st_size)
            return -1;

        if (st1.st_size < st2.st_size)
            return 1;

        /*
         * Same size:
         * fall back to name.
         */
        return strcmp(name1, name2);
    }

    /*
     * -t:
     * newest first.
     */
    if (current_options->sort_mode == SORT_TIME) {

        time_t time1;
        time_t time2;

        time1 = selected_time(&st1);
        time2 = selected_time(&st2);

        if (time1 > time2)
            return -1;

        if (time1 < time2)
            return 1;

        return strcmp(name1, name2);
    }

    return strcmp(name1, name2);
}


void sort_names(char **names,
                size_t count,
                const char *directory,
                const Options *options)
{
    current_directory = directory;
    current_options = options;

    qsort(
        names,
        count,
        sizeof(char *),
        compare_entries
    );

    current_directory = NULL;
    current_options = NULL;
}


void reverse_names(char **names,
                   size_t count)
{
    size_t i;

    for (i = 0; i < count / 2; i++) {

        char *temp;

        temp = names[i];

        names[i] =
            names[count - 1 - i];

        names[count - 1 - i] =
            temp;
    }
}
