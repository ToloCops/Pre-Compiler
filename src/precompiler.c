#include "precompiler.h"

static char *duplicate_string(const char *s)
{
    if (!s)
        return NULL;
    size_t len = strlen(s);
    char *dup = malloc(len + 1);
    if (dup)
        strcpy(dup, s);
    return dup;
}

static void append_to_buffer(char **buffer, size_t *capacity, size_t *length, const char *str)
{
    size_t add_len = strlen(str);
    while (*length + add_len + 1 > *capacity)
    {
        *capacity *= 2;
        *buffer = realloc(*buffer, *capacity);
        if (!*buffer)
        {
            fprintf(stderr, "Errore di allocazione memoria\n");
            exit(EXIT_FAILURE);
        }
    }
    strcpy(*buffer + *length, str);
    *length += add_len;
}

void init_stats(Stats *stats, long input_file_size, int input_file_lines)
{
    if (!stats)
        return;

    stats->num_variables = 0;
    stats->num_errors = 0;
    stats->num_comment_lines_removed = 0;
    stats->num_included_files = 0;
    stats->input_file_size = input_file_size;
    stats->input_file_lines = input_file_lines;
    stats->output_file_size = 0;
    stats->output_file_lines = 0;
    stats->included_files = NULL;
}

void free_stats(Stats *stats)
{
    free_included_files(stats->included_files);
    stats->included_files = NULL;
}

void print_statistics(const Stats *stats)
{
    if (!stats)
        return;

    printf("Pre-compilation statistics:\n");
    printf("# of checked variables:     %d\n", stats->num_variables);
    printf("# of errors detected:       %d\n", stats->num_errors);
    printf("# of deleted comment lines: %d\n", stats->num_comment_lines_removed);
    printf("# of included files:        %d\n", stats->num_included_files);
    printf("Input file: %ld bytes, %d lines\n", stats->input_file_size, stats->input_file_lines);
    
    IncludedFile *file = stats->included_files;
    while (file) {
        printf("%s: %ld bytes, %d lines\n", file->filename, file->size_bytes, file->num_lines);
        file = file->next;
    }
    
    printf("Output file: %ld bytes, %d lines\n", stats->output_file_size, stats->output_file_lines);

}

