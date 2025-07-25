#include "parser.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
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
            statements = realloc(statements, sizeof(Stmt *) * capacity);
            if (statements == NULL)
            {
                exit(1);
            }
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

    return ast_create_block_stmt(statements, count);
}

Expr *parser_multi_line_expression(Parser *parser)
{
    Expr *expr = parser_expression(parser);

    while (parser_match(parser, TOKEN_NEWLINE))
    {
        Expr *right = parser_expression(parser);
        expr = ast_create_binary_expr(expr, TOKEN_PLUS, right);
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

void parser_init(Parser *parser, Lexer *lexer)
{
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    parser->symbol_table = symbol_table_init();

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
    symbol_table_cleanup(parser->symbol_table);
    lexer_cleanup(parser->lexer);
    for (int i = 0; i < parser->interp_count; i++)
    {
        free(parser->interp_sources[i]);
    }
    free(parser->interp_sources);
    if (parser->previous.start != NULL)
    {
        free((void *)parser->previous.start);
        parser->previous.start = NULL;
    }
    if (parser->current.start != NULL)
    {
        free((void *)parser->current.start);
        parser->current.start = NULL;
    }
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
}

void parser_advance(Parser *parser)
{
    if (parser->previous.start != NULL)
    {
        free((void *)parser->previous.start);
        parser->previous.start = NULL;
    }

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
    if (parser_match(parser, TOKEN_INT))
        return ast_create_primitive_type(TYPE_INT);
    if (parser_match(parser, TOKEN_LONG))
        return ast_create_primitive_type(TYPE_LONG);
    if (parser_match(parser, TOKEN_DOUBLE))
        return ast_create_primitive_type(TYPE_DOUBLE);
    if (parser_match(parser, TOKEN_CHAR))
        return ast_create_primitive_type(TYPE_CHAR);
    if (parser_match(parser, TOKEN_STR))
        return ast_create_primitive_type(TYPE_STRING);
    if (parser_match(parser, TOKEN_BOOL))
        return ast_create_primitive_type(TYPE_BOOL);
    if (parser_match(parser, TOKEN_VOID))
        return ast_create_primitive_type(TYPE_VOID);

    parser_error_at_current(parser, "Expected type");
    return ast_create_primitive_type(TYPE_NIL);
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
        Expr *value = parser_assignment(parser);
        if (expr->type == EXPR_VARIABLE)
        {
            Token name = expr->as.variable.name;
            char *new_start = strndup(name.start, name.length);
            if (new_start == NULL)
            {
                exit(1);
            }
            ast_free_expr(expr);
            name.start = new_start;
            return ast_create_assign_expr(name, value);
        }
        parser_error(parser, "Invalid assignment target");
        ast_free_expr(value);
    }
    return expr;
}

Expr *parser_logical_or(Parser *parser)
{
    Expr *expr = parser_logical_and(parser);
    while (parser_match(parser, TOKEN_OR))
    {
        TokenType operator = parser->previous.type;
        Expr *right = parser_logical_and(parser);
        expr = ast_create_binary_expr(expr, operator, right);
    }
    return expr;
}

Expr *parser_logical_and(Parser *parser)
{
    Expr *expr = parser_equality(parser);
    while (parser_match(parser, TOKEN_AND))
    {
        TokenType operator = parser->previous.type;
        Expr *right = parser_equality(parser);
        expr = ast_create_binary_expr(expr, operator, right);
    }
    return expr;
}

Expr *parser_equality(Parser *parser)
{
    Expr *expr = parser_comparison(parser);
    while (parser_match(parser, TOKEN_BANG_EQUAL) || parser_match(parser, TOKEN_EQUAL_EQUAL))
    {
        TokenType operator = parser->previous.type;
        Expr *right = parser_comparison(parser);
        expr = ast_create_binary_expr(expr, operator, right);
    }
    return expr;
}

Expr *parser_comparison(Parser *parser)
{
    Expr *expr = parser_term(parser);
    while (parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_LESS_EQUAL) ||
           parser_match(parser, TOKEN_GREATER) || parser_match(parser, TOKEN_GREATER_EQUAL))
    {
        TokenType operator = parser->previous.type;
        Expr *right = parser_term(parser);
        expr = ast_create_binary_expr(expr, operator, right);
    }
    return expr;
}

