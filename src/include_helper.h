#ifndef INCLUDE_HELPER_H
#define INCLUDE_HELPER_H

#include <stdio.h>
#include <stdlib.h>

typedef struct IncludedFile {
    char *filename;
    struct IncludedFile *next;
} IncludedFile;

int is_file_already_included(IncludedFile *list, const char *filename);

void add_included_file(IncludedFile **list, const char *filename);

void free_included_files(IncludedFile *list);

#endif