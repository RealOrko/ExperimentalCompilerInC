// tests/token_tests.c

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <float.h>
#include <limits.h>
#include "../arena.h"
#include "../token.h"
#include "../debug.h"

static void setup_tokens(Arena *arena)
{
    arena_init(arena, 4096);
}

static void cleanup_tokens(Arena *arena)
{
    arena_free(arena);
}

static Token create_test_token(Arena *arena, TokenType type, const char *lexeme, int line, const char *filename)
{
    Token tok;
    const char *start = arena_strndup(arena, lexeme, strlen(lexeme));
    const char *file = arena_strdup(arena, filename);
    token_init(&tok, type, start, strlen(lexeme), line, file);
    return tok;
}

void test_token_init()
{
    DEBUG_INFO("\n*** Testing token_init...\n");
    Arena arena;
    setup_tokens(&arena);

    // Basic initialization
    Token tok_basic;
    const char *lexeme = "identifier";
    const char *file = "test.sn";
    token_init(&tok_basic, TOKEN_IDENTIFIER, lexeme, strlen(lexeme), 1, file);
    assert(tok_basic.type == TOKEN_IDENTIFIER);
    assert(strcmp(tok_basic.start, lexeme) == 0);
    assert(tok_basic.length == (int)strlen(lexeme));
    assert(tok_basic.line == 1);
    assert(strcmp(tok_basic.filename, file) == 0);
    assert(tok_basic.literal.int_value == 0); // Default zeroed

    // Different type and values
    Token tok_eof;
    token_init(&tok_eof, TOKEN_EOF, "", 0, 10, "eof.sn");
    assert(tok_eof.type == TOKEN_EOF);
    assert(strcmp(tok_eof.start, "") == 0);
    assert(tok_eof.length == 0);
    assert(tok_eof.line == 10);
    assert(strcmp(tok_eof.filename, "eof.sn") == 0);

    // Edge: length 0
    Token tok_empty;
    token_init(&tok_empty, TOKEN_STRING_LITERAL, "", 0, 5, "empty.sn");
    assert(tok_empty.length == 0);
    assert(strcmp(tok_empty.start, "") == 0);

    // Edge: negative line (allowed, but unusual)
    Token tok_neg_line;
    token_init(&tok_neg_line, TOKEN_PLUS, "+", 1, -1, "neg.sn");
    assert(tok_neg_line.line == -1);

    // NULL pointers (function assumes valid, but test behavior)
    Token tok_null_start;
    token_init(&tok_null_start, TOKEN_IDENTIFIER, NULL, 0, 1, "null.sn");
    assert(tok_null_start.start == NULL);
    assert(tok_null_start.length == 0);

    Token tok_null_file;
    token_init(&tok_null_file, TOKEN_IDENTIFIER, "id", 2, 1, NULL);
    assert(tok_null_file.filename == NULL);

    // Long lexeme
    char long_lexeme[1024];
    memset(long_lexeme, 'a', 1023);
    long_lexeme[1023] = '\0';
    Token tok_long;
    token_init(&tok_long, TOKEN_STRING_LITERAL, long_lexeme, 1023, 1, "long.sn");
    assert(strcmp(tok_long.start, long_lexeme) == 0);
    assert(tok_long.length == 1023);

    cleanup_tokens(&arena);
}

void test_token_is_type_keyword()
{
    DEBUG_INFO("\n*** Testing token_is_type_keyword...\n");

    // Positive cases
    assert(token_is_type_keyword(TOKEN_INT) == 1);
    assert(token_is_type_keyword(TOKEN_LONG) == 1);
    assert(token_is_type_keyword(TOKEN_DOUBLE) == 1);
    assert(token_is_type_keyword(TOKEN_CHAR) == 1);
    assert(token_is_type_keyword(TOKEN_STR) == 1);
    assert(token_is_type_keyword(TOKEN_BOOL) == 1);
    assert(token_is_type_keyword(TOKEN_VOID) == 1);

    // Negative cases
    assert(token_is_type_keyword(TOKEN_EOF) == 0);
    assert(token_is_type_keyword(TOKEN_IDENTIFIER) == 0);
    assert(token_is_type_keyword(TOKEN_PLUS) == 0);
    assert(token_is_type_keyword(TOKEN_STRING_LITERAL) == 0);
    assert(token_is_type_keyword(TOKEN_ERROR) == 0);
    assert(token_is_type_keyword(TOKEN_INT_ARRAY) == 0); // Array types not keywords

    // Edge: invalid types
    assert(token_is_type_keyword(-1) == 0);
    assert(token_is_type_keyword(1000) == 0); // Beyond enum
}

