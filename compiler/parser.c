// parser.c
#include "parser.h"
#include "debug.h"
#include "file.h"
#include <stdio.h>
#include <string.h>

int parser_is_at_end(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_is_at_end");
    int result = parser->current.type == TOKEN_EOF;
    DEBUG_VERBOSE("Exiting parser_is_at_end: result=%d", result);
    return result;
}

void skip_newlines(Parser *parser)
{
    DEBUG_VERBOSE("Entering skip_newlines");
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        if (parser_check(parser, TOKEN_INDENT) || parser_check(parser, TOKEN_DEDENT))
        {
            DEBUG_VERBOSE("Breaking skip_newlines due to INDENT or DEDENT");
            break;
        }
    }
    DEBUG_VERBOSE("Exiting skip_newlines");
}

int is_at_function_boundary(Parser *parser)
{
    DEBUG_VERBOSE("Entering is_at_function_boundary");
    if (parser_check(parser, TOKEN_DEDENT))
    {
        DEBUG_VERBOSE("Found DEDENT, returning 1");
        return 1;
    }
    if (parser_check(parser, TOKEN_FN))
    {
        DEBUG_VERBOSE("Found FN, returning 1");
        return 1;
    }
    if (parser_check(parser, TOKEN_EOF))
    {
        DEBUG_VERBOSE("Found EOF, returning 1");
        return 1;
    }
    DEBUG_VERBOSE("No boundary found, returning 0");
    return 0;
}

Stmt *parser_indented_block(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_indented_block");
    if (!parser_check(parser, TOKEN_INDENT))
    {
        parser_error(parser, "Expected indented block");
        DEBUG_VERBOSE("Error: Expected indented block");
        return NULL;
    }
    parser_advance(parser);
    DEBUG_VERBOSE("Advanced past INDENT token");

    int current_indent = parser->lexer->indent_stack[parser->lexer->indent_size - 1];
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    while (!parser_is_at_end(parser) &&
           parser->lexer->indent_stack[parser->lexer->indent_size - 1] >= current_indent)
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            DEBUG_VERBOSE("Skipped NEWLINE token");
        }

        if (parser_check(parser, TOKEN_DEDENT))
        {
            DEBUG_VERBOSE("Found DEDENT, breaking loop");
            break;
        }

        if (parser_check(parser, TOKEN_EOF))
        {
            DEBUG_VERBOSE("Found EOF, breaking loop");
            break;
        }

        Stmt *stmt = parser_declaration(parser);
        if (stmt == NULL)
        {
            DEBUG_VERBOSE("parser_declaration returned NULL, continuing");
            continue;
        }

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Stmt **new_statements = arena_alloc(parser->arena, sizeof(Stmt *) * capacity);
            if (new_statements == NULL)
            {
                DEBUG_VERBOSE("Failed to allocate memory for statements, exiting");
                exit(1);
            }
            if (statements != NULL && count > 0)
            {
                memcpy(new_statements, statements, sizeof(Stmt *) * count);
            }
            statements = new_statements;
            DEBUG_VERBOSE("Reallocated statements array, new capacity=%d", capacity);
        }
        statements[count++] = stmt;
        DEBUG_VERBOSE("Added statement to block, count=%d", count);
    }

    if (parser_check(parser, TOKEN_DEDENT))
    {
        parser_advance(parser);
        DEBUG_VERBOSE("Advanced past DEDENT token");
    }
    else if (parser->lexer->indent_stack[parser->lexer->indent_size - 1] < current_indent)
    {
        parser_error(parser, "Expected dedent to end block");
        DEBUG_VERBOSE("Error: Expected dedent to end block");
    }

    Stmt *result = ast_create_block_stmt(parser->arena, statements, count, NULL);
    DEBUG_VERBOSE("Exiting parser_indented_block: created block with %d statements", count);
    return result;
}

Expr *parser_multi_line_expression(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_multi_line_expression");
    Expr *expr = parser_expression(parser);
    DEBUG_VERBOSE("Parsed initial expression");

    while (parser_match(parser, TOKEN_NEWLINE))
    {
        Token op_token = parser->previous;
        DEBUG_VERBOSE("Found NEWLINE, parsing right expression");
        Expr *right = parser_expression(parser);
        expr = ast_create_binary_expr(parser->arena, expr, TOKEN_PLUS, right, &op_token);
        DEBUG_VERBOSE("Created binary expression with PLUS operator");
    }

    DEBUG_VERBOSE("Exiting parser_multi_line_expression");
    return expr;
}

int skip_newlines_and_check_end(Parser *parser)
{
    DEBUG_VERBOSE("Entering skip_newlines_and_check_end");
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        DEBUG_VERBOSE("Skipped NEWLINE token");
    }
    int result = parser_is_at_end(parser);
    DEBUG_VERBOSE("Exiting skip_newlines_and_check_end: result=%d", result);
    return result;
}

void parser_init(Arena *arena, Parser *parser, Lexer *lexer, SymbolTable *symbol_table)
{
    DEBUG_VERBOSE("Entering parser_init");
    parser->arena = arena;
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    parser->symbol_table = symbol_table;

    // Add built-in functions to the global symbol table
    Token print_token;
    print_token.start = arena_strdup(arena, "print");
    print_token.length = 5;
    print_token.type = TOKEN_IDENTIFIER;
    print_token.line = 0;
    print_token.filename = arena_strdup(arena, "<built-in>");
    DEBUG_VERBOSE("Created print_token for built-in print function");

    Type *any_type = ast_create_primitive_type(arena, TYPE_ANY);
    Type **builtin_params = arena_alloc(arena, sizeof(Type *));
    builtin_params[0] = any_type;

    Type *print_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, print_token, print_type, SYMBOL_GLOBAL);
    DEBUG_VERBOSE("Added print function to symbol table");

    Token to_string_token;
    to_string_token.start = arena_strdup(arena, "to_string");
    to_string_token.length = 9;
    to_string_token.type = TOKEN_IDENTIFIER;
    to_string_token.line = 0;
    to_string_token.filename = arena_strdup(arena, "<built-in>");
    DEBUG_VERBOSE("Created to_string_token for built-in to_string function");

    Type *to_string_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, to_string_token, to_string_type, SYMBOL_GLOBAL);
    DEBUG_VERBOSE("Added to_string function to symbol table");

    parser->previous.type = TOKEN_ERROR;
    parser->previous.start = NULL;
    parser->previous.length = 0;
    parser->previous.line = 0;
    parser->current = parser->previous;

    parser_advance(parser);
    DEBUG_VERBOSE("Advanced parser to first token");
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    DEBUG_VERBOSE("Exiting parser_init");
}

void parser_cleanup(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_cleanup");
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    parser->previous.start = NULL;
    parser->current.start = NULL;
    DEBUG_VERBOSE("Exiting parser_cleanup");
}

void parser_error(Parser *parser, const char *message)
{
    DEBUG_VERBOSE("Entering parser_error: message=%s", message);
    parser_error_at(parser, &parser->previous, message);
    DEBUG_VERBOSE("Exiting parser_error");
}

