// parser.c
#include "parser.h"
#include "debug.h"
#include "file.h"
#include <stdio.h>
#include <string.h>

int parser_is_at_end(Parser *parser)
{
    return parser->current.type == TOKEN_EOF;
}

void skip_newlines(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        if (parser_check(parser, TOKEN_INDENT) || parser_check(parser, TOKEN_DEDENT))
        {
            break;
        }
    }
}

int is_at_function_boundary(Parser *parser)
{
    if (parser_check(parser, TOKEN_DEDENT))
    {
        return 1;
    }
    if (parser_check(parser, TOKEN_FN))
    {
        return 1;
    }
    if (parser_check(parser, TOKEN_EOF))
    {
        return 1;
    }
    return 0;
}

Stmt *parser_indented_block(Parser *parser)
{
    if (!parser_check(parser, TOKEN_INDENT))
    {
        parser_error(parser, "Expected indented block");
        return NULL;
    }
    parser_advance(parser);

    int current_indent = parser->lexer->indent_stack[parser->lexer->indent_size - 1];
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    while (!parser_is_at_end(parser) &&
           parser->lexer->indent_stack[parser->lexer->indent_size - 1] >= current_indent)
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }

        if (parser_check(parser, TOKEN_DEDENT))
        {
            break;
        }

        if (parser_check(parser, TOKEN_EOF))
        {
            break;
        }

        Stmt *stmt = parser_declaration(parser);
        if (stmt == NULL)
        {
            continue;
        }

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Stmt **new_statements = arena_alloc(parser->arena, sizeof(Stmt *) * capacity);
            if (new_statements == NULL)
            {
                exit(1);
            }
            if (statements != NULL && count > 0)
            {
                memcpy(new_statements, statements, sizeof(Stmt *) * count);
            }
            statements = new_statements;
        }
        statements[count++] = stmt;
    }

    if (parser_check(parser, TOKEN_DEDENT))
    {
        parser_advance(parser);
    }
    else if (parser->lexer->indent_stack[parser->lexer->indent_size - 1] < current_indent)
    {
        parser_error(parser, "Expected dedent to end block");
    }

    return ast_create_block_stmt(parser->arena, statements, count, NULL);
}

Expr *parser_multi_line_expression(Parser *parser)
{
    Expr *expr = parser_expression(parser);

    while (parser_match(parser, TOKEN_NEWLINE))
    {
        Token op_token = parser->previous;
        Expr *right = parser_expression(parser);
        expr = ast_create_binary_expr(parser->arena, expr, TOKEN_PLUS, right, &op_token);
    }

    return expr;
}

int skip_newlines_and_check_end(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }
    return parser_is_at_end(parser);
}

void parser_init(Arena *arena, Parser *parser, Lexer *lexer, SymbolTable *symbol_table)
{
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

    Type *any_type = ast_create_primitive_type(arena, TYPE_ANY);
    Type **builtin_params = arena_alloc(arena, sizeof(Type *));
    builtin_params[0] = any_type;

    Type *print_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, print_token, print_type, SYMBOL_GLOBAL);

    Token to_string_token;
    to_string_token.start = arena_strdup(arena, "to_string");
    to_string_token.length = 9;
    to_string_token.type = TOKEN_IDENTIFIER;
    to_string_token.line = 0;
    to_string_token.filename = arena_strdup(arena, "<built-in>");

    Type *to_string_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, to_string_token, to_string_type, SYMBOL_GLOBAL);

    parser->previous.type = TOKEN_ERROR;
    parser->previous.start = NULL;
    parser->previous.length = 0;
    parser->previous.line = 0;
    parser->current = parser->previous;

    parser_advance(parser);
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
}

void parser_cleanup(Parser *parser)
{
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    parser->previous.start = NULL;
    parser->current.start = NULL;
}

void parser_error(Parser *parser, const char *message)
{
    parser_error_at(parser, &parser->previous, message);
}

void parser_error_at_current(Parser *parser, const char *message)
{
    parser_error_at(parser, &parser->current, message);
}

void parser_error_at(Parser *parser, Token *token, const char *message)
{
    if (parser->panic_mode)
        return;

    parser->panic_mode = 1;
    parser->had_error = 1;

    fprintf(stderr, "[%s:%d] Error", parser->lexer->filename, token->line);
    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);

    parser->lexer->indent_size = 1;

    // Advance if the error is at the current token to prevent infinite loops
    // by skipping the offending token and allowing parsing to progress.
    if (token == &parser->current)
    {
        parser_advance(parser);
    }
}

