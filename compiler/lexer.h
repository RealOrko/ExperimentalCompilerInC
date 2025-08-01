#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "arena.h"

typedef struct
{
    const char *start;
    const char *current;
    int line;
    const char *filename;
    int *indent_stack;
    int indent_size;
    int indent_capacity;
    int at_line_start;
    Arena *arena;
} Lexer;

void lexer_init(Arena *arena, Lexer *lexer, const char *source, const char *filename);
Token lexer_scan_token(Lexer *lexer);
void lexer_cleanup(Lexer *lexer);

int lexer_is_at_end(Lexer *lexer);
char lexer_advance(Lexer *lexer);
char lexer_peek(Lexer *lexer);
char lexer_peek_next(Lexer *lexer);
int lexer_match(Lexer *lexer, char expected);

Token lexer_make_token(Lexer *lexer, TokenType type);
Token lexer_error_token(Lexer *lexer, const char *message);
void lexer_skip_whitespace(Lexer *lexer);
Token lexer_scan_identifier(Lexer *lexer);
Token lexer_scan_number(Lexer *lexer);
Token lexer_scan_string(Lexer *lexer);
Token lexer_scan_char(Lexer *lexer);

TokenType lexer_identifier_type(Lexer *lexer);
TokenType lexer_check_keyword(Lexer *lexer, int start, int length, const char *rest, TokenType type);

void lexer_report_indentation_error(Lexer *lexer, int expected, int actual);

#endif