void parser_error_at_current(Parser *parser, const char *message)
{
    DEBUG_VERBOSE("Entering parser_error_at_current: message=%s", message);
    parser_error_at(parser, &parser->current, message);
    DEBUG_VERBOSE("Exiting parser_error_at_current");
}

void parser_error_at(Parser *parser, Token *token, const char *message)
{
    DEBUG_VERBOSE("Entering parser_error_at: message=%s, token_type=%d", message, token->type);
    if (parser->panic_mode)
    {
        DEBUG_VERBOSE("In panic mode, returning");
        return;
    }

    parser->panic_mode = 1;
    parser->had_error = 1;

    fprintf(stderr, "[%s:%d] Error", parser->lexer->filename, token->line);
    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
        DEBUG_VERBOSE("Error at EOF");
    }
    else if (token->type == TOKEN_ERROR)
    {
        DEBUG_VERBOSE("Error token type");
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
        DEBUG_VERBOSE("Error at token: %.*s", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);

    parser->lexer->indent_size = 1;
    DEBUG_VERBOSE("Reset indent_size to 1");

    if (token == &parser->current)
    {
        parser_advance(parser);
        DEBUG_VERBOSE("Advanced parser past error token");
    }
    DEBUG_VERBOSE("Exiting parser_error_at");
}

void parser_advance(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_advance: current_type=%d", parser->current.type);
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = lexer_scan_token(parser->lexer);
        DEBUG_VERBOSE("Scanned token: type=%d", parser->current.type);
        if (parser->current.type != TOKEN_ERROR)
            break;
        parser_error_at_current(parser, parser->current.start);
    }
    DEBUG_VERBOSE("Exiting parser_advance: new_current_type=%d", parser->current.type);
}

void parser_consume(Parser *parser, TokenType type, const char *message)
{
    DEBUG_VERBOSE("Entering parser_consume: expected_type=%d, current_type=%d", type, parser->current.type);
    if (parser->current.type == type)
    {
        parser_advance(parser);
        DEBUG_VERBOSE("Consumed expected token");
        return;
    }
    parser_error_at_current(parser, message);
    DEBUG_VERBOSE("Error: expected token type %d", type);
}

int parser_check(Parser *parser, TokenType type)
{
    DEBUG_VERBOSE("parser_check: checking for type=%d, current_type=%d", type, parser->current.type);
    return parser->current.type == type;
}

int parser_match(Parser *parser, TokenType type)
{
    DEBUG_VERBOSE("Entering parser_match: expected_type=%d, current_type=%d", type, parser->current.type);
    if (!parser_check(parser, type))
    {
        DEBUG_VERBOSE("No match, returning 0");
        return 0;
    }
    parser_advance(parser);
    DEBUG_VERBOSE("Matched and advanced, returning 1");
    return 1;
}

static void synchronize(Parser *parser)
{
    DEBUG_VERBOSE("Entering synchronize");
    parser->panic_mode = 0;
    DEBUG_VERBOSE("Reset panic_mode");

    while (!parser_is_at_end(parser))
    {
        if (parser->previous.type == TOKEN_SEMICOLON || parser->previous.type == TOKEN_NEWLINE)
        {
            DEBUG_VERBOSE("Found SEMICOLON or NEWLINE, exiting synchronize");
            return;
        }

        switch (parser->current.type)
        {
        case TOKEN_FN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_RETURN:
        case TOKEN_IMPORT:
        case TOKEN_ELSE:
            DEBUG_VERBOSE("Found synchronization token: type=%d", parser->current.type);
            return;
        case TOKEN_NEWLINE:
            parser_advance(parser);
            DEBUG_VERBOSE("Advanced past NEWLINE in synchronize");
            break;
        default:
            parser_advance(parser);
            DEBUG_VERBOSE("Advanced past token in synchronize: type=%d", parser->current.type);
            break;
        }
    }
    DEBUG_VERBOSE("Exiting synchronize: reached end");
}

Type *parser_type(Parser *parser) {
    DEBUG_VERBOSE("Entering parser_type");
    Type *type = NULL;
    TokenType tt = parser->current.type;
    TypeKind kind;
    switch (tt) {
    case TOKEN_INT: kind = TYPE_INT; break;
    case TOKEN_LONG: kind = TYPE_LONG; break;
    case TOKEN_DOUBLE: kind = TYPE_DOUBLE; break;
    case TOKEN_CHAR: kind = TYPE_CHAR; break;
    case TOKEN_STR: kind = TYPE_STRING; break;
    case TOKEN_BOOL: kind = TYPE_BOOL; break;
    case TOKEN_VOID: kind = TYPE_VOID; break;
    case TOKEN_NIL: kind = TYPE_NIL; break;
    default:
        parser_error_at_current(parser, "Expected type");
        DEBUG_VERBOSE("Error: Expected type, got token type %d", tt);
        return NULL;
    }
    parser_advance(parser);
    type = ast_create_primitive_type(parser->arena, kind);
    // Handle array types by wrapping the base type in array types for each [] pair
    while (parser_match(parser, TOKEN_LEFT_BRACKET)) {
        parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after '[' in array type");
        type = ast_create_array_type(parser->arena, type);
    }
    DEBUG_VERBOSE("Exiting parser_type: parsed type %s", ast_type_to_string(parser->arena, type));
    return type;
}

Expr *parser_expression(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_expression");
    Expr *result = parser_assignment(parser);
    if (result == NULL)
    {
        parser_error_at_current(parser, "Expected expression");
        parser_advance(parser);
        DEBUG_VERBOSE("Error: Expected expression, advancing");
    }
    DEBUG_VERBOSE("Exiting parser_expression");
    return result;
}

Expr *parser_assignment(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_assignment");
    Expr *expr = parser_logical_or(parser);
    DEBUG_VERBOSE("Parsed logical_or expression");

    if (parser_match(parser, TOKEN_EQUAL))
    {
        Token equals = parser->previous;
        DEBUG_VERBOSE("Found EQUAL, parsing assignment value");
        Expr *value = parser_assignment(parser);
        if (expr->type == EXPR_VARIABLE)
        {
            Token name = expr->as.variable.name;
            char *new_start = arena_strndup(parser->arena, name.start, name.length);
            if (new_start == NULL)
            {
                DEBUG_VERBOSE("Error: Out of memory in assignment");
                exit(1);
            }
            name.start = new_start;
            Expr *result = ast_create_assign_expr(parser->arena, name, value, &equals);
            DEBUG_VERBOSE("Created assignment expression");
            return result;
        }
        parser_error(parser, "Invalid assignment target");
        DEBUG_VERBOSE("Error: Invalid assignment target");
    }
    DEBUG_VERBOSE("Exiting parser_assignment");
    return expr;
}

Expr *parser_logical_or(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_logical_or");
    Expr *expr = parser_logical_and(parser);
    while (parser_match(parser, TOKEN_OR))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_logical_and(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
        DEBUG_VERBOSE("Created binary OR expression");
    }
    DEBUG_VERBOSE("Exiting parser_logical_or");
    return expr;
}