void parser_advance(Parser *parser)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = lexer_scan_token(parser->lexer);
        if (parser->current.type != TOKEN_ERROR)
            break;
        parser_error_at_current(parser, parser->current.start);
    }
}

void parser_consume(Parser *parser, TokenType type, const char *message)
{
    if (parser->current.type == type)
    {
        parser_advance(parser);
        return;
    }
    parser_error_at_current(parser, message);
}

int parser_check(Parser *parser, TokenType type)
{
    return parser->current.type == type;
}

int parser_match(Parser *parser, TokenType type)
{
    if (!parser_check(parser, type))
        return 0;
    parser_advance(parser);
    return 1;
}

static void synchronize(Parser *parser)
{
    parser->panic_mode = 0;

    while (!parser_is_at_end(parser))
    {
        if (parser->previous.type == TOKEN_SEMICOLON || parser->previous.type == TOKEN_NEWLINE)
            return;

        switch (parser->current.type)
        {
        case TOKEN_FN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_RETURN:
        case TOKEN_IMPORT:
        case TOKEN_ELSE: // Added to handle else clauses during synchronization
            return;
        case TOKEN_NEWLINE:
            parser_advance(parser);
            break;
        default:
            parser_advance(parser);
            break;
        }
    }
}

Type *parser_type(Parser *parser)
{
    Type *type = NULL;

    // Parse the base primitive type.
    if (parser_match(parser, TOKEN_INT)) {
        type = ast_create_primitive_type(parser->arena, TYPE_INT);
    } else if (parser_match(parser, TOKEN_LONG)) {
        type = ast_create_primitive_type(parser->arena, TYPE_LONG);
    } else if (parser_match(parser, TOKEN_DOUBLE)) {
        type = ast_create_primitive_type(parser->arena, TYPE_DOUBLE);
    } else if (parser_match(parser, TOKEN_CHAR)) {
        type = ast_create_primitive_type(parser->arena, TYPE_CHAR);
    } else if (parser_match(parser, TOKEN_STR)) {
        type = ast_create_primitive_type(parser->arena, TYPE_STRING);
    } else if (parser_match(parser, TOKEN_BOOL)) {
        type = ast_create_primitive_type(parser->arena, TYPE_BOOL);
    } else if (parser_match(parser, TOKEN_VOID)) {
        type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    } else {
        parser_error_at_current(parser, "Expected type");
        // Return a fallback type to allow parsing to continue without crashing.
        return ast_create_primitive_type(parser->arena, TYPE_NIL);
    }

    // Handle optional array suffixes (e.g., [] or [][] for multi-dimensional).
    while (parser_match(parser, TOKEN_LEFT_BRACKET)) {
        parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after '[' in array type");
        type = ast_create_array_type(parser->arena, type);
    }

    return type;
}

Expr *parser_expression(Parser *parser)
{
    Expr *result = parser_assignment(parser);
    if (result == NULL)
    {
        parser_error_at_current(parser, "Expected expression");
        parser_advance(parser);
    }
    return result;
}

Expr *parser_assignment(Parser *parser)
{
    Expr *expr = parser_logical_or(parser);

    if (parser_match(parser, TOKEN_EQUAL))
    {
        Token equals = parser->previous;
        Expr *value = parser_assignment(parser);
        if (expr->type == EXPR_VARIABLE)
        {
            Token name = expr->as.variable.name;
            char *new_start = arena_strndup(parser->arena, name.start, name.length);
            if (new_start == NULL)
            {
                exit(1);
            }
            name.start = new_start;
            return ast_create_assign_expr(parser->arena, name, value, &equals);
        }
        parser_error(parser, "Invalid assignment target");
    }
    return expr;
}