Expr *parser_term(Parser *parser)
{
    Expr *expr = parser_factor(parser);
    while (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS))
    {
        TokenType operator = parser->previous.type;
        Expr *right = parser_factor(parser);
        expr = ast_create_binary_expr(expr, operator, right);
    }
    return expr;
}

Expr *parser_factor(Parser *parser)
{
    Expr *expr = parser_unary(parser);
    while (parser_match(parser, TOKEN_STAR) || parser_match(parser, TOKEN_SLASH) || parser_match(parser, TOKEN_MODULO))
    {
        TokenType operator = parser->previous.type;
        Expr *right = parser_unary(parser);
        expr = ast_create_binary_expr(expr, operator, right);
    }
    return expr;
}

Expr *parser_unary(Parser *parser)
{
    if (parser_match(parser, TOKEN_BANG) || parser_match(parser, TOKEN_MINUS))
    {
        TokenType operator = parser->previous.type;
        Expr *right = parser_unary(parser);
        return ast_create_unary_expr(operator, right);
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
            expr = ast_create_increment_expr(expr);
        }
        else if (parser_match(parser, TOKEN_MINUS_MINUS))
        {
            expr = ast_create_decrement_expr(expr);
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
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_INT), false);
    }
    if (parser_match(parser, TOKEN_LONG_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_LONG), false);
    }
    if (parser_match(parser, TOKEN_DOUBLE_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_DOUBLE), false);
    }
    if (parser_match(parser, TOKEN_CHAR_LITERAL))
    {
        LiteralValue value;
        value.char_value = parser->previous.literal.char_value;
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_CHAR), false);
    }
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        LiteralValue value;
        value.string_value = parser->previous.literal.string_value;
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_STRING), false);
    }
    if (parser_match(parser, TOKEN_BOOL_LITERAL))
    {
        LiteralValue value;
        value.bool_value = parser->previous.literal.bool_value;
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_BOOL), false);
    }
    if (parser_match(parser, TOKEN_NIL))
    {
        LiteralValue value;
        value.int_value = 0;
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_NIL), false);
    }
    if (parser_match(parser, TOKEN_IDENTIFIER))
    {
        return ast_create_variable_expr(parser->previous);
    }
    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        Expr *expr = parser_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }

    if (parser_match(parser, TOKEN_INTERPOL_STRING))
    {
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
                    char *seg = malloc(len + 1);
                    strncpy(seg, segment_start, len);
                    seg[len] = '\0';
                    LiteralValue v;
                    v.string_value = seg;
                    Expr *seg_expr = ast_create_literal_expr(v, ast_create_primitive_type(TYPE_STRING), false);

                    if (count >= capacity)
                    {
                        capacity = capacity == 0 ? 8 : capacity * 2;
                        parts = realloc(parts, sizeof(Expr *) * capacity);
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
                    for (int j = 0; j < count; j++)
                        ast_free_expr(parts[j]);
                    free(parts);
                    free((char *)content);
                    return ast_create_literal_expr((LiteralValue){0}, ast_create_primitive_type(TYPE_STRING), false);
                }
                int expr_len = p - expr_start;
                char *expr_src = malloc(expr_len + 1);
                strncpy(expr_src, expr_start, expr_len);
                expr_src[expr_len] = '\0';

                Lexer sub_lexer;
                lexer_init(&sub_lexer, expr_src, "interpolated");
                Parser sub_parser;
                parser_init(&sub_parser, &sub_lexer);
                SymbolTable *unused_table = sub_parser.symbol_table;
                sub_parser.symbol_table = parser->symbol_table;

                symbol_table_cleanup(unused_table);

                Expr *inner = parser_expression(&sub_parser);
                if (sub_parser.previous.start != NULL)
                {
                    free((void *)sub_parser.previous.start);
                    sub_parser.previous.start = NULL;
                }
                if (sub_parser.current.start != NULL)
                {
                    free((void *)sub_parser.current.start);
                    sub_parser.current.start = NULL;
                }
                if (inner == NULL || sub_parser.had_error)
                {
                    parser_error_at_current(parser, "Invalid expression in interpolation");
                    ast_free_expr(inner);
                    free(expr_src);
                    lexer_cleanup(&sub_lexer);
                    for (int j = 0; j < count; j++)
                        ast_free_expr(parts[j]);
                    free(parts);
                    free((char *)content);
                    return ast_create_literal_expr((LiteralValue){0}, ast_create_primitive_type(TYPE_STRING), false);
                }

                if (count >= capacity)
                {
                    capacity = capacity == 0 ? 8 : capacity * 2;
                    parts = realloc(parts, sizeof(Expr *) * capacity);
                }
                parts[count++] = inner;

                if (parser->interp_count >= parser->interp_capacity)
                {
                    parser->interp_capacity = parser->interp_capacity ? parser->interp_capacity * 2 : 8;
                    parser->interp_sources = realloc(parser->interp_sources, sizeof(char *) * parser->interp_capacity);
                    if (parser->interp_sources == NULL)
                    {
                        exit(1);
                    }
                }
                parser->interp_sources[parser->interp_count++] = expr_src;

                lexer_cleanup(&sub_lexer);

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
            char *seg = malloc(len + 1);
            strncpy(seg, segment_start, len);
            seg[len] = '\0';
            LiteralValue v;
            v.string_value = seg;
            Expr *seg_expr = ast_create_literal_expr(v, ast_create_primitive_type(TYPE_STRING), false);

            if (count >= capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                parts = realloc(parts, sizeof(Expr *) * capacity);
            }
            parts[count++] = seg_expr;
        }

        free((char *)content);

        return ast_create_interpolated_expr(parts, count);
    }

    parser_error_at_current(parser, "Expected expression");
    LiteralValue value;
    value.int_value = 0;
    return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_NIL), false);
}

