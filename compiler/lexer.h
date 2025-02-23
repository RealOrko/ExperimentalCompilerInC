#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOK_IMPORT,
    TOK_FUNC,
    TOK_IDENT,
    TOK_COLON,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_ARROW,
    TOK_STRING,
    TOK_SEMICOLON,
    TOK_NEWLINE,
    TOK_INDENT,
    TOK_DEDENT,
    TOK_DOT,
    TOK_EOF
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int indent_level;
} Token;

#define MAX_TOKENS 1000

extern int debug_enabled; // Declare, not define

Token* tokenize(const char* source, int* token_count);
void free_tokens(Token* tokens, int token_count);

#endif