Expr *parser_logical_or(Parser *parser)
{
    Expr *expr = parser_logical_and(parser);
    while (parser_match(parser, TOKEN_OR))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_logical_and(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_logical_and(Parser *parser)
{
    Expr *expr = parser_equality(parser);
    while (parser_match(parser, TOKEN_AND))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_equality(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_equality(Parser *parser)
{
    Expr *expr = parser_comparison(parser);
    while (parser_match(parser, TOKEN_BANG_EQUAL) || parser_match(parser, TOKEN_EQUAL_EQUAL))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_comparison(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_comparison(Parser *parser)
{
    Expr *expr = parser_term(parser);
    while (parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_LESS_EQUAL) ||
           parser_match(parser, TOKEN_GREATER) || parser_match(parser, TOKEN_GREATER_EQUAL))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_term(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_term(Parser *parser)
{
    Expr *expr = parser_factor(parser);
    while (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_factor(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_factor(Parser *parser)
{
    Expr *expr = parser_unary(parser);
    while (parser_match(parser, TOKEN_STAR) || parser_match(parser, TOKEN_SLASH) || parser_match(parser, TOKEN_MODULO))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_unary(parser);
        expr = ast_create_binary_expr(parser->arena, expr, operator, right, &op);
    }
    return expr;
}

Expr *parser_unary(Parser *parser)
{
    if (parser_match(parser, TOKEN_BANG) || parser_match(parser, TOKEN_MINUS))
    {
        Token op = parser->previous;
        TokenType operator = op.type;
        Expr *right = parser_unary(parser);
        return ast_create_unary_expr(parser->arena, operator, right, &op);
    }
    return parser_postfix(parser);
}

Expr *parser_postfix(Parser *parser)
{
    Expr *expr = parser_primary(parser);
    for (;;)
    {
        if (parser_match(parser, TOKEN_LEFT_PAREN))
        {
            expr = parser_call(parser, expr);
        }
        else if (parser_match(parser, TOKEN_LEFT_BRACKET))
        {
            expr = parser_array_access(parser, expr);
        }
        else if (parser_match(parser, TOKEN_PLUS_PLUS))
        {
            expr = ast_create_increment_expr(parser->arena, expr, &parser->previous);
        }
        else if (parser_match(parser, TOKEN_MINUS_MINUS))
        {
            expr = ast_create_decrement_expr(parser->arena, expr, &parser->previous);
        }
        else
        {
            break;
        }
    }
    return expr;
}

Expr *parser_primary(Parser *parser)
{
    if (parser_match(parser, TOKEN_INT_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_INT), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_LONG_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_LONG), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_DOUBLE_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_DOUBLE), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_CHAR_LITERAL))
    {
        LiteralValue value;
        value.char_value = parser->previous.literal.char_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_CHAR), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        LiteralValue value;
        value.string_value = arena_strdup(parser->arena, parser->previous.literal.string_value);
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_STRING), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_BOOL_LITERAL))
    {
        LiteralValue value;
        value.bool_value = parser->previous.literal.bool_value;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_BOOL), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_NIL))
    {
        LiteralValue value;
        value.int_value = 0;
        return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_NIL), false, &parser->previous);
    }
    if (parser_match(parser, TOKEN_IDENTIFIER))
    {
        Token var_token = parser->previous;
        var_token.start = arena_strndup(parser->arena, parser->previous.start, parser->previous.length);
        return ast_create_variable_expr(parser->arena, var_token, &parser->previous);
    }
    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        Expr *expr = parser_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }

    if (parser_match(parser, TOKEN_INTERPOL_STRING))
    {
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
                            exit(1);
                        }
                        if (parts != NULL && count > 0)
                        {
                            memcpy(new_parts, parts, sizeof(Expr *) * count);
                        }
                        parts = new_parts;
                    }
                    parts[count++] = seg_expr;
                }

                p++;
                const char *expr_start = p;
                while (*p && *p != '}')
                    p++;
                if (!*p)
                {
                    parser_error_at_current(parser, "Unterminated interpolated expression");
                    LiteralValue zero = {0};
                    return ast_create_literal_expr(parser->arena, zero, ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
                }
                int expr_len = p - expr_start;
                char *expr_src = arena_strndup(parser->arena, expr_start, expr_len);

                Lexer sub_lexer;
                lexer_init(parser->arena, &sub_lexer, expr_src, "interpolated");
                Parser sub_parser;
                parser_init(parser->arena, &sub_parser, &sub_lexer, parser->symbol_table);
                sub_parser.symbol_table = parser->symbol_table;

                Expr *inner = parser_expression(&sub_parser);
                if (inner == NULL || sub_parser.had_error)
                {
                    parser_error_at_current(parser, "Invalid expression in interpolation");
                    LiteralValue zero = {0};
                    return ast_create_literal_expr(parser->arena, zero, ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
                }

                if (count >= capacity)
                {
                    capacity = capacity == 0 ? 8 : capacity * 2;
                    Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                    if (new_parts == NULL)
                    {
                        exit(1);
                    }
                    if (parts != NULL && count > 0)
                    {
                        memcpy(new_parts, parts, sizeof(Expr *) * count);
                    }
                    parts = new_parts;
                }
                parts[count++] = inner;

                if (parser->interp_count >= parser->interp_capacity)
                {
                    parser->interp_capacity = parser->interp_capacity ? parser->interp_capacity * 2 : 8;
                    char **new_interp_sources = arena_alloc(parser->arena, sizeof(char *) * parser->interp_capacity);
                    if (new_interp_sources == NULL)
                    {
                        exit(1);
                    }
                    if (parser->interp_sources != NULL && parser->interp_count > 0)
                    {
                        memcpy(new_interp_sources, parser->interp_sources, sizeof(char *) * parser->interp_count);
                    }
                    parser->interp_sources = new_interp_sources;
                }
                parser->interp_sources[parser->interp_count++] = expr_src;

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
                    exit(1);
                }
                if (parts != NULL && count > 0)
                {
                    memcpy(new_parts, parts, sizeof(Expr *) * count);
                }
                parts = new_parts;
            }
            parts[count++] = seg_expr;
        }

        return ast_create_interpolated_expr(parser->arena, parts, count, &interpol_token);
    }

    parser_error_at_current(parser, "Expected expression");
    LiteralValue value;
    value.int_value = 0;
    return ast_create_literal_expr(parser->arena, value, ast_create_primitive_type(parser->arena, TYPE_NIL), false, NULL);
}

