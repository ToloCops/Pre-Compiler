#ifndef PRECOMPILER_H
#define PRECOMPILER_H

#include "include_helper.h"
#include "identifier_helper.h"

#include <stdio.h>
#include <stdbool.h>
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

    IncludedFile *included_files; 

} Stats;

void init_stats(Stats *stats, long input_file_size, int input_file_lines);

void free_stats(Stats *stats);

void print_statistics(const Stats *stats);

char *process_includes(const char *code, Stats *stats);

char *remove_comments(const char *code, Stats *stats);

void check_identifiers(const char *code, Stats *stats);

#endif /* PRECOMPILER_H */