void test_token_get_array_token_type()
{
    DEBUG_INFO("\n*** Testing token_get_array_token_type...\n");

    // Mapping cases
    assert(token_get_array_token_type(TOKEN_INT) == TOKEN_INT_ARRAY);
    assert(token_get_array_token_type(TOKEN_LONG) == TOKEN_LONG_ARRAY);
    assert(token_get_array_token_type(TOKEN_DOUBLE) == TOKEN_DOUBLE_ARRAY);
    assert(token_get_array_token_type(TOKEN_CHAR) == TOKEN_CHAR_ARRAY);
    assert(token_get_array_token_type(TOKEN_STR) == TOKEN_STR_ARRAY);
    assert(token_get_array_token_type(TOKEN_BOOL) == TOKEN_BOOL_ARRAY);
    assert(token_get_array_token_type(TOKEN_VOID) == TOKEN_VOID_ARRAY);

    // No mapping, return base
    assert(token_get_array_token_type(TOKEN_IDENTIFIER) == TOKEN_IDENTIFIER);
    assert(token_get_array_token_type(TOKEN_EOF) == TOKEN_EOF);
    assert(token_get_array_token_type(TOKEN_PLUS) == TOKEN_PLUS);
    assert(token_get_array_token_type(TOKEN_ERROR) == TOKEN_ERROR);

    // Already array types
    assert(token_get_array_token_type(TOKEN_INT_ARRAY) == TOKEN_INT_ARRAY);

    // Invalid
    assert(token_get_array_token_type(-1) == -1);
    assert(token_get_array_token_type(1000) == 1000);
}

void test_token_set_int_literal()
{
    DEBUG_INFO("\n*** Testing token_set_int_literal...\n");
    Arena arena;
    setup_tokens(&arena);

    Token tok = create_test_token(&arena, TOKEN_INT_LITERAL, "123", 1, "test.sn");
    token_set_int_literal(&tok, 456);
    assert(tok.literal.int_value == 456);

    // Edge: min/max
    token_set_int_literal(&tok, INT64_MIN);
    assert(tok.literal.int_value == INT64_MIN);
    token_set_int_literal(&tok, INT64_MAX);
    assert(tok.literal.int_value == INT64_MAX);

    // Zero
    token_set_int_literal(&tok, 0);
    assert(tok.literal.int_value == 0);

    // Negative
    token_set_int_literal(&tok, -123);
    assert(tok.literal.int_value == -123);

    cleanup_tokens(&arena);
}

void test_token_set_double_literal()
{
    DEBUG_INFO("\n*** Testing token_set_double_literal...\n");
    Arena arena;
    setup_tokens(&arena);

    Token tok = create_test_token(&arena, TOKEN_DOUBLE_LITERAL, "1.23", 1, "test.sn");
    token_set_double_literal(&tok, 4.56);
    assert(tok.literal.double_value == 4.56);

    // Edge: min/max
    token_set_double_literal(&tok, DBL_MIN);
    assert(tok.literal.double_value == DBL_MIN);
    token_set_double_literal(&tok, DBL_MAX);
    assert(tok.literal.double_value == DBL_MAX);

    // Zero
    token_set_double_literal(&tok, 0.0);
    assert(tok.literal.double_value == 0.0);

    // Negative
    token_set_double_literal(&tok, -1.23);
    assert(tok.literal.double_value == -1.23);

    // Infinity/NaN (if supported)
    // token_set_double_literal(&tok, INFINITY);
    // assert(tok.literal.double_value == INFINITY);
    // token_set_double_literal(&tok, NAN);
    // assert(isnan(tok.literal.double_value));

    cleanup_tokens(&arena);
}

void test_token_set_char_literal()
{
    DEBUG_INFO("\n*** Testing token_set_char_literal...\n");
    Arena arena;
    setup_tokens(&arena);

    Token tok = create_test_token(&arena, TOKEN_CHAR_LITERAL, "'a'", 1, "test.sn");
    token_set_char_literal(&tok, 'b');
    assert(tok.literal.char_value == 'b');

    // Edge: null char
    token_set_char_literal(&tok, '\0');
    assert(tok.literal.char_value == '\0');

    // Special chars
    token_set_char_literal(&tok, '\n');
    assert(tok.literal.char_value == '\n');
    token_set_char_literal(&tok, '\t');
    assert(tok.literal.char_value == '\t');

    // ASCII extremes
    token_set_char_literal(&tok, 0);
    assert(tok.literal.char_value == 0);
    token_set_char_literal(&tok, 127);
    assert(tok.literal.char_value == 127);

    cleanup_tokens(&arena);
}

