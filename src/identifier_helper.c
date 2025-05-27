#include "identifier_helper.h"

char *custom_types[MAX_CUSTOM_TYPES];
int custom_type_count = 0;

int is_type_keyword(const char *token) {
    const char *types[] = {
        "int", "char", "float", "double", "long", "short", "unsigned",
        "signed", "void", "struct", "union", "enum", NULL
    };
    for (int i = 0; types[i] != NULL; i++) {
        if (strcmp(token, types[i]) == 0)
            return 1;
    }
    return 0;
}

int is_custom_type(const char *token) {
    for (int i = 0; i < custom_type_count; i++) {
        if (strcmp(token, custom_types[i]) == 0)
            return 1;
    }
    return 0;
}

void add_custom_type(const char *token) {
    if (custom_type_count < MAX_CUSTOM_TYPES) {
        custom_types[custom_type_count++] = strdup(token);
    }
}


int is_type(const char *token) {
    return is_type_keyword(token) || is_custom_type(token);
}

int is_valid_identifier(const char *token) {
    if (strchr(token, '-') != NULL) return 0;
    if (!(isalpha(token[0]) || token[0] == '_')) return 0;
    for (int i = 1; token[i] != '\0'; i++) {
        if (!(isalnum(token[i]) || token[i] == '_')) return 0;
    }
    return 1;
}