Expr *parser_logical_and(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_logical_and");
    Expr *expr = parser_equality(parser);
    while (parser_match(parser, TOKEN_AND))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_equality(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
        DEBUG_VERBOSE("Created binary AND expression");
    }
    DEBUG_VERBOSE("Exiting parser_logical_and");
    return expr;
}

Expr *parser_equality(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_equality");
    Expr *expr = parser_comparison(parser);
    while (parser_match(parser, TOKEN_BANG_EQUAL) || parser_match(parser, TOKEN_EQUAL_EQUAL))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_comparison(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
        DEBUG_VERBOSE("Created binary equality expression: op=%d", operator);
    }
    DEBUG_VERBOSE("Exiting parser_equality");
    return expr;
}

Expr *parser_comparison(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_comparison");
    Expr *expr = parser_term(parser);
    while (parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_LESS_EQUAL) ||
           parser_match(parser, TOKEN_GREATER) || parser_match(parser, TOKEN_GREATER_EQUAL))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_term(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
        DEBUG_VERBOSE("Created binary comparison expression: op=%d", operator);
    }
    DEBUG_VERBOSE("Exiting parser_comparison");
    return expr;
}

Expr *parser_term(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_term");
    Expr *expr = parser_factor(parser);
    while (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_factor(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
        DEBUG_VERBOSE("Created binary term expression: op=%d", operator);
    }
    DEBUG_VERBOSE("Exiting parser_term");
    return expr;
}

Expr *parser_factor(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_factor");
    Expr *expr = parser_unary(parser);
    while (parser_match(parser, TOKEN_STAR) || parser_match(parser, TOKEN_SLASH) || parser_match(parser, TOKEN_MODULO))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_unary(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
        DEBUG_VERBOSE("Created binary factor expression: op=%d", operator);
    }
    DEBUG_VERBOSE("Exiting parser_factor");
    return expr;
}

Expr *parser_unary(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_unary");
    if (parser_match(parser, TOKEN_BANG) || parser_match(parser, TOKEN_MINUS))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_unary(parser);
        Expr *result = ast_create_unary_expr(parser->arena, operator, right, &op);
        DEBUG_VERBOSE("Created unary expression: op=%d", operator);
        return result;
    }
    Expr *result = parser_postfix(parser);
    DEBUG_VERBOSE("Exiting parser_unary");
    return result;
}

Expr *parser_postfix(Parser *parser) {
    DEBUG_VERBOSE("Entering parser_postfix");
    Expr *expr = parser_primary(parser);

    while (true) {
        if (parser_match(parser, TOKEN_LEFT_PAREN)) {
            expr = parser_call(parser, expr);
            DEBUG_VERBOSE("Parsed call expression");
        } else if (parser_match(parser, TOKEN_LEFT_BRACKET)) {
            Expr *index = parser_expression(parser);
            parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index.");
            expr = ast_create_array_access_expr(parser->arena, expr, index, &parser->previous);
            DEBUG_VERBOSE("Parsed array access expression");
        } else if (parser_match(parser, TOKEN_DOT)) {
            parser_consume(parser, TOKEN_IDENTIFIER, "Expected member name after '.'.");
            Token name = parser->previous;
            expr = ast_create_member_expr(parser->arena, expr, name, &parser->previous);
            DEBUG_VERBOSE("Parsed member access expression");
        } else if (parser_match(parser, TOKEN_PLUS_PLUS)) {
            expr = ast_create_increment_expr(parser->arena, expr, &parser->previous);
            DEBUG_VERBOSE("Parsed increment expression");
        } else if (parser_match(parser, TOKEN_MINUS_MINUS)) {
            expr = ast_create_decrement_expr(parser->arena, expr, &parser->previous);
            DEBUG_VERBOSE("Parsed decrement expression");
        } else {
            break;
        }
    }

    DEBUG_VERBOSE("Exiting parser_postfix");
    return expr;
}