void test_token_set_string_literal()
{
    DEBUG_INFO("\n*** Testing token_set_string_literal...\n");
    Arena arena;
    setup_tokens(&arena);

    Token tok = create_test_token(&arena, TOKEN_STRING_LITERAL, "\"hello\"", 1, "test.sn");
    const char *str = "world";
    token_set_string_literal(&tok, str);
    assert(strcmp(tok.literal.string_value, str) == 0);

    // Empty string
    token_set_string_literal(&tok, "");
    assert(strcmp(tok.literal.string_value, "") == 0);

    // NULL (function sets it, but assume valid)
    token_set_string_literal(&tok, NULL);
    assert(tok.literal.string_value == NULL);

    // Long string
    char long_str[1024];
    memset(long_str, 'x', 1023);
    long_str[1023] = '\0';
    token_set_string_literal(&tok, long_str);
    assert(strcmp(tok.literal.string_value, long_str) == 0);

    cleanup_tokens(&arena);
}

void test_token_set_bool_literal()
{
    DEBUG_INFO("\n*** Testing token_set_bool_literal...\n");
    Arena arena;
    setup_tokens(&arena);

    Token tok = create_test_token(&arena, TOKEN_BOOL_LITERAL, "true", 1, "test.sn");
    token_set_bool_literal(&tok, 1);
    assert(tok.literal.bool_value == 1);

    token_set_bool_literal(&tok, 0);
    assert(tok.literal.bool_value == 0);

    // Non-zero as true
    token_set_bool_literal(&tok, 42);
    assert(tok.literal.bool_value == 42); // But typically 1/0, function sets int

    token_set_bool_literal(&tok, -1);
    assert(tok.literal.bool_value == -1);

    cleanup_tokens(&arena);
}

void test_token_type_to_string()
{
    DEBUG_INFO("\n*** Testing token_type_to_string...\n");

    // Sample some
    assert(strcmp(token_type_to_string(TOKEN_EOF), "EOF") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INT_LITERAL), "INT_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_PLUS), "PLUS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ERROR), "ERROR") == 0);

    // All enums exhaustively
    assert(strcmp(token_type_to_string(TOKEN_EOF), "EOF") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INDENT), "INDENT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DEDENT), "DEDENT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_NEWLINE), "NEWLINE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INT_LITERAL), "INT_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LONG_LITERAL), "LONG_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DOUBLE_LITERAL), "DOUBLE_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_CHAR_LITERAL), "CHAR_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_STRING_LITERAL), "STRING_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INTERPOL_STRING), "INTERPOL_STRING") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BOOL_LITERAL), "BOOL_LITERAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_IDENTIFIER), "IDENTIFIER") == 0);
    assert(strcmp(token_type_to_string(TOKEN_FN), "FN") == 0);
    assert(strcmp(token_type_to_string(TOKEN_VAR), "VAR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_RETURN), "RETURN") == 0);
    assert(strcmp(token_type_to_string(TOKEN_IF), "IF") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ELSE), "ELSE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_FOR), "FOR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_WHILE), "WHILE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_IMPORT), "IMPORT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_NIL), "NIL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INT), "INT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LONG), "LONG") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DOUBLE), "DOUBLE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_CHAR), "CHAR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_STR), "STR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BOOL), "BOOL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_VOID), "VOID") == 0);
    assert(strcmp(token_type_to_string(TOKEN_INT_ARRAY), "INT_ARRAY") == 0); 
    assert(strcmp(token_type_to_string(TOKEN_LONG_ARRAY), "LONG_ARRAY") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DOUBLE_ARRAY), "DOUBLE_ARRAY") == 0);
    assert(strcmp(token_type_to_string(TOKEN_CHAR_ARRAY), "CHAR_ARRAY") == 0);
    assert(strcmp(token_type_to_string(TOKEN_STR_ARRAY), "STR_ARRAY") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BOOL_ARRAY), "BOOL_ARRAY") == 0);
    assert(strcmp(token_type_to_string(TOKEN_VOID_ARRAY), "VOID_ARRAY") == 0);
    assert(strcmp(token_type_to_string(TOKEN_PLUS), "PLUS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_MINUS), "MINUS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_STAR), "STAR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_SLASH), "SLASH") == 0);
    assert(strcmp(token_type_to_string(TOKEN_MODULO), "MODULO") == 0);
    assert(strcmp(token_type_to_string(TOKEN_EQUAL), "EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_EQUAL_EQUAL), "EQUAL_EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BANG), "BANG") == 0);
    assert(strcmp(token_type_to_string(TOKEN_BANG_EQUAL), "BANG_EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LESS), "LESS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LESS_EQUAL), "LESS_EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_GREATER), "GREATER") == 0);
    assert(strcmp(token_type_to_string(TOKEN_GREATER_EQUAL), "GREATER_EQUAL") == 0);
    assert(strcmp(token_type_to_string(TOKEN_AND), "AND") == 0);
    assert(strcmp(token_type_to_string(TOKEN_OR), "OR") == 0);
    assert(strcmp(token_type_to_string(TOKEN_PLUS_PLUS), "PLUS_PLUS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_MINUS_MINUS), "MINUS_MINUS") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LEFT_PAREN), "LEFT_PAREN") == 0);
    assert(strcmp(token_type_to_string(TOKEN_RIGHT_PAREN), "RIGHT_PAREN") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LEFT_BRACE), "LEFT_BRACE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_RIGHT_BRACE), "RIGHT_BRACE") == 0);
    assert(strcmp(token_type_to_string(TOKEN_LEFT_BRACKET), "LEFT_BRACKET") == 0);
    assert(strcmp(token_type_to_string(TOKEN_RIGHT_BRACKET), "RIGHT_BRACKET") == 0);
    assert(strcmp(token_type_to_string(TOKEN_SEMICOLON), "SEMICOLON") == 0);
    assert(strcmp(token_type_to_string(TOKEN_COLON), "COLON") == 0);
    assert(strcmp(token_type_to_string(TOKEN_COMMA), "COMMA") == 0);
    assert(strcmp(token_type_to_string(TOKEN_DOT), "DOT") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ARROW), "ARROW") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ERROR), "ERROR") == 0);

    // Invalid: <0 or >= ERROR (but ERROR is last)
    assert(strcmp(token_type_to_string(-1), "INVALID") == 0);
    assert(strcmp(token_type_to_string(TOKEN_ERROR + 1), "INVALID") == 0);
    assert(strcmp(token_type_to_string(1000), "INVALID") == 0);

    // Unknown in switch
    // Assuming all defined, but for array types as above
}

