#include "precompiler.h"

/* Funzione ausiliaria: duplica una stringa usando malloc */
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

/* Funzione ausiliaria per appendere una stringa in un buffer dinamico.
   Il buffer e la sua capacità, insieme alla lunghezza corrente, vengono gestiti
   per garantire spazio sufficiente. */
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

/* Inizializza la struttura delle statistiche */
void init_stats(Stats *stats)
{
    if (stats)
    {
        stats->num_variables = 0;
        stats->num_errors = 0;
        stats->num_comment_lines_removed = 0;
        stats->num_included_files = 0;
        stats->input_file_size = 0;
        stats->input_file_lines = 0;
        stats->output_file_size = 0;
        stats->output_file_lines = 0;
    }
}

/* Libera eventuali risorse allocate per le statistiche (attualmente non necessarie) */
void free_stats(Stats *stats)
{
    (void)stats; // nessuna risorsa da liberare per ora
}

/* Stampa le statistiche sullo standard output */
void print_statistics(const Stats *stats)
{
    if (!stats)
        return;

    printf("Statistiche di elaborazione:\n");
    printf("Numero di variabili controllate: %d\n", stats->num_variables);
    printf("Numero di errori rilevati: %d\n", stats->num_errors);
    printf("Numero di righe di commento eliminate: %d\n", stats->num_comment_lines_removed);
    printf("Numero di file inclusi: %d\n", stats->num_included_files);
    printf("File di input: %ld byte, %d righe\n", stats->input_file_size, stats->input_file_lines);
    printf("File di output: %ld byte, %d righe\n", stats->output_file_size, stats->output_file_lines);
}

/*
   Funzione process_includes: analizza il codice riga per riga.
   Se trova una direttiva #include, estrae il nome del file (da virgolette o angolari),
   apre il file e ne copia il contenuto al posto della direttiva.
*/
char *process_includes(const char *code, Stats *stats, IncludedFile **included_files)
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
                if (is_file_already_included(*included_files, filename)) {
                    continue;
                }
                add_included_file(included_files, filename);
                FILE *fp = fopen(filename, "r");
                if (fp)
                {
                    stats->num_included_files++;
                    fseek(fp, 0, SEEK_END);
                    long fsize = ftell(fp);
                    rewind(fp);
                    char *file_content = malloc(fsize + 1);
                    if (file_content)
                    {
                        size_t read_bytes = fread(file_content, 1, fsize, fp);
                        file_content[read_bytes] = '\0';
                    }
                    fclose(fp);
                    char *include_free = process_includes(file_content, stats, included_files);  // Process the included file recursively
                    free(file_content);
                    append_to_buffer(&result, &capacity, &length, include_free);
                    free(include_free);
                    append_to_buffer(&result, &capacity, &length, "\n");
                }
                else
                {
                    fprintf(stderr, "Error opening file to include: %s\n", filename);
                }
            }
            else
            {
                /* Se non viene trovato un filename, copia la riga così com'è */
                append_to_buffer(&result, &capacity, &length, line);
                append_to_buffer(&result, &capacity, &length, "\n");
            }
        }
        else
        {
            /* Linea normale: copia nel risultato */
            append_to_buffer(&result, &capacity, &length, line);
            append_to_buffer(&result, &capacity, &length, "\n");
        }
        line_num++;
    }
    return result;
}

/*
   Funzione remove_comments: rimuove i commenti dal codice.
   Gestisce sia i commenti su singola linea (//) che quelli a blocchi (/* ... * /).
   I newline vengono copiati per mantenere la corrispondenza delle righe.
*/
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
    while (*p)
    {
        if (p[0] == '/' && p[1] == '/')
        {
            /* Commento su singola linea: salta fino a newline */
            while (*p && *p != '\n')
            {
                p++;
            }
            stats->num_comment_lines_removed++;
            if (*p == '\n')
            {
                append_to_buffer(&result, &capacity, &length, "\n");
                p++;
            }
        }
        else if (p[0] == '/' && p[1] == '*')
        {
            /* Commento a blocchi: salta fino a "*\/", copiando i newline incontrati */
            p += 2;
            while (*p && !(p[0] == '*' && p[1] == '/'))
            {
                if (*p == '\n')
                {
                    stats->num_comment_lines_removed++;
                    append_to_buffer(&result, &capacity, &length, "\n");
                }
                p++;
            }
            if (*p)
                p += 2; // salta "*/"
        }
        else
        {
            /* Carattere normale: copia */
            char tmp[2] = {*p, '\0'};
            append_to_buffer(&result, &capacity, &length, tmp);
            p++;
        }
    }
    return result;
}

int is_valid_identifier(const char *token) {
    if (strchr(token, '-') != NULL)
        return 0;
    if (!(isalpha(token[0]) || token[0] == '_'))
        return 0;
    for (int i = 1; token[i] != '\0'; i++) {
        if (!(isalnum(token[i]) || token[i] == '_'))
            return 0;
    }
    return 1;
}

int is_type_keyword(const char *token) {
    const char *types[] = {"int", "char", "float", "double", "long", "short", "unsigned", NULL};
    for (int i = 0; types[i] != NULL; i++) {
        if (strcmp(token, types[i]) == 0)
            return 1;
    }
    return 0;
}

void check_identifiers(const char *code, Stats *stats) {
    char token[256];
    int token_index = 0;
    int expecting_identifier = 0;
    int line_number = 1;

    for (const char *p = code; *p != '\0'; p++) {
        if (*p == '\n') line_number++;

        if (isspace(*p)) {
            if (token_index > 0) {
                token[token_index] = '\0';

                if (expecting_identifier) {
                    stats->num_variables++;
                    if (!is_valid_identifier(token)) {
                        stats->num_errors++;
                        printf("Errore: identificatore non valido '%s' alla linea %d\n", token, line_number);
                    }
                    // Mantieni expecting_identifier attivo (potrebbe esserci ',')
                } else if (is_type_keyword(token)) {
                    expecting_identifier = 1;
                }

                token_index = 0;
            }
            continue;
        }

        if (isalnum(*p) || *p == '_' || *p == '-') {
            if (token_index < (int)sizeof(token) - 1)
                token[token_index++] = *p;
        } else {
            if (token_index > 0) {
                token[token_index] = '\0';

                if (expecting_identifier) {
                    stats->num_variables++;
                    if (!is_valid_identifier(token)) {
                        stats->num_errors++;
                        printf("Errore: identificatore non valido '%s' alla linea %d\n", token, line_number);
                    }
                } else if (is_type_keyword(token)) {
                    expecting_identifier = 1;
                }

                token_index = 0;
            }

            // Gestione dei delimitatori
            if (*p == ',') {
                // Continuiamo ad aspettare un identificatore
                continue;
            } else if (*p == ';') {
                expecting_identifier = 0;
            } else {
                // '=' o altro → ignora
            }
        }
    }

    // Gestione token residuo
    if (token_index > 0) {
        token[token_index] = '\0';
        if (expecting_identifier) {
            stats->num_variables++;
            if (!is_valid_identifier(token)) {
                stats->num_errors++;
                printf("Errore: identificatore non valido '%s' alla linea %d\n", token, line_number);
            }
        }
    }
}