Expr *parser_primary(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_primary");
    if (parser_match(parser, TOKEN_INT_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        Expr *result = ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_INT), false, &parser->previous);
        DEBUG_VERBOSE("Created INT literal expression");
        return result;
    }
    if (parser_match(parser, TOKEN_LONG_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        Expr *result = ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_LONG), false, &parser->previous);
        DEBUG_VERBOSE("Created LONG literal expression");
        return result;
    }
    if (parser_match(parser, TOKEN_DOUBLE_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        Expr *result = ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_DOUBLE), false, &parser->previous);
        DEBUG_VERBOSE("Created DOUBLE literal expression");
        return result;
    }
    if (parser_match(parser, TOKEN_CHAR_LITERAL))
    {
        LiteralValue value;
        value.char_value = parser->previous.literal.char_value;
        Expr *result = ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_CHAR), false, &parser->previous);
        DEBUG_VERBOSE("Created CHAR literal expression");
        return result;
    }
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        LiteralValue value;
        value.string_value = arena_strdup(parser->arena, parser->previous.literal.string_value);
        Expr *result = ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_STRING), false, &parser->previous);
        DEBUG_VERBOSE("Created STRING literal expression");
        return result;
    }
    if (parser_match(parser, TOKEN_BOOL_LITERAL))
    {
        LiteralValue value;
        value.bool_value = parser->previous.literal.bool_value;
        Expr *result = ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_BOOL), false, &parser->previous);
        DEBUG_VERBOSE("Created BOOL literal expression");
        return result;
    }
    if (parser_match(parser, TOKEN_NIL))
    {
        LiteralValue value;
        value.int_value = 0;
        Expr *result = ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_NIL), false, &parser->previous);
        DEBUG_VERBOSE("Created NIL literal expression");
        return result;
    }
    if (parser_match(parser, TOKEN_IDENTIFIER))
    {
        Token var_token = parser->previous;
        var_token.start = arena_strndup(parser->arena, parser->previous.start, parser->previous.length);
        Expr *result = ast_create_variable_expr(parser->arena, var_token, &parser->previous);
        DEBUG_VERBOSE("Created variable expression: %.*s", var_token.length, var_token.start);
        return result;
    }
    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        DEBUG_VERBOSE("Found LEFT_PAREN, parsing grouped expression");
        Expr *expr = parser_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        DEBUG_VERBOSE("Consumed RIGHT_PAREN");
        return expr;
    }

    if (parser_match(parser, TOKEN_INTERPOL_STRING))
    {
        DEBUG_VERBOSE("Entering interpolated string parsing");
        Token interpol_token = parser->previous;
        const char *content = parser->previous.literal.string_value;
        Expr **parts = NULL;
        int capacity = 0;
        int count = 0;

        const char *p = content;
        const char *segment_start = p;

        while (*p)
        {
            if (*p == '{')
            {
                if (p > segment_start)
                {
                    int len = p - segment_start;
                    char *seg = arena_strndup(parser->arena, segment_start, len);
                    LiteralValue v;
                    v.string_value = seg;
                    Expr *seg_expr = ast_create_literal_expr(parser->arena, v, ast_create_primitive_type(parser->arena, TYPE_STRING), false, &interpol_token);

                    if (count >= capacity)
                    {
                        capacity = capacity == 0 ? 8 : capacity * 2;
                        Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                        if (new_parts == NULL)
                        {
                            DEBUG_VERBOSE("Error: Out of memory for interpolated parts");
                            exit(1);
                        }
                        if (parts != NULL && count > 0)
                        {
                            memcpy(new_parts, parts, sizeof(Expr *) * count);
                        }
                        parts = new_parts;
                        DEBUG_VERBOSE("Reallocated parts array, new capacity=%d", capacity);
                    }
                    parts[count++] = seg_expr;
                    DEBUG_VERBOSE("Added string segment to interpolated parts, count=%d", count);
                }

                p++;
                const char *expr_start = p;
                while (*p && *p != '}')
                    p++;
                if (!*p)
                {
                    parser_error_at_current(parser, "Unterminated interpolated expression");
                    DEBUG_VERBOSE("Error: Unterminated interpolated expression");
                    LiteralValue zero = {0};
                    return ast_create_literal_expr(parser->arena, zero, ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
                }
                int expr_len = p - expr_start;
                char *expr_src = arena_strndup(parser->arena, expr_start, expr_len);
                DEBUG_VERBOSE("Extracted interpolated expression: %s", expr_src);

                Lexer sub_lexer;
                lexer_init(parser->arena, &sub_lexer, expr_src, "interpolated");
                Parser sub_parser;
                parser_init(parser->arena, &sub_parser, &sub_lexer, parser->symbol_table);
                sub_parser.symbol_table = parser->symbol_table;
                DEBUG_VERBOSE("Initialized sub-parser for interpolated expression");

                Expr *inner = parser_expression(&sub_parser);
                if (inner == NULL || sub_parser.had_error)
                {
                    parser_error_at_current(parser, "Invalid expression in interpolation");
                    DEBUG_VERBOSE("Error: Invalid expression in interpolation");
                    LiteralValue zero = {0};
                    return ast_create_literal_expr(parser->arena, zero, ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
                }

                if (count >= capacity)
                {
                    capacity = capacity == 0 ? 8 : capacity * 2;
                    Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                    if (new_parts == NULL)
                    {
                        DEBUG_VERBOSE("Error: Out of memory for interpolated parts");
                        exit(1);
                    }
                    if (parts != NULL && count > 0)
                    {
                        memcpy(new_parts, parts, sizeof(Expr *) * count);
                    }
                    parts = new_parts;
                    DEBUG_VERBOSE("Reallocated parts array, new capacity=%d", capacity);
                }
                parts[count++] = inner;
                DEBUG_VERBOSE("Added expression to interpolated parts, count=%d", count);

                if (parser->interp_count >= parser->interp_capacity)
                {
                    parser->interp_capacity = parser->interp_capacity ? parser->interp_capacity * 2 : 8;
                    char **new_interp_sources = arena_alloc(parser->arena, sizeof(char *) * parser->interp_capacity);
                    if (new_interp_sources == NULL)
                    {
                        DEBUG_VERBOSE("Error: Out of memory for interp_sources");
                        exit(1);
                    }
                    if (parser->interp_sources != NULL && parser->interp_count > 0)
                    {
                        memcpy(new_interp_sources, parser->interp_sources, sizeof(char *) * parser->interp_count);
                    }
                    parser->interp_sources = new_interp_sources;
                    DEBUG_VERBOSE("Reallocated interp_sources, new capacity=%d", parser->interp_capacity);
                }
                parser->interp_sources[parser->interp_count++] = expr_src;
                DEBUG_VERBOSE("Stored interpolated source, interp_count=%d", parser->interp_count);

                p++;
                segment_start = p;
            }
            else
            {
                p++;
            }
        }

        if (p > segment_start)
        {
            int len = p - segment_start;
            char *seg = arena_strndup(parser->arena, segment_start, len);
            LiteralValue v;
            v.string_value = seg;
            Expr *seg_expr = ast_create_literal_expr(parser->arena, v, ast_create_primitive_type(parser->arena, TYPE_STRING), false, &interpol_token);

            if (count >= capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                if (new_parts == NULL)
                {
                    DEBUG_VERBOSE("Error: Out of memory for interpolated parts");
                    exit(1);
                }
                if (parts != NULL && count > 0)
                {
                    memcpy(new_parts, parts, sizeof(Expr *) * count);
                }
                parts = new_parts;
                DEBUG_VERBOSE("Reallocated parts array, new capacity=%d", capacity);
            }
            parts[count++] = seg_expr;
            DEBUG_VERBOSE("Added final string segment to interpolated parts, count=%d", count);
        }

        Expr *result = ast_create_interpolated_expr(parser->arena, parts, count, &interpol_token);
        DEBUG_VERBOSE("Created interpolated expression with %d parts", count);
        return result;
    }

    parser_error_at_current(parser, "Expected expression");
    DEBUG_VERBOSE("Error: Expected expression in parser_primary");
    LiteralValue value;
    value.int_value = 0;
    Expr *result = ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_NIL), false, NULL);
    DEBUG_VERBOSE("Exiting parser_primary: returning NIL due to error");
    return result;
}

Expr *parser_call(Parser *parser, Expr *callee)
{
    DEBUG_VERBOSE("Entering parser_call");
    Token paren = parser->previous;
    Expr **arguments = NULL;
    int arg_count = 0;
    int capacity = 0;

    if (!parser_check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            if (arg_count >= 255)
            {
                parser_error_at_current(parser, "Cannot have more than 255 arguments");
                DEBUG_VERBOSE("Error: Too many arguments");
            }
            Expr *arg = parser_expression(parser);
            if (arg_count >= capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                Expr **new_arguments = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                if (new_arguments == NULL)
                {
                    DEBUG_VERBOSE("Error: Out of memory for arguments");
                    exit(1);
                }
                if (arguments != NULL && arg_count > 0)
                {
                    memcpy(new_arguments, arguments, sizeof(Expr *) * arg_count);
                }
                arguments = new_arguments;
                DEBUG_VERBOSE("Reallocated arguments array, new capacity=%d", capacity);
            }
            arguments[arg_count++] = arg;
            DEBUG_VERBOSE("Added argument, count=%d", arg_count);
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
    DEBUG_VERBOSE("Consumed RIGHT_PAREN for call");
    Expr *result = ast_create_call_expr(parser->arena, callee, arguments, arg_count, &paren);
    DEBUG_VERBOSE("Exiting parser_call: created call expression with %d arguments", arg_count);
    return result;
}

Expr *parser_array_access(Parser *parser, Expr *array)
{
    DEBUG_VERBOSE("Entering parser_array_access");
    Token bracket = parser->previous;
    Expr *index = parser_expression(parser);
    parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index");
    DEBUG_VERBOSE("Consumed RIGHT_BRACKET for array access");
    Expr *result = ast_create_array_access_expr(parser->arena, array, index, &bracket);
    DEBUG_VERBOSE("Exiting parser_array_access: created array access expression");
    return result;
}

Stmt *parser_statement(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_statement");
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        DEBUG_VERBOSE("Skipped NEWLINE token");
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        DEBUG_VERBOSE("Error: Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        DEBUG_VERBOSE("Found VAR, parsing variable declaration");
        Stmt *result = parser_var_declaration(parser);
        DEBUG_VERBOSE("Exiting parser_statement: parsed variable declaration");
        return result;
    }
    if (parser_match(parser, TOKEN_IF))
    {
        DEBUG_VERBOSE("Found IF, parsing if statement");
        Stmt *result = parser_if_statement(parser);
        DEBUG_VERBOSE("Exiting parser_statement: parsed if statement");
        return result;
    }
    if (parser_match(parser, TOKEN_WHILE))
    {
        DEBUG_VERBOSE("Found WHILE, parsing while statement");
        Stmt *result = parser_while_statement(parser);
        DEBUG_VERBOSE("Exiting parser_statement: parsed while statement");
        return result;
    }
    if (parser_match(parser, TOKEN_FOR))
    {
        DEBUG_VERBOSE("Found FOR, parsing for statement");
        Stmt *result = parser_for_statement(parser);
        DEBUG_VERBOSE("Exiting parser_statement: parsed for statement");
        return result;
    }
    if (parser_match(parser, TOKEN_RETURN))
    {
        DEBUG_VERBOSE("Found RETURN, parsing return statement");
        Stmt *result = parser_return_statement(parser);
        DEBUG_VERBOSE("Exiting parser_statement: parsed return statement");
        return result;
    }
    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        DEBUG_VERBOSE("Found LEFT_BRACE, parsing block statement");
        Stmt *result = parser_block_statement(parser);
        DEBUG_VERBOSE("Exiting parser_statement: parsed block statement");
        return result;
    }

    DEBUG_VERBOSE("Parsing expression statement");
    Stmt *result = parser_expression_statement(parser);
    DEBUG_VERBOSE("Exiting parser_statement: parsed expression statement");
    return result;
}

