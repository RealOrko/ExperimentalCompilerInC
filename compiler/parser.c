/**
 * parser.c
 * Implementation of the parser
 */

#include "parser.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to log parser state
static void log_parser_state(Parser *parser)
{
    DEBUG_VERBOSE("Current token: %s, Previous token: %s",
                  token_type_to_string(parser->current.type),
                  token_type_to_string(parser->previous.type));
}

// Check if we've reached the end of the file
int parser_is_at_end(Parser *parser)
{
    return parser->current.type == TOKEN_EOF;
}

// Function to skip over any newlines
void skip_newlines(Parser *parser)
{
    // Skip newlines but capture INDENT/DEDENT tokens
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        DEBUG_VERBOSE("Skipping NEWLINE");
        // Check for INDENT/DEDENT after newline
        if (parser_check(parser, TOKEN_INDENT) || parser_check(parser, TOKEN_DEDENT))
        {
            // Don't skip INDENT/DEDENT - let the caller handle it
            break;
        }
    }
}

// Helper for detecting function boundaries
int is_at_function_boundary(Parser *parser)
{
    if (parser_check(parser, TOKEN_DEDENT))
    {
        DEBUG_VERBOSE("At function boundary due to DEDENT");
        return 1;
    }
    if (parser_check(parser, TOKEN_FN))
    {
        DEBUG_VERBOSE("At function boundary due to FN token");
        return 1;
    }
    if (parser_check(parser, TOKEN_EOF))
    {
        DEBUG_VERBOSE("At function boundary due to EOF");
        return 1;
    }
    return 0;
}

Stmt *parser_indented_block(Parser *parser)
{
    DEBUG_VERBOSE("Entering parse_indented_block");
    if (!parser_check(parser, TOKEN_INDENT))
    {
        parser_error(parser, "Expected indented block");
        return NULL;
    }
    parser_advance(parser); // Consume INDENT
    DEBUG_VERBOSE("Consumed INDENT, entering block with indent_stack top %d",
                  parser->lexer->indent_stack[parser->lexer->indent_size - 1]);

    int current_indent = parser->lexer->indent_stack[parser->lexer->indent_size - 1];
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    DEBUG_VERBOSE("Pushing new scope for block");
    symbol_table_push_scope(parser->symbol_table);

    while (!parser_is_at_end(parser) &&
           parser->lexer->indent_stack[parser->lexer->indent_size - 1] >= current_indent)
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            DEBUG_VERBOSE("Skipping NEWLINE in block");
        }

        if (parser_check(parser, TOKEN_DEDENT))
        {
            DEBUG_VERBOSE("Found DEDENT in block");
            break;
        }

        if (parser_check(parser, TOKEN_EOF))
        {
            DEBUG_VERBOSE("Found EOF in block");
            break;
        }

        DEBUG_VERBOSE("Parsing statement in block, current indent level = %d",
                      parser->lexer->indent_stack[parser->lexer->indent_size - 1]);
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
                DEBUG_ERROR("Out of memory");
                exit(1);
            }
        }
        statements[count++] = stmt;
        DEBUG_VERBOSE("Added statement %d to block, type = %d", count - 1, stmt->type);
    }

    // Consume DEDENT if present
    if (parser_check(parser, TOKEN_DEDENT))
    {
        parser_advance(parser);
        DEBUG_VERBOSE("Consumed DEDENT, exiting block");
    }
    else if (parser->lexer->indent_stack[parser->lexer->indent_size - 1] < current_indent)
    {
        parser_error(parser, "Expected dedent to end block");
    }

    DEBUG_VERBOSE("Popping scope for block");
    symbol_table_pop_scope(parser->symbol_table);

    DEBUG_VERBOSE("Exiting parse_indented_block with %d statements", count);
    return ast_create_block_stmt(statements, count);
}

Expr *parser_multi_line_expression(Parser *parser)
{
    DEBUG_VERBOSE("Entering parse_multi_line_expression");
    Expr *expr = parser_expression(parser);

    while (parser_match(parser, TOKEN_NEWLINE))
    {
        DEBUG_VERBOSE("Continuing expression across newline");
        Expr *right = parser_expression(parser);
        expr = ast_create_binary_expr(expr, TOKEN_PLUS, right);
    }

    DEBUG_VERBOSE("Exiting parse_multi_line_expression");
    return expr;
}

