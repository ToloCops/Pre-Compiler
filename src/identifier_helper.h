#ifndef IDENTIFIER_HELPER_H
#define IDENTIFIER_HELPER_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_CUSTOM_TYPES 100

typedef enum { START, TYPE, EXPECT_ID, AFTER_ID, SKIP_INIT } ParserState;

char *custom_types[MAX_CUSTOM_TYPES];
int custom_type_count = 0;

int is_type_keyword(const char *token);

int is_custom_type(const char *token);

void add_custom_type(const char *token);

int is_type(const char *token);

int is_valid_identifier(const char *token);

#endif