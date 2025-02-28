/**
 * lexer.h
 * Lexical analyzer for the compiler
 */

#ifndef LEXER_H
#define LEXER_H

#include "token.h"

typedef struct
{
    const char *start;    // Start of the current lexeme
    const char *current;  // Current character being examined
    int line;             // Current line number
    const char *filename; // Source filename

    // Add these fields for indentation tracking
    int *indent_stack;   // Stack of indentation levels
    int indent_size;     // Current size of the indent stack
    int indent_capacity; // Capacity of the indent stack
    int at_line_start;   // Whether we're at the start of a line
} Lexer;

// Lexer initialization and scanning
void init_lexer(Lexer *lexer, const char *source, const char *filename);
Token scan_token(Lexer *lexer);

// Helper functions
int is_at_end(Lexer *lexer);
char advance(Lexer *lexer);
char peek(Lexer *lexer);
char peek_next(Lexer *lexer);
int match(Lexer *lexer, char expected);

// Token scanners
Token make_token(Lexer *lexer, TokenType type);
Token error_token(Lexer *lexer, const char *message);
void skip_whitespace(Lexer *lexer);
Token scan_identifier(Lexer *lexer);
Token scan_number(Lexer *lexer);
Token scan_string(Lexer *lexer);
Token scan_char(Lexer *lexer);

// Identifier handling
TokenType identifier_type(Lexer *lexer);
TokenType check_keyword(Lexer *lexer, int start, int length, const char *rest, TokenType type);

#endif // LEXER_H