char *process_includes(const char *code, Stats *stats)
{
    size_t capacity = strlen(code) + 1;
    size_t length = 0;
    char *result = malloc(capacity);
    if (!result)
    {
        fprintf(stderr, "Error allocating memory for result buffer\n");
        exit(EXIT_FAILURE);
    }
    result[0] = '\0';

    const char *pos = code;
    char line[1024];
    int line_num = 1;

    while (*pos)
    {

        size_t len = 0;
        const char *line_start = pos;
        while (pos[len] && pos[len] != '\n') {
            len++;
        }

        // Handle lines that are too long
        if (len >= sizeof(line)) {
            fprintf(stderr, "Warning (linea %d): linea troppo lunga, troncata a %zu caratteri:\n%.50s...\n", line_num, sizeof(line) - 1, line_start);
            strncpy(line, line_start, sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
        }
        else {
            strncpy(line, line_start, len);
            line[len] = '\0';
        }

        pos = line_start + len; // Set pointer to the end of the line

        if (*pos == '\n') {
            pos++;
        }

        // Check for #include directive
        if (strncmp(line, "#include", 8) == 0)
        {
            char filename[256];
            int found = 0;
            char *start = strchr(line, '\"');
            if (start)
            {
                start++;
                char *end = strchr(start, '\"');
                if (end)
                {
                    size_t fname_len = end - start;
                    if (fname_len < sizeof(filename))
                    {
                        strncpy(filename, start, fname_len);
                        filename[fname_len] = '\0';
                        found = 1;
                    }
                }
            }
            else
            {
                start = strchr(line, '<');
                if (start)
                {
                    start++;
                    char *end = strchr(start, '>');
                    if (end)
                    {
                        size_t fname_len = end - start;
                        if (fname_len < sizeof(filename))
                        {
                            strncpy(filename, start, fname_len);
                            filename[fname_len] = '\0';
                            found = 1;
                        }
                    }
                }
            }
            if (found)
            {
                // Avoid including the same file multiple times
                if (is_file_already_included(stats->included_files, filename)) {
                    continue;
                }
                FILE *fp = fopen(filename, "r");
                if (fp)
                {
                    fseek(fp, 0, SEEK_END);
                    long fsize = ftell(fp);
                    rewind(fp);

                    char *file_content = malloc(fsize + 1);
                    if (file_content)
                    {
                        size_t read_bytes = fread(file_content, 1, fsize, fp);
                        file_content[read_bytes] = '\0';

                        int num_lines = 1;
                        for (size_t i = 0; i < read_bytes; i++)
                        {
                            if (file_content[i] == '\n')
                            {
                                num_lines++;
                            }
                        }

                        add_included_file(&stats->included_files, filename, fsize, num_lines);

                        // Recursively process includes in the included file
                        char *include_free = process_includes(file_content, stats);  // Process the included file recursively
                        free(file_content);
                        append_to_buffer(&result, &capacity, &length, include_free);
                        free(include_free);

                        stats->num_included_files++;
                    }
                    fclose(fp);
                }
                else
                {
                    // Not a valid include, just copy the line
                    fprintf(stderr, "Error opening file to include: %s\n", filename);
                }
            }
            else
            {
                // Normal line, just copy
                append_to_buffer(&result, &capacity, &length, line);
                append_to_buffer(&result, &capacity, &length, "\n");
            }
        }
        else
        {
            append_to_buffer(&result, &capacity, &length, line);
            append_to_buffer(&result, &capacity, &length, "\n");
        }
        line_num++;
    }
    return result;
}

char *remove_comments(const char *code, Stats *stats)
{
    size_t capacity = 1024;
    size_t length = 0;
    char *result = malloc(capacity);
    if (!result)
    {
        fprintf(stderr, "Errore di allocazione in remove_comments\n");
        exit(EXIT_FAILURE);
    }
    result[0] = '\0';

    const char *p = code;
    bool code_line = false;
    while (*p)
    {
        if (p[0] == '/' && p[1] == '/')
        {
            // Inline comment: skip until newline
            while (*p && *p != '\n')
            {
                p++;
            }
            if (*p == '\n')
            {
                if (code_line) {
                    append_to_buffer(&result, &capacity, &length, "\n");
                } else {
                    stats->num_comment_lines_removed++;
                }
                p++;
                code_line = false;
            }
        }
        else if (p[0] == '/' && p[1] == '*')
        {
            // Comment block: skip until "*/" copying newlines
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/'))
            {
                if (*p == '\n')
                {
                    stats->num_comment_lines_removed++;
                    code_line = false;
                }
                p++;
            }
            if (*p) {
                p += 2; // Jump over "*/"
                while (*p == '\n' || *p == '\r') {
                    if (*p == '\n') {
                        stats->num_comment_lines_removed++;
                    }
                    p++; // Jump over newline
                }
                if (code_line) {
                    append_to_buffer(&result, &capacity, &length, "\n");
                }
                code_line = false;
            }
        }
        else
        {
            // Normal code: copy character
            if (*p == '\n' || *p == '\r') {
                code_line = false;
            } else {
                code_line = true;
            }
            char tmp[2] = {*p, '\0'};
            append_to_buffer(&result, &capacity, &length, tmp);
            p++;
        }
    }
    return result;
}

void check_identifiers(const char *code, Stats *stats) {
    char token[256];
    int token_index = 0;
    int line_number = 1;
    ParserState state = START;
    int in_typedef = 0;

    for (const char *p = code; *p != '\0'; p++) {
        if (*p == '\n') line_number++;

        if (isalnum(*p) || *p == '_' || *p == '-') {
            if (token_index < (int)sizeof(token) - 1)
                token[token_index++] = *p;
            continue;
        }

        if (*p == '*') {
            if (state == TYPE) {
                continue;
            }
        }

        if (token_index > 0) {
            token[token_index] = '\0';
            token_index = 0;

            if (strcmp(token, "typedef") == 0) {
                in_typedef = 1;
                state = TYPE;
                continue;
            }

            switch (state) {
                case START:
                case TYPE:
                    if (is_type(token)) {
                        state = TYPE;
                    } else {
                        state = START;
                    }
                    break;

                case EXPECT_ID:
                    stats->num_variables++;
                    if (!is_valid_identifier(token)) {
                        stats->num_errors++;
                        printf("Errore: identificatore non valido '%s' alla linea %d\n", token, line_number);
                    }

                    if (in_typedef) {
                        add_custom_type(token);
                        in_typedef = 0;
                        state = START;
                    } else {
                        state = AFTER_ID;
                    }
                    break;

                case AFTER_ID:
                case SKIP_INIT:
                    break;
            }
        }

        switch (*p) {
            case ';':
                state = START;
                break;
            case ',':
                if (state == AFTER_ID || state == SKIP_INIT)
                    state = EXPECT_ID;
                break;
            case '=':
                if (state == AFTER_ID)
                    state = SKIP_INIT;
                break;
            default:
                if (state == TYPE)
                    state = EXPECT_ID;
                break;
        }
    }
}