Stmt *parser_declaration(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_declaration");
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        DEBUG_VERBOSE("Skipped NEWLINE token");
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        DEBUG_VERBOSE("Error: Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        DEBUG_VERBOSE("Found VAR, parsing variable declaration");
        Stmt *result = parser_var_declaration(parser);
        DEBUG_VERBOSE("Exiting parser_declaration: parsed variable declaration");
        return result;
    }
    if (parser_match(parser, TOKEN_FN))
    {
        DEBUG_VERBOSE("Found FN, parsing function declaration");
        Stmt *result = parser_function_declaration(parser);
        DEBUG_VERBOSE("Exiting parser_declaration: parsed function declaration");
        return result;
    }
    if (parser_match(parser, TOKEN_IMPORT))
    {
        DEBUG_VERBOSE("Found IMPORT, parsing import statement");
        Stmt *result = parser_import_statement(parser);
        DEBUG_VERBOSE("Exiting parser_declaration: parsed import statement");
        return result;
    }

    DEBUG_VERBOSE("Parsing statement as declaration");
    Stmt *result = parser_statement(parser);
    DEBUG_VERBOSE("Exiting parser_declaration: parsed statement");
    return result;
}

Stmt *parser_var_declaration(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_var_declaration");
    Token var_token = parser->previous;
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            DEBUG_VERBOSE("Error: Out of memory for variable name");
            return NULL;
        }
        DEBUG_VERBOSE("Parsed variable name: %.*s", name.length, name.start);
    }
    else
    {
        parser_error_at_current(parser, "Expected variable name");
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            DEBUG_VERBOSE("Error: Out of memory for variable name");
            return NULL;
        }
        DEBUG_VERBOSE("Error: Expected variable name, using %.*s", name.length, name.start);
    }

    parser_consume(parser, TOKEN_COLON, "Expected ':' after variable name");
    DEBUG_VERBOSE("Consumed COLON after variable name");
    Type *type = parser_type(parser);
    DEBUG_VERBOSE("Parsed variable type");

    Expr *initializer = NULL;
    if (parser_match(parser, TOKEN_EQUAL))
    {
        initializer = parser_expression(parser);
        DEBUG_VERBOSE("Parsed initializer expression");
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after variable declaration");
        DEBUG_VERBOSE("Consumed SEMICOLON or NEWLINE after declaration");
    }

    Stmt *result = ast_create_var_decl_stmt(parser->arena, name, type, initializer, &var_token);
    DEBUG_VERBOSE("Exiting parser_var_declaration: created variable declaration");
    return result;
}

Stmt *parser_function_declaration(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_function_declaration");
    Token fn_token = parser->previous;
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            DEBUG_VERBOSE("Error: Out of memory for function name");
            return NULL;
        }
        DEBUG_VERBOSE("Parsed function name: %.*s", name.length, name.start);
    }
    else
    {
        parser_error_at_current(parser, "Expected function name");
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            DEBUG_VERBOSE("Error: Out of memory for function name");
            return NULL;
        }
        DEBUG_VERBOSE("Error: Expected function name, using %.*s", name.length, name.start);
    }

    Parameter *params = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        DEBUG_VERBOSE("Found LEFT_PAREN, parsing parameters");
        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                if (param_count >= 255)
                {
                    parser_error_at_current(parser, "Cannot have more than 255 parameters");
                    DEBUG_VERBOSE("Error: Too many parameters");
                }
                Token param_name;
                if (parser_check(parser, TOKEN_IDENTIFIER))
                {
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        DEBUG_VERBOSE("Error: Out of memory for parameter name");
                        return NULL;
                    }
                    DEBUG_VERBOSE("Parsed parameter name: %.*s", param_name.length, param_name.start);
                }
                else
                {
                    parser_error_at_current(parser, "Expected parameter name");
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        DEBUG_VERBOSE("Error: Out of memory for parameter name");
                        return NULL;
                    }
                    DEBUG_VERBOSE("Error: Expected parameter name, using %.*s", param_name.length, param_name.start);
                }
                parser_consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
                DEBUG_VERBOSE("Consumed COLON after parameter name");
                Type *param_type = parser_type(parser);
                DEBUG_VERBOSE("Parsed parameter type");

                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                    Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                    if (new_params == NULL)
                    {
                        DEBUG_VERBOSE("Error: Out of memory for parameters");
                        exit(1);
                    }
                    if (params != NULL && param_count > 0)
                    {
                        memcpy(new_params, params, sizeof(Parameter) * param_count);
                    }
                    params = new_params;
                    DEBUG_VERBOSE("Reallocated parameters array, new capacity=%d", param_capacity);
                }
                params[param_count].name = param_name;
                params[param_count].type = param_type;
                param_count++;
                DEBUG_VERBOSE("Added parameter, count=%d", param_count);
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
        DEBUG_VERBOSE("Consumed RIGHT_PAREN after parameters");
    }

    Type *return_type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    if (parser_match(parser, TOKEN_COLON))
    {
        return_type = parser_type(parser);
        DEBUG_VERBOSE("Parsed return type");
    }

    Type **param_types = arena_alloc(parser->arena, sizeof(Type *) * param_count);
    if (param_types == NULL && param_count > 0)
    {
        DEBUG_VERBOSE("Error: Out of memory for param_types");
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
    {
        param_types[i] = params[i].type;
    }
    Type *function_type = ast_create_function_type(parser->arena, return_type, param_types, param_count);
    DEBUG_VERBOSE("Created function type with %d parameters", param_count);

    symbol_table_add_symbol(parser->symbol_table, name, function_type);
    DEBUG_VERBOSE("Added function to symbol table: %.*s", name.length, name.start);

    parser_consume(parser, TOKEN_ARROW, "Expected '=>' before function body");
    DEBUG_VERBOSE("Consumed ARROW before function body");
    skip_newlines(parser);
    DEBUG_VERBOSE("Skipped newlines before function body");

    Stmt *body = parser_indented_block(parser);
    if (body == NULL)
    {
        body = ast_create_block_stmt(parser->arena, NULL, 0, NULL);
        DEBUG_VERBOSE("Created empty block statement for function body");
    }

    Stmt **stmts = body->as.block.statements;
    int stmt_count = body->as.block.count;
    body->as.block.statements = NULL;

    Stmt *result = ast_create_function_stmt(parser->arena, name, params, param_count, return_type, stmts, stmt_count, &fn_token);
    DEBUG_VERBOSE("Exiting parser_function_declaration: created function with %d statements", stmt_count);
    return result;
}