Expr *parser_call(Parser *parser, Expr *callee)
{
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
            }
            Expr *arg = parser_expression(parser);
            if (arg_count >= capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                Expr **new_arguments = arena_alloc(parser->arena, sizeof(Expr *) * capacity);
                if (new_arguments == NULL)
                {
                    exit(1);
                }
                if (arguments != NULL && arg_count > 0)
                {
                    memcpy(new_arguments, arguments, sizeof(Expr *) * arg_count);
                }
                arguments = new_arguments;
            }
            arguments[arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
    return ast_create_call_expr(parser->arena, callee, arguments, arg_count, &paren);
}

Expr *parser_array_access(Parser *parser, Expr *array)
{
    Token bracket = parser->previous;
    Expr *index = parser_expression(parser);
    parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index");
    return ast_create_array_access_expr(parser->arena, array, index, &bracket);
}

Stmt *parser_statement(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        return parser_var_declaration(parser);
    }
    if (parser_match(parser, TOKEN_IF))
    {
        return parser_if_statement(parser);
    }
    if (parser_match(parser, TOKEN_WHILE))
    {
        return parser_while_statement(parser);
    }
    if (parser_match(parser, TOKEN_FOR))
    {
        return parser_for_statement(parser);
    }
    if (parser_match(parser, TOKEN_RETURN))
    {
        return parser_return_statement(parser);
    }
    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        return parser_block_statement(parser);
    }

    return parser_expression_statement(parser);
}

Stmt *parser_declaration(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        return parser_var_declaration(parser);
    }
    if (parser_match(parser, TOKEN_FN))
    {
        return parser_function_declaration(parser);
    }
    if (parser_match(parser, TOKEN_IMPORT))
    {
        return parser_import_statement(parser);
    }

    return parser_statement(parser);
}

Stmt *parser_var_declaration(Parser *parser)
{
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
            return NULL;
        }
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
            return NULL;
        }
    }

    parser_consume(parser, TOKEN_COLON, "Expected ':' after variable name");
    Type *type = parser_type(parser);

    Expr *initializer = NULL;
    if (parser_match(parser, TOKEN_EQUAL))
    {
        initializer = parser_expression(parser);
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after variable declaration");
    }

    return ast_create_var_decl_stmt(parser->arena, name, type, initializer, &var_token);
}

