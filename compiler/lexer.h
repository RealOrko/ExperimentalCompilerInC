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
void lexer_init(Lexer *lexer, const char *source, const char *filename);
Token lexer_scan_token(Lexer *lexer);
void lexer_cleanup(Lexer *lexer);

// Helper functions
int lexer_is_at_end(Lexer *lexer);
char lexer_advance(Lexer *lexer);
char lexer_peek(Lexer *lexer);
char lexer_peek_next(Lexer *lexer);
int lexer_match(Lexer *lexer, char expected);

// Token scanners
Token lexer_make_token(Lexer *lexer, TokenType type);
Token lexer_error_token(Lexer *lexer, const char *message);
void lexer_skip_whitespace(Lexer *lexer);
Token lexer_scan_identifier(Lexer *lexer);
Token lexer_scan_number(Lexer *lexer);
Token lexer_scan_string(Lexer *lexer);
Token lexer_scan_char(Lexer *lexer);

// Identifier handling
TokenType lexer_identifier_type(Lexer *lexer);
TokenType lexer_check_keyword(Lexer *lexer, int start, int length, const char *rest, TokenType type);

// Indentation handling
void lexer_report_indentation_error(Lexer *lexer, int expected, int actual);

#endif // LEXER_H