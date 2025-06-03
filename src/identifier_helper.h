#ifndef IDENTIFIER_HELPER_H
#define IDENTIFIER_HELPER_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_CUSTOM_TYPES 100

// Parser states for the state machine
typedef enum { START, TYPE, EXPECT_ID, AFTER_ID, SKIP_INIT } ParserState;

// Check if the token is a standard type keyword
int is_type_keyword(const char *token);

// Check if the token is a custom type defined by the user
int is_custom_type(const char *token);

void add_custom_type(const char *token);

// Check if the token is a type keyword or a custom type
int is_type(const char *token);

int is_valid_identifier(const char *token);

#endif