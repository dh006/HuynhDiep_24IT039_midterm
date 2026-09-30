#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <pwd.h>
#include <grp.h>

#ifdef __linux__
#include <sys/sysmacros.h>
#endif

#include "display.h"


static char get_suffix(mode_t mode)
{
    if (S_ISDIR(mode))
        return '/';

    if (S_ISLNK(mode))
        return '@';

    if (S_ISSOCK(mode))
        return '=';

    if (S_ISFIFO(mode))
        return '|';

#ifdef S_ISWHT
    if (S_ISWHT(mode))
        return '%';
#endif

    if (mode &
        (S_IXUSR |
         S_IXGRP |
         S_IXOTH)) {

        return '*';
    }

    return '\0';
}


static void print_text(
    const char *text,
    const Options *options)
{
    int replace_nonprintable;

    if (options->print_mode ==
        PRINT_QUESTION) {

        replace_nonprintable = 1;
    }

    else if (options->print_mode ==
             PRINT_RAW) {

        replace_nonprintable = 0;
    }

    else {

        replace_nonprintable =
            isatty(STDOUT_FILENO);
    }


    if (!replace_nonprintable) {

        fputs(
            text,
            stdout
        );

        return;
    }


    {
        const char *ptr;
        mbstate_t state;

        ptr = text;

        memset(
            &state,
            0,
            sizeof(state)
        );


        while (*ptr != '\0') {

            wchar_t wc;
            size_t length;

            length = mbrtowc(
                &wc,
                ptr,
                MB_CUR_MAX,
                &state
            );


            if (length == (size_t)-1 ||
                length == (size_t)-2) {

                putchar('?');

                ptr++;

                memset(
                    &state,
                    0,
                    sizeof(state)
                );

                continue;
            }


            if (length == 0)
                break;


            if (iswprint(wc)) {

                fwrite(
                    ptr,
                    1,
                    length,
                    stdout
                );
            }

            else {

                putchar('?');
            }


            ptr += length;
        }
    }
}


static void mode_to_string(
    mode_t mode,
    char permissions[11])
{
    if (S_ISREG(mode))
        permissions[0] = '-';

    else if (S_ISDIR(mode))
        permissions[0] = 'd';

    else if (S_ISLNK(mode))
        permissions[0] = 'l';

    else if (S_ISCHR(mode))
        permissions[0] = 'c';

    else if (S_ISBLK(mode))
        permissions[0] = 'b';

    else if (S_ISFIFO(mode))
        permissions[0] = 'p';

    else if (S_ISSOCK(mode))
        permissions[0] = 's';

#ifdef S_ISWHT
    else if (S_ISWHT(mode))
        permissions[0] = 'w';
#endif

    else
        permissions[0] = '?';


    permissions[1] =
        (mode & S_IRUSR)
        ? 'r' : '-';

    permissions[2] =
        (mode & S_IWUSR)
        ? 'w' : '-';


    if (mode & S_ISUID)
        permissions[3] =
            (mode & S_IXUSR)
            ? 's' : 'S';
    else
        permissions[3] =
            (mode & S_IXUSR)
            ? 'x' : '-';


    permissions[4] =
        (mode & S_IRGRP)
        ? 'r' : '-';

    permissions[5] =
        (mode & S_IWGRP)
        ? 'w' : '-';


    if (mode & S_ISGID)
        permissions[6] =
            (mode & S_IXGRP)
            ? 's' : 'S';
    else
        permissions[6] =
            (mode & S_IXGRP)
            ? 'x' : '-';


    permissions[7] =
        (mode & S_IROTH)
        ? 'r' : '-';

    permissions[8] =
        (mode & S_IWOTH)
        ? 'w' : '-';


    if (mode & S_ISVTX)
        permissions[9] =
            (mode & S_IXOTH)
            ? 't' : 'T';
    else
        permissions[9] =
            (mode & S_IXOTH)
            ? 'x' : '-';


    permissions[10] = '\0';
}


static void print_owner(
    const struct stat *st,
    const Options *options)
{
    struct passwd *pw;

    if (options->numeric_ids) {

        printf(
            "%lu ",
            (unsigned long)
            st->st_uid
        );

        return;
    }


    pw = getpwuid(st->st_uid);


    if (pw != NULL)
        printf(
            "%s ",
            pw->pw_name
        );

    else
        printf(
            "%lu ",
            (unsigned long)
            st->st_uid
        );
}