Stmt *parser_return_statement(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_return_statement");
    Token keyword = parser->previous;
    keyword.start = arena_strndup(parser->arena, keyword.start, keyword.length);
    if (keyword.start == NULL)
    {
        parser_error_at_current(parser, "Out of memory");
        DEBUG_VERBOSE("Error: Out of memory for keyword");
        return NULL;
    }
    DEBUG_VERBOSE("Parsed return keyword");

    Expr *value = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        value = parser_expression(parser);
        DEBUG_VERBOSE("Parsed return value expression");
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after return value");
        DEBUG_VERBOSE("Consumed SEMICOLON or NEWLINE after return");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
        DEBUG_VERBOSE("Consumed SEMICOLON after return");
    }

    Stmt *result = ast_create_return_stmt(parser->arena, keyword, value, &keyword);
    DEBUG_VERBOSE("Exiting parser_return_statement: created return statement");
    return result;
}

Stmt *parser_if_statement(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_if_statement");
    Token if_token = parser->previous;
    Expr *condition = parser_expression(parser);
    DEBUG_VERBOSE("Parsed if condition");
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after if condition");
    DEBUG_VERBOSE("Consumed ARROW after if condition");
    skip_newlines(parser);
    DEBUG_VERBOSE("Skipped newlines after if condition");

    Stmt *then_branch;
    if (parser_check(parser, TOKEN_INDENT))
    {
        then_branch = parser_indented_block(parser);
        DEBUG_VERBOSE("Parsed indented then branch");
    }
    else
    {
        then_branch = parser_statement(parser);
        DEBUG_VERBOSE("Parsed single statement then branch");
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                DEBUG_VERBOSE("Error: Out of memory for block statements");
                exit(1);
            }
            block_stmts[0] = then_branch;
            block_stmts[1] = parser_indented_block(parser);
            then_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
            DEBUG_VERBOSE("Created block for then branch with additional indented block");
        }
    }

    Stmt *else_branch = NULL;
    skip_newlines(parser);
    if (parser_match(parser, TOKEN_ELSE))
    {
        DEBUG_VERBOSE("Found ELSE, parsing else branch");
        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after else");
        DEBUG_VERBOSE("Consumed ARROW after else");
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            else_branch = parser_indented_block(parser);
            DEBUG_VERBOSE("Parsed indented else branch");
        }
        else
        {
            else_branch = parser_statement(parser);
            DEBUG_VERBOSE("Parsed single statement else branch");
            skip_newlines(parser);
            if (parser_check(parser, TOKEN_INDENT))
            {
                Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
                if (block_stmts == NULL)
                {
                    DEBUG_VERBOSE("Error: Out of memory for block statements");
                    exit(1);
                }
                block_stmts[0] = else_branch;
                block_stmts[1] = parser_indented_block(parser);
                else_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
                DEBUG_VERBOSE("Created block for else branch with additional indented block");
            }
        }
    }

    Stmt *result = ast_create_if_stmt(parser->arena, condition, then_branch, else_branch, &if_token);
    DEBUG_VERBOSE("Exiting parser_if_statement: created if statement");
    return result;
}

Stmt *parser_while_statement(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_while_statement");
    Token while_token = parser->previous;
    Expr *condition = parser_expression(parser);
    DEBUG_VERBOSE("Parsed while condition");
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after while condition");
    DEBUG_VERBOSE("Consumed ARROW after while condition");
    skip_newlines(parser);
    DEBUG_VERBOSE("Skipped newlines after while condition");

    Stmt *body;
    if (parser_check(parser, TOKEN_INDENT))
    {
        body = parser_indented_block(parser);
        DEBUG_VERBOSE("Parsed indented while body");
    }
    else
    {
        body = parser_statement(parser);
        DEBUG_VERBOSE("Parsed single statement while body");
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                DEBUG_VERBOSE("Error: Out of memory for block statements");
                exit(1);
            }
            block_stmts[0] = body;
            block_stmts[1] = parser_indented_block(parser);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
            DEBUG_VERBOSE("Created block for while body with additional indented block");
        }
    }

    Stmt *result = ast_create_while_stmt(parser->arena, condition, body, &while_token);
    DEBUG_VERBOSE("Exiting parser_while_statement: created while statement");
    return result;
}