void test_token_print()
{
    DEBUG_INFO("\n*** Testing token_print (no crash, manual output check)...\n");
    Arena arena;
    setup_tokens(&arena);

    // Non-literal
    Token tok_id = create_test_token(&arena, TOKEN_IDENTIFIER, "var", 1, "test.sn");
    token_print(&tok_id);

    // Int literal
    Token tok_int = create_test_token(&arena, TOKEN_INT_LITERAL, "123", 2, "test.sn");
    token_set_int_literal(&tok_int, 123);
    token_print(&tok_int);

    // Long literal
    Token tok_long = create_test_token(&arena, TOKEN_LONG_LITERAL, "123l", 3, "test.sn");
    token_set_int_literal(&tok_long, 123); // Uses int_value
    token_print(&tok_long);

    // Double
    Token tok_double = create_test_token(&arena, TOKEN_DOUBLE_LITERAL, "1.23", 4, "test.sn");
    token_set_double_literal(&tok_double, 1.23);
    token_print(&tok_double);

    // Char
    Token tok_char = create_test_token(&arena, TOKEN_CHAR_LITERAL, "'a'", 5, "test.sn");
    token_set_char_literal(&tok_char, 'a');
    token_print(&tok_char);

    // String
    Token tok_str = create_test_token(&arena, TOKEN_STRING_LITERAL, "\"hello\"", 6, "test.sn");
    token_set_string_literal(&tok_str, "hello");
    token_print(&tok_str);

    // Interpol string (same as string)
    Token tok_interpol = create_test_token(&arena, TOKEN_INTERPOL_STRING, "$\"hello\"", 7, "test.sn");
    token_set_string_literal(&tok_interpol, "hello");
    token_print(&tok_interpol);

    // Bool true
    Token tok_bool_t = create_test_token(&arena, TOKEN_BOOL_LITERAL, "true", 8, "test.sn");
    token_set_bool_literal(&tok_bool_t, 1);
    token_print(&tok_bool_t);

    // Bool false
    Token tok_bool_f = create_test_token(&arena, TOKEN_BOOL_LITERAL, "false", 9, "test.sn");
    token_set_bool_literal(&tok_bool_f, 0);
    token_print(&tok_bool_f);

    // No value printed for non-literals
    Token tok_plus = create_test_token(&arena, TOKEN_PLUS, "+", 10, "test.sn");
    token_print(&tok_plus);

    // Edge: empty lexeme
    Token tok_empty = create_test_token(&arena, TOKEN_EOF, "", 11, "test.sn");
    token_print(&tok_empty);

    // NULL token? Function assumes valid, but call with NULL crashes, so skip

    // Long lexeme
    char long_lex[1024];
    memset(long_lex, 'a', 1023);
    long_lex[1023] = '\0';
    Token tok_long_lex = create_test_token(&arena, TOKEN_IDENTIFIER, long_lex, 12, "test.sn");
    token_print(&tok_long_lex);

    cleanup_tokens(&arena);
}