static void print_group(
    const struct stat *st,
    const Options *options)
{
    struct group *gr;

    if (options->numeric_ids) {

        printf(
            "%lu ",
            (unsigned long)
            st->st_gid
        );

        return;
    }


    gr = getgrgid(st->st_gid);


    if (gr != NULL)
        printf(
            "%s ",
            gr->gr_name
        );

    else
        printf(
            "%lu ",
            (unsigned long)
            st->st_gid
        );
}


static time_t get_selected_time(
    const struct stat *st,
    const Options *options)
{
    if (options->time_mode ==
        TIME_CTIME)
        return st->st_ctime;

    if (options->time_mode ==
        TIME_ATIME)
        return st->st_atime;

    return st->st_mtime;
}


static void print_file_time(
    const struct stat *st,
    const Options *options)
{
    time_t selected;
    struct tm time_info;
    char buffer[64];

    selected =
        get_selected_time(
            st,
            options
        );


    if (localtime_r(
            &selected,
            &time_info) == NULL) {

        printf(
            "??? ?? ??:?? "
        );

        return;
    }


    strftime(
        buffer,
        sizeof(buffer),
        "%b %e %H:%M",
        &time_info
    );


    printf(
        "%s ",
        buffer
    );
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


static long long blocks_for_unit(
    const struct stat *st,
    long long unit_size)
{
    long long bytes;

    bytes =
        (long long)
        st->st_blocks *
        512LL;


    if (unit_size <= 0)
        unit_size = 512;


    return
        (bytes + unit_size - 1) /
        unit_size;
}


static void print_size(
    const struct stat *st,
    const Options *options)
{
    if (S_ISCHR(st->st_mode) ||
        S_ISBLK(st->st_mode)) {

        printf(
            "%u, %u ",
            major(st->st_rdev),
            minor(st->st_rdev)
        );

        return;
    }


    if (options->block_mode ==
        BLOCK_HUMAN) {

        char buffer[64];

        human_size(
            (long long)
            st->st_size,
            buffer,
            sizeof(buffer)
        );

        printf(
            "%s ",
            buffer
        );

        return;
    }


    printf(
        "%lld ",
        (long long)
        st->st_size
    );
}


static void print_blocks(
    const struct stat *st,
    const Options *options)
{
    if (options->block_mode ==
        BLOCK_HUMAN) {

        char buffer[64];

        long long bytes;

        bytes =
            (long long)
            st->st_blocks *
            512LL;


        human_size(
            bytes,
            buffer,
            sizeof(buffer)
        );


        printf(
            "%s ",
            buffer
        );

        return;
    }


    if (options->block_mode ==
        BLOCK_KILOBYTES) {

        printf(
            "%lld ",
            blocks_for_unit(
                st,
                1024
            )
        );

        return;
    }


    /*
     * Default:
     * use BLOCKSIZE, or 512 bytes
     * when BLOCKSIZE is not present.
     */
    printf(
        "%lld ",
        blocks_for_unit(
            st,
            options->block_size
        )
    );
}


static void print_link_target(
    const char *path,
    const Options *options)
{
    char target[4096];

    ssize_t length;


    length = readlink(
        path,
        target,
        sizeof(target) - 1
    );


    if (length < 0)
        return;


    target[length] = '\0';


    printf(" -> ");


    print_text(
        target,
        options
    );
}


int display_file(
    const char *path,
    const char *name,
    const Options *options)
{
    struct stat st;


    if (lstat(
            path,
            &st) != 0) {

        perror(path);

        return 1;
    }


    if (options->show_inode) {

        printf(
            "%lu ",
            (unsigned long)
            st.st_ino
        );
    }


    if (options->show_blocks) {

        print_blocks(
            &st,
            options
        );
    }


    if (options->long_format) {

        char permissions[11];

        mode_to_string(
            st.st_mode,
            permissions
        );


        printf(
            "%s ",
            permissions
        );


        printf(
            "%lu ",
            (unsigned long)
            st.st_nlink
        );


        print_owner(
            &st,
            options
        );


        print_group(
            &st,
            options
        );


        print_size(
            &st,
            options
        );


        print_file_time(
            &st,
            options
        );
    }


    print_text(
        name,
        options
    );


    if (options->classify) {

        char suffix;

        suffix =
            get_suffix(
                st.st_mode
            );


        if (suffix != '\0')
            putchar(suffix);
    }


    if (options->long_format &&
        S_ISLNK(st.st_mode)) {

        print_link_target(
            path,
            options
        );
    }


    putchar('\n');


    return 0;
}