Stmt *parser_for_statement(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_for_statement");
    Token for_token = parser->previous;
    Stmt *initializer = NULL;
    if (parser_match(parser, TOKEN_VAR))
    {
        DEBUG_VERBOSE("Found VAR, parsing for initializer");
        Token var_token = parser->previous;
        Token name;
        if (parser_check(parser, TOKEN_IDENTIFIER))
        {
            name = parser->current;
            parser_advance(parser);
            name.start = arena_strndup(parser->arena, name.start, name.length);
            if (name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                DEBUG_VERBOSE("Error: Out of memory for variable name");
                return NULL;
            }
            DEBUG_VERBOSE("Parsed variable name: %.*s", name.length, name.start);
        }
        else
        {
            parser_error_at_current(parser, "Expected variable name");
            name = parser->current;
            parser_advance(parser);
            name.start = arena_strndup(parser->arena, name.start, name.length);
            if (name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                DEBUG_VERBOSE("Error: Out of memory for variable name");
                return NULL;
            }
            DEBUG_VERBOSE("Error: Expected variable name, using %.*s", name.length, name.start);
        }
        parser_consume(parser, TOKEN_COLON, "Expected ':' after variable name");
        DEBUG_VERBOSE("Consumed COLON after variable name");
        Type *type = parser_type(parser);
        DEBUG_VERBOSE("Parsed variable type");
        Expr *init_expr = NULL;
        if (parser_match(parser, TOKEN_EQUAL))
        {
            init_expr = parser_expression(parser);
            DEBUG_VERBOSE("Parsed initializer expression");
        }
        initializer = ast_create_var_decl_stmt(parser->arena, name, type, init_expr, &var_token);
        DEBUG_VERBOSE("Created variable declaration for for initializer");
    }
    else if (!parser_check(parser, TOKEN_SEMICOLON))
    {
        Expr *init_expr = parser_expression(parser);
        initializer = ast_create_expr_stmt(parser->arena, init_expr, NULL);
        DEBUG_VERBOSE("Created expression statement for for initializer");
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after initializer");
    DEBUG_VERBOSE("Consumed SEMICOLON after initializer");

    Expr *condition = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON))
    {
        condition = parser_expression(parser);
        DEBUG_VERBOSE("Parsed for condition");
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after condition");
    DEBUG_VERBOSE("Consumed SEMICOLON after condition");

    Expr *increment = NULL;
    if (!parser_check(parser, TOKEN_ARROW))
    {
        increment = parser_expression(parser);
        DEBUG_VERBOSE("Parsed for increment");
    }
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after for clauses");
    DEBUG_VERBOSE("Consumed ARROW after for clauses");
    skip_newlines(parser);
    DEBUG_VERBOSE("Skipped newlines after for clauses");

    Stmt *body;
    if (parser_check(parser, TOKEN_INDENT))
    {
        body = parser_indented_block(parser);
        DEBUG_VERBOSE("Parsed indented for body");
    }
    else
    {
        body = parser_statement(parser);
        DEBUG_VERBOSE("Parsed single statement for body");
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                DEBUG_VERBOSE("Error: Out of memory for block statements");
                exit(1);
            }
            block_stmts[0] = body;
            block_stmts[1] = parser_indented_block(parser);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
            DEBUG_VERBOSE("Created block for for body with additional indented block");
        }
    }

    Stmt *result = ast_create_for_stmt(parser->arena, initializer, condition, increment, body, &for_token);
    DEBUG_VERBOSE("Exiting parser_for_statement: created for statement");
    return result;
}

Stmt *parser_block_statement(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_block_statement");
    Token brace = parser->previous;
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    symbol_table_push_scope(parser->symbol_table);
    DEBUG_VERBOSE("Pushed new scope for block");

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            DEBUG_VERBOSE("Skipped NEWLINE token");
        }
        if (parser_is_at_end(parser) || parser_check(parser, TOKEN_DEDENT))
        {
            DEBUG_VERBOSE("Found end or DEDENT, breaking loop");
            break;
        }

        Stmt *stmt = parser_declaration(parser);
        if (stmt == NULL)
        {
            DEBUG_VERBOSE("parser_declaration returned NULL, continuing");
            continue;
        }

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Stmt **new_statements = arena_alloc(parser->arena, sizeof(Stmt *) * capacity);
            if (new_statements == NULL)
            {
                DEBUG_VERBOSE("Error: Out of memory for statements");
                exit(1);
            }
            if (statements != NULL && count > 0)
            {
                memcpy(new_statements, statements, sizeof(Stmt *) * count);
            }
            statements = new_statements;
            DEBUG_VERBOSE("Reallocated statements array, new capacity=%d", capacity);
        }
        statements[count++] = stmt;
        DEBUG_VERBOSE("Added statement to block, count=%d", count);
    }

    symbol_table_pop_scope(parser->symbol_table);
    DEBUG_VERBOSE("Popped scope for block");

    Stmt *result = ast_create_block_stmt(parser->arena, statements, count, &brace);
    DEBUG_VERBOSE("Exiting parser_block_statement: created block with %d statements", count);
    return result;
}

Stmt *parser_expression_statement(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_expression_statement");
    Expr *expression = parser_expression(parser);
    DEBUG_VERBOSE("Parsed expression");

    if (parser_match(parser, TOKEN_SEMICOLON))
    {
        // Consumed semicolon; this is fine for statements ending with ';'.
    }
    else if (parser_check(parser, TOKEN_NEWLINE))
    {
        // Consume newline if present.
        parser_consume(parser, TOKEN_NEWLINE, "Expected newline after expression");
    }
    else if (parser_is_at_end(parser))
    {
        // Accept end-of-file immediately after the expression without requiring a newline.
        // This handles files that do not end with a trailing newline.
        DEBUG_VERBOSE("Accepted end-of-file after expression without trailing newline");
    }
    else
    {
        // Error if neither ';' nor newline nor EOF follows the expression.
        parser_error(parser, "Expected ';' or newline after expression");
    }
    DEBUG_VERBOSE("Consumed SEMICOLON or NEWLINE after expression or accepted EOF");

    Stmt *result = ast_create_expr_stmt(parser->arena, expression, &parser->previous);
    DEBUG_VERBOSE("Exiting parser_expression_statement: created expression statement");
    return result;
}

Stmt *parser_import_statement(Parser *parser)
{
    DEBUG_VERBOSE("Entering parser_import_statement");
    Token import_token = parser->previous;
    Token module_name;
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        module_name = parser->previous;
        module_name.start = arena_strdup(parser->arena, parser->previous.literal.string_value);
        if (module_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            DEBUG_VERBOSE("Error: Out of memory for module name");
            return NULL;
        }
        module_name.length = strlen(module_name.start);
        module_name.line = parser->previous.line;
        module_name.filename = parser->previous.filename;
        module_name.type = TOKEN_STRING_LITERAL;
        DEBUG_VERBOSE("Parsed module name: %s", module_name.start);
    }
    else
    {
        parser_error_at_current(parser, "Expected module name as string");
        module_name = parser->current;
        parser_advance(parser);
        module_name.start = arena_strndup(parser->arena, module_name.start, module_name.length);
        if (module_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            DEBUG_VERBOSE("Error: Out of memory for module name");
            return NULL;
        }
        DEBUG_VERBOSE("Error: Expected module name, using %.*s", module_name.length, module_name.start);
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after import statement");
        DEBUG_VERBOSE("Consumed SEMICOLON or NEWLINE after import");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
        DEBUG_VERBOSE("Consumed SEMICOLON after import");
    }

    Stmt *result = ast_create_import_stmt(parser->arena, module_name, &import_token);
    DEBUG_VERBOSE("Exiting parser_import_statement: created import statement");
    return result;
}

Module *parser_execute(Parser *parser, const char *filename)
{
    DEBUG_VERBOSE("Entering parser_execute: filename=%s", filename);
    Module *module = arena_alloc(parser->arena, sizeof(Module));
    ast_init_module(parser->arena, module, filename);
    DEBUG_VERBOSE("Initialized module");

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            DEBUG_VERBOSE("Skipped NEWLINE token");
        }
        if (parser_is_at_end(parser))
        {
            DEBUG_VERBOSE("Reached end of file, breaking");
            break;
        }

        Stmt *stmt = parser_declaration(parser);
        if (stmt != NULL)
        {
            ast_module_add_statement(parser->arena, module, stmt);
            DEBUG_VERBOSE("Added statement to module");
            ast_print_stmt(parser->arena, stmt, 0);
        }

        if (parser->panic_mode)
        {
            synchronize(parser);
            DEBUG_VERBOSE("Synchronized after panic mode");
        }
    }

    if (parser->had_error)
    {
        DEBUG_VERBOSE("Exiting parser_execute: returning NULL due to errors");
        return NULL;
    }

    DEBUG_VERBOSE("Exiting parser_execute: parsed %d statements", module->count);
    return module;
}