// Skip all newlines and check if we're at the end
int skip_newlines_and_check_end(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        DEBUG_VERBOSE("Skipping NEWLINE in skip_newlines_and_check_end");
    }
    return parser_is_at_end(parser);
}

void parser_init(Parser *parser, Lexer *lexer)
{
    DEBUG_VERBOSE("Initializing parser");
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    parser->symbol_table = symbol_table_init();

    // Initialize tokens to avoid uninit reads
    parser->previous.type = TOKEN_ERROR; // Safe invalid type
    parser->previous.start = NULL;
    parser->previous.length = 0;
    parser->previous.line = 0;
    parser->current = parser->previous; // Copy to current

    parser_advance(parser);
}

void parser_cleanup(Parser *parser)
{
    DEBUG_VERBOSE("Cleaning up parser");
    symbol_table_cleanup(parser->symbol_table);
    lexer_cleanup(parser->lexer);
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
        // Nothing
    }
    else
    {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);

    parser->lexer->indent_size = 1;
    DEBUG_VERBOSE("Error reported: %s", message);
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
    DEBUG_VERBOSE("Advanced to token: %s", token_type_to_string(parser->current.type));
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
    DEBUG_VERBOSE("Matched token: %s", token_type_to_string(type));
    return 1;
}

static void synchronize(Parser *parser)
{
    DEBUG_VERBOSE("Synchronizing parser after error");
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
            return; // Found a new statement, stop synchronizing
        case TOKEN_NEWLINE:
            parser_advance(parser); // Skip newlines and unknown tokens
            break;
        default:
            parser_advance(parser); // Skip other tokens
            break;
        }
    }
}

Type *parser_type(Parser *parser)
{
    DEBUG_VERBOSE("Parsing type");
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
    DEBUG_VERBOSE("Entering parse_expression");
    Expr *result = parser_assignment(parser);
    if (result == NULL)
    {
        parser_error_at_current(parser, "Expected expression");
        parser_advance(parser); // Advance after error to prevent looping
    }
    DEBUG_VERBOSE("Exiting parse_expression");
    return result;
}

Expr *parser_assignment(Parser *parser)
{
    Expr *expr = parser_logical_or(parser);

    if (parser_match(parser, TOKEN_EQUAL))
    {
        DEBUG_VERBOSE("Parsing assignment");
        Expr *value = parser_assignment(parser);
        if (expr->type == EXPR_VARIABLE)
        {
            Token name = expr->as.variable.name;
            ast_free_expr(expr);
            return ast_create_assign_expr(name, value);
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
        DEBUG_VERBOSE("Parsed integer literal: %d", (int)value.int_value);
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_INT), false);
    }
    if (parser_match(parser, TOKEN_LONG_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        DEBUG_VERBOSE("Parsed long literal: %ld", value.int_value);
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_LONG), false);
    }
    if (parser_match(parser, TOKEN_DOUBLE_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        DEBUG_VERBOSE("Parsed double literal: %f", value.double_value);
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_DOUBLE), false);
    }
    if (parser_match(parser, TOKEN_CHAR_LITERAL))
    {
        LiteralValue value;
        value.char_value = parser->previous.literal.char_value;
        DEBUG_VERBOSE("Parsed char literal: %c", value.char_value);
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_CHAR), false);
    }
    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        LiteralValue value;
        value.string_value = parser->previous.literal.string_value;
        DEBUG_VERBOSE("Parsed string literal: %s", value.string_value);
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_STRING), false);
    }
    if (parser_match(parser, TOKEN_INTERPOL_STRING))
    {
        LiteralValue value;
        value.string_value = parser->previous.literal.string_value;
        DEBUG_VERBOSE("Parsed interpolated string literal: %s", value.string_value);
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_STRING), true);
    }
    if (parser_match(parser, TOKEN_BOOL_LITERAL))
    {
        LiteralValue value;
        value.bool_value = parser->previous.literal.bool_value;
        DEBUG_VERBOSE("Parsed bool literal: %s", value.bool_value ? "true" : "false");
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_BOOL), false);
    }
    if (parser_match(parser, TOKEN_NIL))
    {
        LiteralValue value;
        value.int_value = 0;
        DEBUG_VERBOSE("Parsed nil");
        return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_NIL), false);
    }
    if (parser_match(parser, TOKEN_IDENTIFIER))
    {
        DEBUG_VERBOSE("Parsed variable: %.*s", parser->previous.length, parser->previous.start);
        return ast_create_variable_expr(parser->previous);
    }
    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        Expr *expr = parser_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }

    parser_error_at_current(parser, "Expected expression");
    LiteralValue value;
    value.int_value = 0;
    return ast_create_literal_expr(value, ast_create_primitive_type(TYPE_NIL), false);
}