Expr *parser_call(Parser *parser, Expr *callee)
{
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
                arguments = realloc(arguments, sizeof(Expr *) * capacity);
                if (arguments == NULL)
                {
                    exit(1);
                }
            }
            arguments[arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
    return ast_create_call_expr(callee, arguments, arg_count);
}

Expr *parser_array_access(Parser *parser, Expr *array)
{
    Expr *index = parser_expression(parser);
    parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index");
    return ast_create_array_access_expr(array, index);
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
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = strndup(name.start, name.length);
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
        name.start = strndup(name.start, name.length);
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

    Stmt *decl = ast_create_var_decl_stmt(name, type, initializer);
    free((void *)name.start);
    return decl;
}

Stmt *parser_function_declaration(Parser *parser)
{
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
        name.start = strndup(name.start, name.length);
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
        name.start = strndup(name.start, name.length);
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
                    param_name.start = strndup(param_name.start, param_name.length);
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
                    param_name.start = strndup(param_name.start, param_name.length);
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
                    params = realloc(params, sizeof(Parameter) * param_capacity);
                    if (params == NULL)
                    {
                        exit(1);
                    }
                }
                params[param_count].name = param_name;
                params[param_count].type = param_type;
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }

    Type *return_type = ast_create_primitive_type(TYPE_VOID);
    if (parser_match(parser, TOKEN_COLON))
    {
        ast_free_type(return_type);
        return_type = parser_type(parser);
    }

    Type **param_types = malloc(sizeof(Type *) * param_count);
    if (param_types == NULL && param_count > 0)
    {
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
    {
        param_types[i] = params[i].type;
    }
    Type *function_type = ast_create_function_type(return_type, param_types, param_count);
    free(param_types);

    symbol_table_add_symbol(parser->symbol_table, name, function_type);
    ast_free_type(function_type);

    parser_consume(parser, TOKEN_ARROW, "Expected '=>' before function body");
    skip_newlines(parser);

    Stmt *body = parser_indented_block(parser);
    if (body == NULL)
    {
        body = ast_create_block_stmt(NULL, 0);
    }

    Stmt **stmts = body->as.block.statements;
    int stmt_count = body->as.block.count;
    body->as.block.statements = NULL;
    ast_free_stmt(body);

    Stmt *func_stmt = ast_create_function_stmt(name, params, param_count, return_type,
                                               stmts, stmt_count);

    free((void *)name.start);
    for (int i = 0; i < param_count; i++)
    {
        free((void *)params[i].name.start);
    }
    free(params);
    return func_stmt;
}