Stmt *parser_function_declaration(Parser *parser)
{
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
            return NULL;
        }
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
            return NULL;
        }
    }

    Parameter *params = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                if (param_count >= 255)
                {
                    parser_error_at_current(parser, "Cannot have more than 255 parameters");
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
                        return NULL;
                    }
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
                        return NULL;
                    }
                }
                parser_consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
                Type *param_type = parser_type(parser);

                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                    Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                    if (new_params == NULL)
                    {
                        exit(1);
                    }
                    if (params != NULL && param_count > 0)
                    {
                        memcpy(new_params, params, sizeof(Parameter) * param_count);
                    }
                    params = new_params;
                }
                params[param_count].name = param_name;
                params[param_count].type = param_type;
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }

    Type *return_type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    if (parser_match(parser, TOKEN_COLON))
    {
        return_type = parser_type(parser);
    }

    Type **param_types = arena_alloc(parser->arena, sizeof(Type *) * param_count);
    if (param_types == NULL && param_count > 0)
    {
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
    {
        param_types[i] = params[i].type;
    }
    Type *function_type = ast_create_function_type(parser->arena, return_type, param_types, param_count);

    symbol_table_add_symbol(parser->symbol_table, name, function_type);

    parser_consume(parser, TOKEN_ARROW, "Expected '=>' before function body");
    skip_newlines(parser);

    Stmt *body = parser_indented_block(parser);
    if (body == NULL)
    {
        body = ast_create_block_stmt(parser->arena, NULL, 0, NULL);
    }

    Stmt **stmts = body->as.block.statements;
    int stmt_count = body->as.block.count;
    body->as.block.statements = NULL;

    return ast_create_function_stmt(parser->arena, name, params, param_count, return_type, stmts, stmt_count, &fn_token);
}

Stmt *parser_return_statement(Parser *parser)
{
    Token keyword = parser->previous;
    keyword.start = arena_strndup(parser->arena, keyword.start, keyword.length);
    if (keyword.start == NULL)
    {
        parser_error_at_current(parser, "Out of memory");
        return NULL;
    }

    Expr *value = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        value = parser_expression(parser);
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after return value");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_return_stmt(parser->arena, keyword, value, &keyword);
}

Stmt *parser_if_statement(Parser *parser)
{
    Token if_token = parser->previous;
    Expr *condition = parser_expression(parser);
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after if condition");
    skip_newlines(parser);

    Stmt *then_branch;
    if (parser_check(parser, TOKEN_INDENT))
    {
        then_branch = parser_indented_block(parser);
    }
    else
    {
        then_branch = parser_statement(parser);
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = then_branch;
            block_stmts[1] = parser_indented_block(parser);
            then_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    Stmt *else_branch = NULL;
    skip_newlines(parser);
    if (parser_match(parser, TOKEN_ELSE))
    {
        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after else");
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            else_branch = parser_indented_block(parser);
        }
        else
        {
            else_branch = parser_statement(parser);
            skip_newlines(parser);
            if (parser_check(parser, TOKEN_INDENT))
            {
                Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
                if (block_stmts == NULL)
                {
                    exit(1);
                }
                block_stmts[0] = else_branch;
                block_stmts[1] = parser_indented_block(parser);
                else_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
            }
        }
    }

    return ast_create_if_stmt(parser->arena, condition, then_branch, else_branch, &if_token);
}

Stmt *parser_while_statement(Parser *parser)
{
    Token while_token = parser->previous;
    Expr *condition = parser_expression(parser);
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after while condition");
    skip_newlines(parser);

    Stmt *body;
    if (parser_check(parser, TOKEN_INDENT))
    {
        body = parser_indented_block(parser);
    }
    else
    {
        body = parser_statement(parser);
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = body;
            block_stmts[1] = parser_indented_block(parser);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    return ast_create_while_stmt(parser->arena, condition, body, &while_token);
}

Stmt *parser_for_statement(Parser *parser)
{
    Token for_token = parser->previous;
    Stmt *initializer = NULL;
    if (parser_match(parser, TOKEN_VAR))
    {
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
                return NULL;
            }
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
                return NULL;
            }
        }
        parser_consume(parser, TOKEN_COLON, "Expected ':' after variable name");
        Type *type = parser_type(parser);
        Expr *init_expr = NULL;
        if (parser_match(parser, TOKEN_EQUAL))
        {
            init_expr = parser_expression(parser);
        }
        initializer = ast_create_var_decl_stmt(parser->arena, name, type, init_expr, &var_token);
    }
    else if (!parser_check(parser, TOKEN_SEMICOLON))
    {
        Expr *init_expr = parser_expression(parser);
        initializer = ast_create_expr_stmt(parser->arena, init_expr, NULL);
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after initializer");

    Expr *condition = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON))
    {
        condition = parser_expression(parser);
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after condition");

    Expr *increment = NULL;
    if (!parser_check(parser, TOKEN_ARROW))
    {
        increment = parser_expression(parser);
    }
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after for clauses");
    skip_newlines(parser);

    Stmt *body;
    if (parser_check(parser, TOKEN_INDENT))
    {
        body = parser_indented_block(parser);
    }
    else
    {
        body = parser_statement(parser);
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = body;
            block_stmts[1] = parser_indented_block(parser);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    return ast_create_for_stmt(parser->arena, initializer, condition, increment, body, &for_token);
}

Stmt *parser_block_statement(Parser *parser)
{
    Token brace = parser->previous;
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    symbol_table_push_scope(parser->symbol_table);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
        if (parser_is_at_end(parser) || parser_check(parser, TOKEN_DEDENT))
            break;

        Stmt *stmt = parser_declaration(parser);
        if (stmt == NULL)
            continue;

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            Stmt **new_statements = arena_alloc(parser->arena, sizeof(Stmt *) * capacity);
            if (new_statements == NULL)
            {
                exit(1);
            }
            if (statements != NULL && count > 0)
            {
                memcpy(new_statements, statements, sizeof(Stmt *) * count);
            }
            statements = new_statements;
        }
        statements[count++] = stmt;
    }

    symbol_table_pop_scope(parser->symbol_table);

    return ast_create_block_stmt(parser->arena, statements, count, &brace);
}

