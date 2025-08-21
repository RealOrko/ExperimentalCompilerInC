// token.c
#include "token.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void token_init(Token *token, TokenType type, const char *start, int length, int line, const char *filename)
{
    DEBUG_VERBOSE("Entering token_init: type=%s, start='%.*s', length=%d, line=%d, filename='%s'",
                  token_type_to_string(type), length, start, length, line, filename ? filename : "NULL");

    token->type = type;
    token->start = start;
    token->length = length;
    token->line = line;
    token->literal.int_value = 0;
    token->filename = filename;

    DEBUG_VERBOSE("Exiting token_init");
}

int token_is_type_keyword(TokenType type)
{
    DEBUG_VERBOSE("Entering token_is_type_keyword: type=%s", token_type_to_string(type));

    switch (type)
    {
    case TOKEN_INT:
    case TOKEN_LONG:
    case TOKEN_DOUBLE:
    case TOKEN_CHAR:
    case TOKEN_STR:
    case TOKEN_BOOL:
    case TOKEN_VOID:
        DEBUG_VERBOSE("Exiting token_is_type_keyword: returning 1");
        return 1;
    default:
        DEBUG_VERBOSE("Exiting token_is_type_keyword: returning 0");
        return 0;
    }
}

TokenType token_get_array_token_type(TokenType base_type)
{
    DEBUG_VERBOSE("Entering token_get_array_token_type: base_type=%s", token_type_to_string(base_type));

    TokenType result;
    switch (base_type)
    {
    case TOKEN_INT:
        result = TOKEN_INT_ARRAY;
        break;
    case TOKEN_LONG:
        result = TOKEN_LONG_ARRAY;
        break;
    case TOKEN_DOUBLE:
        result = TOKEN_DOUBLE_ARRAY;
        break;
    case TOKEN_CHAR:
        result = TOKEN_CHAR_ARRAY;
        break;
    case TOKEN_STR:
        result = TOKEN_STR_ARRAY;
        break;
    case TOKEN_BOOL:
        result = TOKEN_BOOL_ARRAY;
        break;
    case TOKEN_VOID:
        result = TOKEN_VOID_ARRAY;
        break;
    default:
        result = base_type;
        break;
    }

    DEBUG_VERBOSE("Exiting token_get_array_token_type: returning %s", token_type_to_string(result));
    return result;
}

void token_set_int_literal(Token *token, int64_t value)
{
    DEBUG_VERBOSE("Entering token_set_int_literal: value=%ld", value);

    token->literal.int_value = value;

    DEBUG_VERBOSE("Exiting token_set_int_literal");
}

void token_set_double_literal(Token *token, double value)
{
    DEBUG_VERBOSE("Entering token_set_double_literal: value=%f", value);

    token->literal.double_value = value;

    DEBUG_VERBOSE("Exiting token_set_double_literal");
}

void token_set_char_literal(Token *token, char value)
{
    DEBUG_VERBOSE("Entering token_set_char_literal: value='%c'", value);

    token->literal.char_value = value;

    DEBUG_VERBOSE("Exiting token_set_char_literal");
}

void token_set_string_literal(Token *token, const char *value)
{
    DEBUG_VERBOSE("Entering token_set_string_literal: value='%s'", value ? value : "NULL");

    token->literal.string_value = value;

    DEBUG_VERBOSE("Exiting token_set_string_literal");
}