Expr *parser_call(Parser *parser, Expr *callee)
{
    DEBUG_VERBOSE("Parsing function call");
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
                    DEBUG_ERROR("Out of memory");
                    exit(1);
                }
            }
            arguments[arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");
    DEBUG_VERBOSE("Parsed call with %d arguments", arg_count);
    return ast_create_call_expr(callee, arguments, arg_count);
}

Expr *parser_array_access(Parser *parser, Expr *array)
{
    DEBUG_VERBOSE("Parsing array access");
    Expr *index = parser_expression(parser);
    parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index");
    return ast_create_array_access_expr(array, index);
}

Stmt *parser_statement(Parser *parser)
{
    log_parser_state(parser);
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        DEBUG_VERBOSE("Skipping NEWLINE before statement");
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        DEBUG_VERBOSE("Parsing var declaration");
        return parser_var_declaration(parser);
    }
    if (parser_match(parser, TOKEN_IF))
    {
        DEBUG_VERBOSE("Parsing if statement");
        return parser_if_statement(parser);
    }
    if (parser_match(parser, TOKEN_WHILE))
    {
        DEBUG_VERBOSE("Parsing while statement");
        return parser_while_statement(parser);
    }
    if (parser_match(parser, TOKEN_FOR))
    {
        DEBUG_VERBOSE("Parsing for statement");
        return parser_for_statement(parser);
    }
    if (parser_match(parser, TOKEN_RETURN))
    {
        DEBUG_VERBOSE("Parsing return statement");
        return parser_return_statement(parser);
    }
    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        DEBUG_VERBOSE("Parsing block statement");
        return parser_block_statement(parser);
    }

    DEBUG_VERBOSE("Parsing expression statement");
    return parser_expression_statement(parser);
}

Stmt *parser_declaration(Parser *parser)
{
    log_parser_state(parser);
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        DEBUG_VERBOSE("Skipping NEWLINE before declaration");
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        DEBUG_VERBOSE("Parsing var declaration");
        return parser_var_declaration(parser);
    }
    if (parser_match(parser, TOKEN_FN))
    {
        DEBUG_VERBOSE("Parsing function declaration");
        return parser_function_declaration(parser);
    }
    if (parser_match(parser, TOKEN_IMPORT))
    {
        DEBUG_VERBOSE("Parsing import statement");
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
    }
    else
    {
        parser_error_at_current(parser, "Expected variable name");
        name = parser->current;
        parser_advance(parser);
    }

    parser_consume(parser, TOKEN_COLON, "Expected ':' after variable name");
    Type *type = parser_type(parser);

    Expr *initializer = NULL;
    if (parser_match(parser, TOKEN_EQUAL))
    {
        DEBUG_VERBOSE("Parsing initializer for variable %.*s", name.length, name.start);
        initializer = parser_expression(parser);
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after variable declaration");
    }

    DEBUG_VERBOSE("Declared variable %.*s", name.length, name.start);
    return ast_create_var_decl_stmt(name, type, initializer);
}

