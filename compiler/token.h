#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>

typedef enum
{
    TOKEN_EOF,
    TOKEN_INDENT,
    TOKEN_DEDENT,
    TOKEN_NEWLINE,
    TOKEN_INT_LITERAL,
    TOKEN_LONG_LITERAL,
    TOKEN_DOUBLE_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_INTERPOL_STRING,
    TOKEN_BOOL_LITERAL,
    TOKEN_IDENTIFIER,
    TOKEN_FN,
    TOKEN_VAR,
    TOKEN_RETURN,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,
    TOKEN_IMPORT,
    TOKEN_NIL,
    TOKEN_INT,
    TOKEN_LONG,
    TOKEN_DOUBLE,
    TOKEN_CHAR,
    TOKEN_STR,
    TOKEN_BOOL,
    TOKEN_VOID,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_MODULO,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_PLUS_PLUS,
    TOKEN_MINUS_MINUS,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_ARROW,
    TOKEN_ERROR
} TokenType;

typedef union
{
    int64_t int_value;
    double double_value;
    char char_value;
    const char *string_value;
    int bool_value;
} LiteralValue;

typedef struct
{
    TokenType type;
    const char *start;
    int length;
    int line;
    LiteralValue literal;
} Token;

void token_init(Token *token, TokenType type, const char *start, int length, int line);
void token_set_int_literal(Token *token, int64_t value);
void token_set_double_literal(Token *token, double value);
void token_set_char_literal(Token *token, char value);
void token_set_string_literal(Token *token, const char *value);
void token_set_bool_literal(Token *token, int value);
const char *token_type_to_string(TokenType type);
void token_print(Token *token);

#endif