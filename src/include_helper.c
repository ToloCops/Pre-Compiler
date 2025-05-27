#include "include_helper.h"

char* duplicate_string(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy != NULL) {
        memcpy(copy, s, len);
    }
    return copy;
}

int is_file_already_included(IncludedFile *list, const char *filename) {
    IncludedFile *current = list;
    while (current) {
        if (strcmp(current->filename, filename) == 0) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void add_included_file(IncludedFile **list, const char *filename, long size_bytes, int num_lines) {
    if (is_file_already_included(*list, filename)) {
        return; // Il file è già incluso
    }
    IncludedFile *new_file = malloc(sizeof(IncludedFile));
    if (!new_file) {
        fprintf(stderr, "Error allocating memory for included file\n");
        exit(EXIT_FAILURE);
    }
    new_file->filename = duplicate_string(filename);
    new_file->size_bytes = size_bytes;
    new_file->num_lines = num_lines;
    new_file->next = *list;
    *list = new_file;
}

void free_included_files(IncludedFile *list) {
    IncludedFile *current = list;
    while (current) {
        IncludedFile *next = current->next;
        free(current->filename);
        free(current);
        current = next;
    }
}