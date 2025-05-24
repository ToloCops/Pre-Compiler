#ifndef PRECOMPILER_H
#define PRECOMPILER_H

#include "include_helper.h"
#include "identifier_helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

void init_stats(Stats *stats);

void free_stats(Stats *stats);

void print_statistics(const Stats *stats);

char *process_includes(const char *code, Stats *stats, IncludedFile **included_files);

char *remove_comments(const char *code, Stats *stats);

void check_identifiers(const char *code, Stats *stats);

#endif /* PRECOMPILER_H */
