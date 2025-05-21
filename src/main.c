#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "precompiler.h"

int main(int argc, char *argv[])
{
    char *input_file = NULL;
    char *output_file = NULL;

    int verbose = 0;

    static struct option long_options[] = {
        {"in",      required_argument, 0, 'i'},
        {"out",     required_argument, 0, 'o'},
        {"verbose", no_argument,       0, 'v'},
        {0, 0, 0, 0}                             // Used to tell the compiler that this is the end of the options
    };

    int c;

    while ((c = getopt_long(argc, argv, "i:o:v", long_options, NULL)) != -1)
    {
        switch (c)
        {
        case 'i':
            input_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        case '?': // Handle unknown options
        default:
            fprintf(stderr, "Usage: %s --in <input_file> [--out <output_file>] [--verbose]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (input_file == NULL) // Check if user provided input file without -i or --in
    {
        if (optind < argc)
        {
            input_file = argv[optind];
        }
        else
        {
            fprintf(stderr, "Input file is required.\n");
            fprintf(stderr, "Usage: %s --in <input_file> [--out <output_file>] [--verbose]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    FILE *fp = fopen(input_file, "r");
    if (fp == NULL)
    {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    rewind(fp);

    char *code = malloc(file_size + 1); // +1 for null terminator
    if (code == NULL)
    {
        perror("Error allocating memory for code");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    size_t read_bytes = fread(code, 1, file_size, fp); // Read the file and store it in code, returning the number of elements of size 1 (byte) read
    if (read_bytes != file_size)
    {
        perror("Error reading input file"); // This could be, for example, due to a modification of the file after seeking
        free(code);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    code[file_size] = '\0';
    fclose(fp);

    Stats stats;
    init_stats(&stats);

    char *code_with_includes = process_includes(code, &stats);
    free(code);

    char *code_no_comments = remove_comments(code_with_includes, &stats);
    free(code_with_includes);

    check_identifiers(code_no_comments, &stats);

    if (output_file)
    {
        FILE *out_fp = fopen(output_file, "w");
        if (!out_fp)
        {
            perror("Error opening output file");
            free(code_no_comments);
            exit(EXIT_FAILURE);
        }
        if (fwrite(code_no_comments, 1, strlen(code_no_comments), out_fp) < strlen(code_no_comments))
        {
            perror("Error writing to output file");
            free(code_no_comments);
            fclose(out_fp);
            exit(EXIT_FAILURE);
        }
        fclose(out_fp);
    }
    else
    {
        printf("%s", code_no_comments);
    }

    if (verbose)
    {
        print_statistics(&stats);
    }

    free(code_no_comments);
    free_stats(&stats);

    return 0;
}