Stmt *parser_expression_statement(Parser *parser)
{
    Expr *expr = parser_expression(parser);

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after expression");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_expr_stmt(parser->arena, expr, &parser->previous);
}

Stmt *parser_import_statement(Parser *parser)
{
    Token import_token = parser->previous;
    Token module_name;
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        module_name = parser->previous;
        module_name.start = arena_strdup(parser->arena, parser->previous.literal.string_value);
        if (module_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
        module_name.length = strlen(module_name.start);
        module_name.line = parser->previous.line;
        module_name.filename = parser->previous.filename;
        module_name.type = TOKEN_STRING_LITERAL;
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
            return NULL;
        }
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after import statement");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_import_stmt(parser->arena, module_name, &import_token);
}

Module *parser_execute(Parser *parser, const char *filename)
{
    Module *module = arena_alloc(parser->arena, sizeof(Module));
    ast_init_module(parser->arena, module, filename);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
        if (parser_is_at_end(parser))
            break;

        Stmt *stmt = parser_declaration(parser);
        if (stmt != NULL)
        {
            ast_module_add_statement(parser->arena, module, stmt);
            ast_print_stmt(parser->arena, stmt, 0);
        }

        if (parser->panic_mode)
        {
            synchronize(parser);
        }
    }

    if (parser->had_error)
    {
        return NULL;
    }

    return module;
}

Module *parse_module_with_imports(Arena *arena, SymbolTable *symbol_table, const char *filename, char ***imported, int *imported_count, int *imported_capacity)
{
    char *source = file_read(arena, filename);
    if (!source)
    {
        DEBUG_ERROR("Failed to read file: %s", filename);
        return NULL;
    }

    Lexer lexer;
    lexer_init(arena, &lexer, source, filename);

    Parser parser;
    parser_init(arena, &parser, &lexer, symbol_table);

    Module *module = parser_execute(&parser, filename);
    if (!module || parser.had_error)
    {
        parser_cleanup(&parser);
        lexer_cleanup(&lexer);
        return NULL;
    }

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
    }

    // Process imports (remove them and prepend imported statements)
    for (int i = 0; i < module->count;)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_IMPORT)
        {
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

            // Check if already imported to avoid cycles/duplicates
            int already_imported = 0;
            for (int j = 0; j < *imported_count; j++)
            {
                if (strcmp((*imported)[j], import_path) == 0)
                {
                    already_imported = 1;
                    break;
                }
            }

            if (already_imported)
            {
                // Remove the import statement
                memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
                module->count--;
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
            }
            (*imported)[(*imported_count)++] = import_path;

            // Recursively parse the imported module
            Module *imported_module = parse_module_with_imports(arena, symbol_table, import_path, imported, imported_count, imported_capacity);
            if (!imported_module)
            {
                parser_cleanup(&parser);
                lexer_cleanup(&lexer);
                return NULL;
            }

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
            }
            memcpy(all_statements + all_count, imported_module->statements, sizeof(Stmt *) * imported_module->count);
            all_count = new_all_count;

            // Remove the import statement
            memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
            module->count--;
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
    }
    memcpy(all_statements + all_count, module->statements, sizeof(Stmt *) * module->count);
    all_count = new_all_count;

    // Update the module with the combined statements
    module->statements = all_statements;
    module->count = all_count;
    module->capacity = all_count; // Capacity doesn't need to be larger

    parser_cleanup(&parser);
    lexer_cleanup(&lexer);
    return module;
}