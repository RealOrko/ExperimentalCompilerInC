/**
 * token.c
 * Implementation of token functions
 */

#include "token.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void init_token(Token *token, TokenType type, const char *start, int length, int line)
{
    token->type = type;
    token->start = start;
    token->length = length;
    token->line = line;
    // Initialize the union to 0
    token->literal.int_value = 0;
}

void token_set_int_literal(Token *token, int64_t value)
{
    token->literal.int_value = value;
}

void token_set_double_literal(Token *token, double value)
{
    token->literal.double_value = value;
}

void token_set_char_literal(Token *token, char value)
{
    token->literal.char_value = value;
}

void token_set_string_literal(Token *token, const char *value)
{
    token->literal.string_value = value;
}

void token_set_bool_literal(Token *token, int value)
{
    token->literal.bool_value = value;
}

const char *token_type_to_string(TokenType type)
{
    switch (type)
    {
    case TOKEN_EOF:
        return "EOF";
    case TOKEN_INT_LITERAL:
        return "INT_LITERAL";
    case TOKEN_LONG_LITERAL:
        return "LONG_LITERAL";
    case TOKEN_DOUBLE_LITERAL:
        return "DOUBLE_LITERAL";
    case TOKEN_CHAR_LITERAL:
        return "CHAR_LITERAL";
    case TOKEN_STRING_LITERAL:
        return "STRING_LITERAL";
    case TOKEN_BOOL_LITERAL:
        return "BOOL_LITERAL";
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_FN:
        return "FN";
    case TOKEN_VAR:
        return "VAR";
    case TOKEN_RETURN:
        return "RETURN";
    case TOKEN_IF:
        return "IF";
    case TOKEN_ELSE:
        return "ELSE";
    case TOKEN_FOR:
        return "FOR";
    case TOKEN_WHILE:
        return "WHILE";
    case TOKEN_IMPORT:
        return "IMPORT";
    case TOKEN_NIL:
        return "NIL";
    case TOKEN_INT:
        return "INT";
    case TOKEN_LONG:
        return "LONG";
    case TOKEN_DOUBLE:
        return "DOUBLE";
    case TOKEN_CHAR:
        return "CHAR";
    case TOKEN_STR:
        return "STR";
    case TOKEN_BOOL:
        return "BOOL";
    case TOKEN_VOID:
        return "VOID";
    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_MINUS:
        return "MINUS";
    case TOKEN_STAR:
        return "STAR";
    case TOKEN_SLASH:
        return "SLASH";
    case TOKEN_MODULO:
        return "MODULO";
    case TOKEN_EQUAL:
        return "EQUAL";
    case TOKEN_EQUAL_EQUAL:
        return "EQUAL_EQUAL";
    case TOKEN_BANG:
        return "BANG";
    case TOKEN_BANG_EQUAL:
        return "BANG_EQUAL";
    case TOKEN_LESS:
        return "LESS";
    case TOKEN_LESS_EQUAL:
        return "LESS_EQUAL";
    case TOKEN_GREATER:
        return "GREATER";
    case TOKEN_GREATER_EQUAL:
        return "GREATER_EQUAL";
    case TOKEN_AND:
        return "AND";
    case TOKEN_OR:
        return "OR";
    case TOKEN_PLUS_PLUS:
        return "PLUS_PLUS";
    case TOKEN_MINUS_MINUS:
        return "MINUS_MINUS";
    case TOKEN_LEFT_PAREN:
        return "LEFT_PAREN";
    case TOKEN_RIGHT_PAREN:
        return "RIGHT_PAREN";
    case TOKEN_LEFT_BRACE:
        return "LEFT_BRACE";
    case TOKEN_RIGHT_BRACE:
        return "RIGHT_BRACE";
    case TOKEN_LEFT_BRACKET:
        return "LEFT_BRACKET";
    case TOKEN_RIGHT_BRACKET:
        return "RIGHT_BRACKET";
    case TOKEN_SEMICOLON:
        return "SEMICOLON";
    case TOKEN_COLON:
        return "COLON";
    case TOKEN_COMMA:
        return "COMMA";
    case TOKEN_DOT:
        return "DOT";
    case TOKEN_ARROW:
        return "ARROW";
    case TOKEN_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

void print_token(Token *token)
{
    char *text = malloc(token->length + 1);
    memcpy(text, token->start, token->length);
    text[token->length] = '\0';

    printf("Token { type: %s, lexeme: '%s', line: %d",
           token_type_to_string(token->type), text, token->line);

    // Print literal value based on token type
    switch (token->type)
    {
    case TOKEN_INT_LITERAL:
        printf(", value: %ld", token->literal.int_value);
        break;
    case TOKEN_LONG_LITERAL:
        printf(", value: %ldl", token->literal.int_value);
        break;
    case TOKEN_DOUBLE_LITERAL:
        printf(", value: %fd", token->literal.double_value);
        break;
    case TOKEN_CHAR_LITERAL:
        printf(", value: '%c'", token->literal.char_value);
        break;
    case TOKEN_STRING_LITERAL:
        printf(", value: \"%s\"", token->literal.string_value);
        break;
    case TOKEN_BOOL_LITERAL:
        printf(", value: %s", token->literal.bool_value ? "true" : "false");
        break;
    default:
        // No literal value to print
        break;
    }

    printf(" }\n");
    free(text);
}