#ifndef PRECOMPILER_H
#define PRECOMPILER_H

#include <stdio.h>
#include <stdlib.h>

/* Statistics */
typedef struct
{
    int num_variables;
    int num_errors;
    int num_comment_lines_removed;
    int num_included_files;

    long input_file_size;
    int input_file_lines;

    long output_file_size;
    int output_file_lines;

} Stats;

typedef struct IncludedFile {
    char *filename;
    struct IncludedFile *next;
} IncludedFile;

void init_stats(Stats *stats);

void free_stats(Stats *stats);

void print_statistics(const Stats *stats);

char *process_includes(const char *code, Stats *stats, IncludedFile **included_files);

int is_file_already_included(IncludedFile *list, const char *filename);

void add_included_file(IncludedFile **list, const char *filename);

void free_included_files(IncludedFile *list);

char *remove_comments(const char *code, Stats *stats);

void check_identifiers(const char *code, Stats *stats);

int is_valid_identifier(const char *token);

int is_type_keyword(const char *token);

#endif /* PRECOMPILER_H */