Stmt *parser_return_statement(Parser *parser)
{
    Token keyword = parser->previous;
    Expr *value = NULL;

    keyword.start = strndup(keyword.start, keyword.length);
    if (keyword.start == NULL)
    {
        parser_error_at_current(parser, "Out of memory");
        return NULL;
    }

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

    return ast_create_return_stmt(keyword, value);
}

Stmt *parser_if_statement(Parser *parser)
{
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
            Stmt **block_stmts = malloc(sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = then_branch;
            block_stmts[1] = parser_indented_block(parser);
            then_branch = ast_create_block_stmt(block_stmts, 2);
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
                Stmt **block_stmts = malloc(sizeof(Stmt *) * 2);
                if (block_stmts == NULL)
                {
                    exit(1);
                }
                block_stmts[0] = else_branch;
                block_stmts[1] = parser_indented_block(parser);
                else_branch = ast_create_block_stmt(block_stmts, 2);
            }
        }
    }

    return ast_create_if_stmt(condition, then_branch, else_branch);
}

Stmt *parser_while_statement(Parser *parser)
{
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
            Stmt **block_stmts = malloc(sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = body;
            block_stmts[1] = parser_indented_block(parser);
            body = ast_create_block_stmt(block_stmts, 2);
        }
    }

    return ast_create_while_stmt(condition, body);
}

Stmt *parser_for_statement(Parser *parser)
{
    Stmt *initializer = NULL;
    if (parser_match(parser, TOKEN_VAR))
    {
        Token name;
        if (parser_check(parser, TOKEN_IDENTIFIER))
        {
            name = parser->current;
            parser_advance(parser);
            name.start = strndup(name.start, name.length);
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
            name.start = strndup(name.start, name.length);
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
        initializer = ast_create_var_decl_stmt(name, type, init_expr);
        free((void *)name.start);
    }
    else if (!parser_check(parser, TOKEN_SEMICOLON))
    {
        Expr *init_expr = parser_expression(parser);
        initializer = ast_create_expr_stmt(init_expr);
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
            Stmt **block_stmts = malloc(sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = body;
            block_stmts[1] = parser_indented_block(parser);
            body = ast_create_block_stmt(block_stmts, 2);
        }
    }

    return ast_create_for_stmt(initializer, condition, increment, body);
}

Stmt *parser_block_statement(Parser *parser)
{
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
            statements = realloc(statements, sizeof(Stmt *) * capacity);
            if (statements == NULL)
            {
                exit(1);
            }
        }
        statements[count++] = stmt;
    }

    symbol_table_pop_scope(parser->symbol_table);

    return ast_create_block_stmt(statements, count);
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

    return ast_create_expr_stmt(expr);
}

Stmt *parser_import_statement(Parser *parser)
{
    Token module_name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        module_name = parser->current;
        parser_advance(parser);
        module_name.start = strndup(module_name.start, module_name.length);
        if (module_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected module name");
        module_name = parser->current;
        parser_advance(parser);
        module_name.start = strndup(module_name.start, module_name.length);
        if (module_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }

    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after import statement");
    Stmt *import_stmt = ast_create_import_stmt(module_name);
    free((void *)module_name.start);
    return import_stmt;
}

Module *parser_execute(Parser *parser, const char *filename)
{
    Module *module = malloc(sizeof(Module));
    ast_init_module(module, filename);

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
        }
        ast_module_add_statement(module, stmt);
        ast_print_stmt(stmt, 0);

        if (parser->panic_mode)
        {
            synchronize(parser);
        }
    }

    if (parser->had_error)
    {
        ast_free_module(module);
        free(module);
        return NULL;
    }

    return module;
}