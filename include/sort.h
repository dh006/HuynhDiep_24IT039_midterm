#ifndef SORT_H
#define SORT_H

#include <stddef.h>

#include "options.h"

void sort_names(char **names,
                size_t count,
                const char *directory,
                const Options *options);

void reverse_names(char **names,
                   size_t count);

#endif