Module *parse_module_with_imports(Arena *arena, SymbolTable *symbol_table, const char *filename, char ***imported, int *imported_count, int *imported_capacity)
{
    DEBUG_VERBOSE("Entering parse_module_with_imports: filename=%s", filename);
    char *source = file_read(arena, filename);
    if (!source)
    {
        DEBUG_ERROR("Failed to read file: %s", filename);
        return NULL;
    }
    DEBUG_VERBOSE("Read source file");

    Lexer lexer;
    lexer_init(arena, &lexer, source, filename);
    DEBUG_VERBOSE("Initialized lexer");

    Parser parser;
    parser_init(arena, &parser, &lexer, symbol_table);
    DEBUG_VERBOSE("Initialized parser");

    Module *module = parser_execute(&parser, filename);
    if (!module || parser.had_error)
    {
        parser_cleanup(&parser);
        lexer_cleanup(&lexer);
        DEBUG_VERBOSE("Error: parser_execute failed or had errors, cleaning up");
        return NULL;
    }
    DEBUG_VERBOSE("Executed parser, module has %d statements", module->count);

    // Collect all statements, starting with imported ones
    Stmt **all_statements = NULL;
    int all_count = 0;
    int all_capacity = 0;

    // Extract directory from current filename
    const char *dir_end = strrchr(filename, '/');
    size_t dir_len = dir_end ? (size_t)(dir_end - filename + 1) : 0;
    char *dir = NULL;
    if (dir_len > 0) {
        dir = arena_alloc(arena, dir_len + 1);
        if (!dir) {
            DEBUG_ERROR("Failed to allocate memory for directory path");
            parser_cleanup(&parser);
            lexer_cleanup(&lexer);
            return NULL;
        }
        strncpy(dir, filename, dir_len);
        dir[dir_len] = '\0';
        DEBUG_VERBOSE("Extracted directory: %s", dir);
    }

    // Process imports (remove them and prepend imported statements)
    for (int i = 0; i < module->count;)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_IMPORT)
        {
            DEBUG_VERBOSE("Processing import statement at index %d", i);
            Token mod_name = stmt->as.import.module_name;
            size_t mod_name_len = strlen(mod_name.start);
            size_t path_len = dir_len + mod_name_len + 4; // ".sn\0"
            char *import_path = arena_alloc(arena, path_len);
            if (!import_path)
            {
                DEBUG_ERROR("Failed to allocate memory for import path");
                parser_cleanup(&parser);
                lexer_cleanup(&lexer);
                return NULL;
            }
            if (dir_len > 0) {
                strcpy(import_path, dir);
            } else {
                import_path[0] = '\0';
            }
            strcat(import_path, mod_name.start);
            strcat(import_path, ".sn");
            DEBUG_VERBOSE("Constructed import path: %s", import_path);

            // Check if already imported to avoid cycles/duplicates
            int already_imported = 0;
            for (int j = 0; j < *imported_count; j++)
            {
                if (strcmp((*imported)[j], import_path) == 0)
                {
                    already_imported = 1;
                    DEBUG_VERBOSE("Import path already imported, skipping: %s", import_path);
                    break;
                }
            }

            if (already_imported)
            {
                // Remove the import statement
                memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
                module->count--;
                DEBUG_VERBOSE("Removed duplicate import statement, new count=%d", module->count);
                continue;
            }

            // Add to imported list
            if (*imported_count >= *imported_capacity)
            {
                *imported_capacity = *imported_capacity == 0 ? 8 : *imported_capacity * 2;
                char **new_imported = arena_alloc(arena, sizeof(char *) * *imported_capacity);
                if (!new_imported)
                {
                    DEBUG_ERROR("Failed to allocate memory for imported list");
                    parser_cleanup(&parser);
                    lexer_cleanup(&lexer);
                    return NULL;
                }
                if (*imported_count > 0)
                {
                    memcpy(new_imported, *imported, sizeof(char *) * *imported_count);
                }
                *imported = new_imported;
                DEBUG_VERBOSE("Reallocated imported list, new capacity=%d", *imported_capacity);
            }
            (*imported)[(*imported_count)++] = import_path;
            DEBUG_VERBOSE("Added import path to list, imported_count=%d", *imported_count);

            // Recursively parse the imported module
            Module *imported_module = parse_module_with_imports(arena, symbol_table, import_path, imported, imported_count, imported_capacity);
            if (!imported_module)
            {
                parser_cleanup(&parser);
                lexer_cleanup(&lexer);
                DEBUG_VERBOSE("Error: Failed to parse imported module");
                return NULL;
            }
            DEBUG_VERBOSE("Parsed imported module with %d statements", imported_module->count);

            // Append imported statements to all_statements
            int new_all_count = all_count + imported_module->count;
            if (new_all_count > all_capacity)
            {
                all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
                Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
                if (!new_statements)
                {
                    DEBUG_ERROR("Failed to allocate memory for statements");
                    parser_cleanup(&parser);
                    lexer_cleanup(&lexer);
                    return NULL;
                }
                if (all_count > 0)
                {
                    memcpy(new_statements, all_statements, sizeof(Stmt *) * all_count);
                }
                all_statements = new_statements;
                DEBUG_VERBOSE("Reallocated all_statements, new capacity=%d", all_capacity);
            }
            memcpy(all_statements + all_count, imported_module->statements, sizeof(Stmt *) * imported_module->count);
            all_count = new_all_count;
            DEBUG_VERBOSE("Appended %d imported statements, total count=%d", imported_module->count, all_count);

            // Remove the import statement
            memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
            module->count--;
            DEBUG_VERBOSE("Removed import statement, new module count=%d", module->count);
        }
        else
        {
            i++;
        }
    }

    // Append the current module's remaining statements
    int new_all_count = all_count + module->count;
    if (new_all_count > all_capacity)
    {
        all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
        Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
        if (!new_statements)
        {
            DEBUG_ERROR("Failed to allocate memory for statements");
            parser_cleanup(&parser);
            lexer_cleanup(&lexer);
            return NULL;
        }
        if (all_count > 0)
        {
            memcpy(new_statements, all_statements, sizeof(Stmt *) * all_count);
        }
        all_statements = new_statements;
        DEBUG_VERBOSE("Reallocated all_statements for module statements, new capacity=%d", all_capacity);
    }
    memcpy(all_statements + all_count, module->statements, sizeof(Stmt *) * module->count);
    all_count = new_all_count;
    DEBUG_VERBOSE("Appended %d module statements, total count=%d", module->count, all_count);

    // Update the module with the combined statements
    module->statements = all_statements;
    module->count = all_count;
    module->capacity = all_count;
    DEBUG_VERBOSE("Updated module: count=%d, capacity=%d", module->count, module->capacity);

    parser_cleanup(&parser);
    lexer_cleanup(&lexer);
    DEBUG_VERBOSE("Exiting parse_module_with_imports: cleaned up parser and lexer");
    return module;
}