Stmt *parser_function_declaration(Parser *parser)
{
    Token name;
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
    }
    else
    {
        parser_error_at_current(parser, "Expected function name");
        name = parser->current;
        parser_advance(parser);
    }
    DEBUG_VERBOSE("Declaring function %.*s", name.length, name.start);

    // Parameter parsing
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
                }
                else
                {
                    parser_error_at_current(parser, "Expected parameter name");
                    param_name = parser->current;
                    parser_advance(parser);
                }
                parser_consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
                Type *param_type = parser_type(parser);

                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                    params = realloc(params, sizeof(Parameter) * param_capacity);
                    if (params == NULL)
                    {
                        DEBUG_ERROR("Out of memory");
                        exit(1);
                    }
                }
                params[param_count].name = param_name;
                params[param_count].type = param_type;
                param_count++;
                DEBUG_VERBOSE("Added parameter %d: %.*s", param_count, param_name.length, param_name.start);
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }
    DEBUG_VERBOSE("Function %.*s has %d parameters", name.length, name.start, param_count);

    Type *return_type = ast_create_primitive_type(TYPE_VOID);
    if (parser_match(parser, TOKEN_COLON))
    {
        ast_free_type(return_type); // Free default if overridden
        return_type = parser_type(parser);
    }

    Type **param_types = malloc(sizeof(Type *) * param_count);
    if (param_types == NULL && param_count > 0)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
    {
        param_types[i] = params[i].type;
    }
    Type *function_type = ast_create_function_type(return_type, param_types, param_count);
    free(param_types); // Free temp array after cloning in function_type

    symbol_table_add_symbol(parser->symbol_table, name, function_type);
    ast_free_type(function_type); // Free after adding (cloned in symbol table)

    DEBUG_VERBOSE("Beginning function scope for %.*s", name.length, name.start);
    symbol_table_begin_function_scope(parser->symbol_table);

    for (int i = 0; i < param_count; i++)
    {
        symbol_table_add_symbol_with_kind(parser->symbol_table, params[i].name, params[i].type, SYMBOL_PARAM);
    }

    parser_consume(parser, TOKEN_ARROW, "Expected '=>' before function body");
    skip_newlines(parser);

    DEBUG_VERBOSE("Parsing function body for %.*s", name.length, name.start);
    Stmt *body = parser_indented_block(parser);
    symbol_table_pop_scope(parser->symbol_table);
    if (body == NULL)
    {
        body = ast_create_block_stmt(NULL, 0); // Empty block as fallback
    }

    // Extract statements from block and free the block struct (without freeing array)
    Stmt **stmts = body->as.block.statements;
    int stmt_count = body->as.block.count;
    body->as.block.statements = NULL; // Prevent array free in ast_free_stmt
    ast_free_stmt(body);

    DEBUG_VERBOSE("Finished parsing function body");

    Stmt *func_stmt = ast_create_function_stmt(name, params, param_count, return_type,
                                               stmts, stmt_count);

    return func_stmt;
}

Stmt *parser_return_statement(Parser *parser)
{
    Token keyword = parser->previous;
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
        // Semicolon consumed
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
                DEBUG_ERROR("Out of memory");
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
                    DEBUG_ERROR("Out of memory");
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
                DEBUG_ERROR("Out of memory");
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
        }
        else
        {
            parser_error_at_current(parser, "Expected variable name");
            name = parser->current;
            parser_advance(parser);
        }
        parser_consume(parser, TOKEN_COLON, "Expected ':' after variable name");
        Type *type = parser_type(parser);
        Expr *init_expr = NULL;
        if (parser_match(parser, TOKEN_EQUAL))
        {
            init_expr = parser_expression(parser);
        }
        initializer = ast_create_var_decl_stmt(name, type, init_expr);
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
                DEBUG_ERROR("Out of memory");
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

    DEBUG_VERBOSE("Pushing new scope for block statement");
    symbol_table_push_scope(parser->symbol_table);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            DEBUG_VERBOSE("Skipping NEWLINE in block statement");
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
                DEBUG_ERROR("Out of memory");
                exit(1);
            }
        }
        statements[count++] = stmt;
    }

    DEBUG_VERBOSE("Popping scope for block statement");
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
        // Semicolon consumed
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
    }
    else
    {
        parser_error_at_current(parser, "Expected module name");
        module_name = parser->current;
        parser_advance(parser);
    }
    DEBUG_VERBOSE("Parsed import: %.*s", module_name.length, module_name.start);

    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after import statement");
    return ast_create_import_stmt(module_name);
}

Module *parser_execute(Parser *parser, const char *filename)
{
    DEBUG_VERBOSE("Starting parse for file: %s", filename);
    Module *module = malloc(sizeof(Module));
    ast_init_module(module, filename);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            DEBUG_VERBOSE("Skipping NEWLINE in main loop");
        }
        if (parser_is_at_end(parser))
            break;

        log_parser_state(parser);
        Stmt *stmt = parser_declaration(parser);
        if (stmt != NULL)
        {
            DEBUG_VERBOSE("Added statement of type %d to module", stmt->type);
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

    DEBUG_VERBOSE("Parsing completed successfully");
    return module;
}