const char *token_type_to_string(TokenType type)
{
    DEBUG_VERBOSE("Entering token_type_to_string: type=%d", type);

    const char *result;
    switch (type)
    {
    case TOKEN_EOF:
        result = "EOF";
        break;
    case TOKEN_INT_LITERAL:
        result = "INT_LITERAL";
        break;
    case TOKEN_LONG_LITERAL:
        result = "LONG_LITERAL";
        break;
    case TOKEN_DOUBLE_LITERAL:
        result = "DOUBLE_LITERAL";
        break;
    case TOKEN_CHAR_LITERAL:
        result = "CHAR_LITERAL";
        break;
    case TOKEN_STRING_LITERAL:
        result = "STRING_LITERAL";
        break;
    case TOKEN_INTERPOL_STRING:
        result = "INTERPOL_STRING";
        break;
    case TOKEN_TRUE:
        result = "TRUE";
        break;
    case TOKEN_FALSE:
        result = "FALSE";
        break;
    case TOKEN_IDENTIFIER:
        result = "IDENTIFIER";
        break;
    case TOKEN_FN:
        result = "FN";
        break;
    case TOKEN_VAR:
        result = "VAR";
        break;
    case TOKEN_RETURN:
        result = "RETURN";
        break;
    case TOKEN_IF:
        result = "IF";
        break;
    case TOKEN_ELSE:
        result = "ELSE";
        break;
    case TOKEN_FOR:
        result = "FOR";
        break;
    case TOKEN_WHILE:
        result = "WHILE";
        break;
    case TOKEN_IMPORT:
        result = "IMPORT";
        break;
    case TOKEN_NIL:
        result = "NIL";
        break;
    case TOKEN_INT:
        result = "INT";
        break;
    case TOKEN_LONG:
        result = "LONG";
        break;
    case TOKEN_DOUBLE:
        result = "DOUBLE";
        break;
    case TOKEN_CHAR:
        result = "CHAR";
        break;
    case TOKEN_STR:
        result = "STR";
        break;
    case TOKEN_BOOL:
        result = "BOOL";
        break;
    case TOKEN_VOID:
        result = "VOID";
        break;
    case TOKEN_INT_ARRAY:
        result = "INT_ARRAY";
        break;
    case TOKEN_LONG_ARRAY:
        result = "LONG_ARRAY";
        break;
    case TOKEN_DOUBLE_ARRAY:
        result = "DOUBLE_ARRAY";
        break;
    case TOKEN_CHAR_ARRAY:
        result = "CHAR_ARRAY";
        break;
    case TOKEN_STR_ARRAY:
        result = "STR_ARRAY";
        break;
    case TOKEN_BOOL_ARRAY:
        result = "BOOL_ARRAY";
        break;
    case TOKEN_VOID_ARRAY:
        result = "VOID_ARRAY";
        break;
    case TOKEN_PLUS:
        result = "PLUS";
        break;
    case TOKEN_MINUS:
        result = "MINUS";
        break;
    case TOKEN_STAR:
        result = "STAR";
        break;
    case TOKEN_SLASH:
        result = "SLASH";
        break;
    case TOKEN_MODULO:
        result = "MODULO";
        break;
    case TOKEN_EQUAL:
        result = "EQUAL";
        break;
    case TOKEN_EQUAL_EQUAL:
        result = "EQUAL_EQUAL";
        break;
    case TOKEN_BANG:
        result = "BANG";
        break;
    case TOKEN_BANG_EQUAL:
        result = "BANG_EQUAL";
        break;
    case TOKEN_LESS:
        result = "LESS";
        break;
    case TOKEN_LESS_EQUAL:
        result = "LESS_EQUAL";
        break;
    case TOKEN_GREATER:
        result = "GREATER";
        break;
    case TOKEN_GREATER_EQUAL:
        result = "GREATER_EQUAL";
        break;
    case TOKEN_AND:
        result = "AND";
        break;
    case TOKEN_OR:
        result = "OR";
        break;
    case TOKEN_PLUS_PLUS:
        result = "PLUS_PLUS";
        break;
    case TOKEN_MINUS_MINUS:
        result = "MINUS_MINUS";
        break;
    case TOKEN_LEFT_PAREN:
        result = "LEFT_PAREN";
        break;
    case TOKEN_RIGHT_PAREN:
        result = "RIGHT_PAREN";
        break;
    case TOKEN_LEFT_BRACE:
        result = "LEFT_BRACE";
        break;
    case TOKEN_RIGHT_BRACE:
        result = "RIGHT_BRACE";
        break;
    case TOKEN_LEFT_BRACKET:
        result = "LEFT_BRACKET";
        break;
    case TOKEN_RIGHT_BRACKET:
        result = "RIGHT_BRACKET";
        break;
    case TOKEN_SEMICOLON:
        result = "SEMICOLON";
        break;
    case TOKEN_COLON:
        result = "COLON";
        break;
    case TOKEN_COMMA:
        result = "COMMA";
        break;
    case TOKEN_DOT:
        result = "DOT";
        break;
    case TOKEN_ARROW:
        result = "ARROW";
        break;
    case TOKEN_INDENT:
        result = "INDENT";
        break;
    case TOKEN_DEDENT:
        result = "DEDENT";
        break;
    case TOKEN_NEWLINE:
        result = "NEWLINE";
        break;
    case TOKEN_ERROR:
        result = "ERROR";
        break;
    default:
        result = "INVALID";
        break;
    }

    DEBUG_VERBOSE("Exiting token_type_to_string: returning '%s'", result);
    return result;
}

void token_print(Token *token)
{
    DEBUG_VERBOSE("Entering token_print");

    char *text = malloc(token->length + 1);
    memcpy(text, token->start, token->length);
    text[token->length] = '\0';

    DEBUG_VERBOSE("Token { type: %s, lexeme: '%s', line: %d",
                  token_type_to_string(token->type), text, token->line);

    switch (token->type)
    {
    case TOKEN_INT_LITERAL:
        DEBUG_VERBOSE(", value: %ld", token->literal.int_value);
        break;
    case TOKEN_LONG_LITERAL:
        DEBUG_VERBOSE(", value: %ldl", token->literal.int_value);
        break;
    case TOKEN_DOUBLE_LITERAL:
        DEBUG_VERBOSE(", value: %fd", token->literal.double_value);
        break;
    case TOKEN_CHAR_LITERAL:
        DEBUG_VERBOSE(", value: '%c'", token->literal.char_value);
        break;
    case TOKEN_STRING_LITERAL:
    case TOKEN_INTERPOL_STRING:
        DEBUG_VERBOSE(", value: \"%s\"", token->literal.string_value);
        break;
    case TOKEN_TRUE:
        DEBUG_VERBOSE(", value: true");
        break;
    case TOKEN_FALSE:
        DEBUG_VERBOSE(", value: false");
        break;
    default:
        break;
    }

    DEBUG_VERBOSE(" }\n");
    free(text);

    DEBUG_VERBOSE("Exiting token_print");
}