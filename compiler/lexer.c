#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

#define MAX_TOKEN_LEN 256

extern int debug_enabled; // Use extern, defined in main.c

Token* tokenize(const char* source, int* token_count) {
    Token* tokens = malloc(MAX_TOKENS * sizeof(Token));
    if (!tokens) return NULL;

    int count = 0;
    int pos = 0;
    int current_indent = 0;
    int indent_stack[100] = {0};
    int indent_depth = 0;

    while (source[pos] != '\0' && count < MAX_TOKENS - 1) {
        if (pos == 0 || source[pos-1] == '\n') {
            int spaces = 0;
            while (source[pos] == ' ') spaces++, pos++;
            if (spaces > current_indent && source[pos] != '\n' && source[pos] != '\0') {
                indent_stack[indent_depth++] = spaces;
                tokens[count].type = TOK_INDENT;
                tokens[count].value = strdup("INDENT");
                tokens[count].indent_level = spaces;
                current_indent = spaces;
                if (debug_enabled) printf("Token %d: %s (indent %d)\n", count, tokens[count].value, spaces);
                count++;
            }
            while (spaces < current_indent && indent_depth > 0) {
                indent_depth--;
                current_indent = indent_stack[indent_depth];
                tokens[count].type = TOK_DEDENT;
                tokens[count].value = strdup("DEDENT");
                tokens[count].indent_level = current_indent;
                if (debug_enabled) printf("Token %d: %s (indent %d)\n", count, tokens[count].value, current_indent);
                count++;
            }
        }

        while (isspace(source[pos]) && source[pos] != '\n') pos++;
        if (source[pos] == '\0') break;

        tokens[count].value = malloc(MAX_TOKEN_LEN);
        tokens[count].indent_level = current_indent;

        if (source[pos] == ':') {
            tokens[count].type = TOK_COLON;
            tokens[count].value[0] = ':';
            tokens[count].value[1] = '\0';
            pos++;
        } else if (source[pos] == '(') {
            tokens[count].type = TOK_LPAREN;
            tokens[count].value[0] = '(';
            tokens[count].value[1] = '\0';
            pos++;
        } else if (source[pos] == ')') {
            tokens[count].type = TOK_RPAREN;
            tokens[count].value[0] = ')';
            tokens[count].value[1] = '\0';
            pos++;
        } else if (source[pos] == '=') {
            if (source[pos+1] == '>') {
                tokens[count].type = TOK_ARROW;
                strcpy(tokens[count].value, "=>");
                pos += 2;
            } else {
                free(tokens[count].value);
                pos++;
                continue;
            }
        } else if (source[pos] == ';') {
            tokens[count].type = TOK_SEMICOLON;
            tokens[count].value[0] = ';';
            tokens[count].value[1] = '\0';
            pos++;
        } else if (source[pos] == '\n') {
            tokens[count].type = TOK_NEWLINE;
            tokens[count].value[0] = '\n';
            tokens[count].value[1] = '\0';
            pos++;
        } else if (source[pos] == '.') {
            tokens[count].type = TOK_DOT;
            tokens[count].value[0] = '.';
            tokens[count].value[1] = '\0';
            pos++;
        } else if (source[pos] == '"') {
            tokens[count].type = TOK_STRING;
            int i = 0;
            pos++;
            while (source[pos] != '"' && source[pos] != '\0' && i < MAX_TOKEN_LEN - 1) {
                tokens[count].value[i++] = source[pos++];
            }
            tokens[count].value[i] = '\0';
            if (source[pos] == '"') pos++;
        } else if (isalpha(source[pos])) {
            int i = 0;
            while (isalnum(source[pos]) && i < MAX_TOKEN_LEN - 1) {
                tokens[count].value[i++] = source[pos++];
            }
            tokens[count].value[i] = '\0';
            if (strcmp(tokens[count].value, "import") == 0) tokens[count].type = TOK_IMPORT;
            else if (strcmp(tokens[count].value, "func") == 0) tokens[count].type = TOK_FUNC;
            else tokens[count].type = TOK_IDENT;
        } else {
            free(tokens[count].value);
            pos++;
            continue;
        }
        if (debug_enabled) printf("Token %d: %s\n", count, tokens[count].value);
        count++;
    }

    while (indent_depth > 0 && count < MAX_TOKENS - 1) {
        indent_depth--;
        current_indent = indent_stack[indent_depth];
        tokens[count].type = TOK_DEDENT;
        tokens[count].value = strdup("DEDENT");
        tokens[count].indent_level = current_indent;
        if (debug_enabled) printf("Token %d: %s (indent %d)\n", count, tokens[count].value, current_indent);
        count++;
    }

    tokens[count].type = TOK_EOF;
    tokens[count].value = strdup("EOF");
    if (debug_enabled) printf("Token %d: %s\n", count, tokens[count].value);
    *token_count = count + 1;
    return tokens;
}

void free_tokens(Token* tokens, int token_count) {
    for (int i = 0; i < token_count; i++) {
        free(tokens[i].value);
    }
    free(tokens);
}