#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "listing.h"
#include "sort.h"
#include "display.h"


static char *join_path(
    const char *directory,
    const char *name)
{
    size_t length;
    char *result;

    length =
        strlen(directory) +
        strlen(name) +
        2;


    result =
        malloc(length);


    if (result == NULL)
        return NULL;


    snprintf(
        result,
        length,
        "%s/%s",
        directory,
        name
    );


    return result;
}


static void human_size(
    long long bytes,
    char *buffer,
    size_t buffer_size)
{
    static const char units[] = {
        'B',
        'K',
        'M',
        'G',
        'T',
        'P'
    };

    double size;
    int unit;

    size = (double)bytes;
    unit = 0;


    while (size >= 1024.0 &&
           unit < 5) {

        size /= 1024.0;

        unit++;
    }


    if (unit == 0) {

        snprintf(
            buffer,
            buffer_size,
            "%.0f%c",
            size,
            units[unit]
        );
    }

    else {

        snprintf(
            buffer,
            buffer_size,
            "%.1f%c",
            size,
            units[unit]
        );
    }
}


static long long convert_blocks(
    long long total512,
    long long unit_size)
{
    long long bytes;

    if (unit_size <= 0)
        unit_size = 512;


    bytes =
        total512 * 512LL;


    return
        (bytes + unit_size - 1) /
        unit_size;
}


static int print_total(
    const char *path,
    char **names,
    size_t count,
    const Options *options)
{
    long long total512;

    size_t i;


    total512 = 0;


    for (i = 0;
         i < count;
         i++) {

        char *full_path;

        struct stat st;


        full_path =
            join_path(
                path,
                names[i]
            );


        if (full_path == NULL) {

            perror("malloc");

            return 1;
        }


        if (lstat(
                full_path,
                &st) != 0) {

            perror(full_path);

            free(full_path);

            continue;
        }


        total512 +=
            (long long)
            st.st_blocks;


        free(full_path);
    }


    if (options->block_mode ==
        BLOCK_HUMAN) {

        char buffer[64];

        human_size(
            total512 * 512LL,
            buffer,
            sizeof(buffer)
        );


        printf(
            "total %s\n",
            buffer
        );
    }


    else if (options->block_mode ==
             BLOCK_KILOBYTES) {

        printf(
            "total %lld\n",
            convert_blocks(
                total512,
                1024
            )
        );
    }


    else {

        printf(
            "total %lld\n",
            convert_blocks(
                total512,
                options->block_size
            )
        );
    }


    return 0;
}


static int list_directory_internal(
    const char *path,
    const Options *options,
    int print_header)
{
    DIR *dir;

    struct dirent *entry;

    char **names;

    size_t count;
    size_t capacity;
    size_t i;

    int status;


    names = NULL;

    count = 0;
    capacity = 0;

    status = 0;


    if (print_header) {

        printf(
            "%s:\n",
            path
        );
    }


    dir = opendir(path);


    if (dir == NULL) {

        perror(path);

        return 1;
    }


    while ((entry =
            readdir(dir)) != NULL) {


        if (!options->show_all &&
            !options->almost_all) {

            if (entry->d_name[0] == '.')
                continue;
        }


        if (options->almost_all &&
            !options->show_all) {

            if (strcmp(
                    entry->d_name,
                    ".") == 0 ||

                strcmp(
                    entry->d_name,
                    "..") == 0) {

                continue;
            }
        }


        if (count == capacity) {

            size_t new_capacity;

            char **temp;


            if (capacity == 0)
                new_capacity = 16;

            else
                new_capacity =
                    capacity * 2;


            temp = realloc(
                names,
                new_capacity *
                sizeof(char *)
            );


            if (temp == NULL) {

                perror("realloc");

                closedir(dir);


                for (i = 0;
                     i < count;
                     i++) {

                    free(names[i]);
                }


                free(names);

                return 1;
            }


            names = temp;

            capacity =
                new_capacity;
        }


        names[count] =
            strdup(
                entry->d_name
            );


        if (names[count] == NULL) {

            perror("strdup");

            closedir(dir);


            for (i = 0;
                 i < count;
                 i++) {

                free(names[i]);
            }


            free(names);

            return 1;
        }


        count++;
    }


    closedir(dir);


    if (!options->no_sort) {

        sort_names(
            names,
            count,
            path,
            options
        );


        if (options->reverse) {

            reverse_names(
                names,
                count
            );
        }
    }


    if (options->long_format ||

        (options->show_blocks &&
         isatty(STDOUT_FILENO))) {

        if (print_total(
                path,
                names,
                count,
                options) != 0) {

            status = 1;
        }
    }


    for (i = 0;
         i < count;
         i++) {

        char *full_path;


        full_path =
            join_path(
                path,
                names[i]
            );


        if (full_path == NULL) {

            perror("malloc");

            status = 1;

            continue;
        }


        if (display_file(
                full_path,
                names[i],
                options) != 0) {

            status = 1;
        }


        free(full_path);
    }


    if (options->directory_mode ==
        DIR_RECURSIVE) {

        for (i = 0;
             i < count;
             i++) {

            char *full_path;

            struct stat st;


            if (strcmp(
                    names[i],
                    ".") == 0 ||

                strcmp(
                    names[i],
                    "..") == 0) {

                continue;
            }


            full_path =
                join_path(
                    path,
                    names[i]
                );


            if (full_path == NULL) {

                perror("malloc");

                status = 1;

                continue;
            }


            if (lstat(
                    full_path,
                    &st) != 0) {

                perror(full_path);

                free(full_path);

                status = 1;

                continue;
            }


            if (S_ISDIR(st.st_mode)) {

                putchar('\n');


                if (list_directory_internal(
                        full_path,
                        options,
                        1) != 0) {

                    status = 1;
                }
            }


            free(full_path);
        }
    }


    for (i = 0;
         i < count;
         i++) {

        free(names[i]);
    }


    free(names);


    return status;
}


int list_directory(
    const char *path,
    const Options *options)
{
    int print_header;

    print_header =
        (options->directory_mode ==
         DIR_RECURSIVE);


    return list_directory_internal(
        path,
        options,
        print_header
    );
}
