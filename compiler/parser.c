/**
 * parser.c
 * Implementation of the parser
 */

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to skip over any newlines
void skip_newlines(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        // Just consume the newlines
    }
}

// Check if we've reached the end of the file
int parser_is_at_end(Parser *parser)
{
    return parser->current.type == TOKEN_EOF;
}

// Skip all newlines and check if we're at the end
int skip_newlines_and_check_end(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        // Just consume the newlines
    }
    return parser_is_at_end(parser);
}

void init_parser(Parser *parser, Lexer *lexer)
{
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    parser->symbol_table = create_symbol_table();

    // Prime the parser with the first token
    parser_advance(parser);
}

void parser_cleanup(Parser *parser)
{
    free_symbol_table(parser->symbol_table);
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
}

void parser_advance(Parser *parser)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = scan_token(parser->lexer);
        if (parser->current.type != TOKEN_ERROR)
            break;

        parser_error_at_current(parser, parser->current.start);
    }
}

void consume(Parser *parser, TokenType type, const char *message)
{
    if (parser->current.type == type)
    {
        parser_advance(parser);
        return;
    }

    parser_error_at_current(parser, message);
}

int check(Parser *parser, TokenType type)
{
    return parser->current.type == type;
}

int parser_match(Parser *parser, TokenType type)
{
    if (!check(parser, type))
        return 0;
    parser_advance(parser);
    return 1;
}

static void synchronize(Parser *parser)
{
    parser->panic_mode = 0;

    while (!parser_is_at_end(parser))
    {
        if (parser->previous.type == TOKEN_SEMICOLON ||
            parser->previous.type == TOKEN_NEWLINE)
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

        default:
            // Do nothing
            ;
        }

        parser_advance(parser);
    }
}

// Parse a type
Type *parse_type(Parser *parser)
{
    if (parser_match(parser, TOKEN_INT))
    {
        return create_primitive_type(TYPE_INT);
    }
    else if (parser_match(parser, TOKEN_LONG))
    {
        return create_primitive_type(TYPE_LONG);
    }
    else if (parser_match(parser, TOKEN_DOUBLE))
    {
        return create_primitive_type(TYPE_DOUBLE);
    }
    else if (parser_match(parser, TOKEN_CHAR))
    {
        return create_primitive_type(TYPE_CHAR);
    }
    else if (parser_match(parser, TOKEN_STR))
    {
        return create_primitive_type(TYPE_STRING);
    }
    else if (parser_match(parser, TOKEN_BOOL))
    {
        return create_primitive_type(TYPE_BOOL);
    }
    else if (parser_match(parser, TOKEN_VOID))
    {
        return create_primitive_type(TYPE_VOID);
    }
    else
    {
        parser_error_at_current(parser, "Expected type");
        return create_primitive_type(TYPE_NIL); // Error recovery
    }
}

// Parse expressions

Expr *parse_expression(Parser *parser)
{
    return parse_assignment(parser);
}

Expr *parse_assignment(Parser *parser)
{
    Expr *expr = parse_logical_or(parser);

    if (parser_match(parser, TOKEN_EQUAL))
    {
        Expr *value = parse_assignment(parser);

        if (expr->type == EXPR_VARIABLE)
        {
            Token name = expr->as.variable.name;
            free_expr(expr);
            return create_assign_expr(name, value);
        }

        parser_error(parser, "Invalid assignment target");
    }

    return expr;
}

Expr *parse_logical_or(Parser *parser)
{
    Expr *expr = parse_logical_and(parser);

    while (parser_match(parser, TOKEN_OR))
    {
        TokenType operator= parser->previous.type;
        Expr *right = parse_logical_and(parser);
        expr = create_binary_expr(expr, operator, right);
    }

    return expr;
}

Expr *parse_logical_and(Parser *parser)
{
    Expr *expr = parse_equality(parser);

    while (parser_match(parser, TOKEN_AND))
    {
        TokenType operator= parser->previous.type;
        Expr *right = parse_equality(parser);
        expr = create_binary_expr(expr, operator, right);
    }

    return expr;
}

Expr *parse_equality(Parser *parser)
{
    Expr *expr = parse_comparison(parser);

    while (parser_match(parser, TOKEN_BANG_EQUAL) || parser_match(parser, TOKEN_EQUAL_EQUAL))
    {
        TokenType operator= parser->previous.type;
        Expr *right = parse_comparison(parser);
        expr = create_binary_expr(expr, operator, right);
    }

    return expr;
}

Expr *parse_comparison(Parser *parser)
{
    Expr *expr = parse_term(parser);

    while (parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_LESS_EQUAL) ||
           parser_match(parser, TOKEN_GREATER) || parser_match(parser, TOKEN_GREATER_EQUAL))
    {
        TokenType operator= parser->previous.type;
        Expr *right = parse_term(parser);
        expr = create_binary_expr(expr, operator, right);
    }

    return expr;
}

Expr *parse_term(Parser *parser)
{
    Expr *expr = parse_factor(parser);

    while (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS))
    {
        TokenType operator= parser->previous.type;
        Expr *right = parse_factor(parser);
        expr = create_binary_expr(expr, operator, right);
    }

    return expr;
}

Expr *parse_factor(Parser *parser)
{
    Expr *expr = parse_unary(parser);

    while (parser_match(parser, TOKEN_STAR) || parser_match(parser, TOKEN_SLASH) || parser_match(parser, TOKEN_MODULO))
    {
        TokenType operator= parser->previous.type;
        Expr *right = parse_unary(parser);
        expr = create_binary_expr(expr, operator, right);
    }

    return expr;
}

Expr *parse_unary(Parser *parser)
{
    if (parser_match(parser, TOKEN_BANG) || parser_match(parser, TOKEN_MINUS))
    {
        TokenType operator= parser->previous.type;
        Expr *right = parse_unary(parser);
        return create_unary_expr(operator, right);
    }

    return parse_postfix(parser);
}

Expr *parse_postfix(Parser *parser)
{
    Expr *expr = parse_primary(parser);

    for (;;)
    {
        if (parser_match(parser, TOKEN_LEFT_PAREN))
        {
            expr = parse_call(parser, expr);
        }
        else if (parser_match(parser, TOKEN_LEFT_BRACKET))
        {
            expr = parse_array_access(parser, expr);
        }
        else if (parser_match(parser, TOKEN_PLUS_PLUS))
        {
            expr = create_increment_expr(expr);
        }
        else if (parser_match(parser, TOKEN_MINUS_MINUS))
        {
            expr = create_decrement_expr(expr);
        }
        else
        {
            break;
        }
    }

    return expr;
}

Expr *parse_primary(Parser *parser)
{
    if (parser_match(parser, TOKEN_INT_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return create_literal_expr(value, create_primitive_type(TYPE_INT));
    }

    if (parser_match(parser, TOKEN_LONG_LITERAL))
    {
        LiteralValue value;
        value.int_value = parser->previous.literal.int_value;
        return create_literal_expr(value, create_primitive_type(TYPE_LONG));
    }

    if (parser_match(parser, TOKEN_DOUBLE_LITERAL))
    {
        LiteralValue value;
        value.double_value = parser->previous.literal.double_value;
        return create_literal_expr(value, create_primitive_type(TYPE_DOUBLE));
    }

    if (parser_match(parser, TOKEN_CHAR_LITERAL))
    {
        LiteralValue value;
        value.char_value = parser->previous.literal.char_value;
        return create_literal_expr(value, create_primitive_type(TYPE_CHAR));
    }

    if (parser_match(parser, TOKEN_STRING_LITERAL))
    {
        LiteralValue value;
        value.string_value = parser->previous.literal.string_value;
        return create_literal_expr(value, create_primitive_type(TYPE_STRING));
    }

    if (parser_match(parser, TOKEN_BOOL_LITERAL))
    {
        LiteralValue value;
        value.bool_value = parser->previous.literal.bool_value;
        return create_literal_expr(value, create_primitive_type(TYPE_BOOL));
    }

    if (parser_match(parser, TOKEN_NIL))
    {
        LiteralValue value;
        value.int_value = 0;
        return create_literal_expr(value, create_primitive_type(TYPE_NIL));
    }

    if (parser_match(parser, TOKEN_IDENTIFIER))
    {
        return create_variable_expr(parser->previous);
    }

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        Expr *expr = parse_expression(parser);
        consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression");
        return expr;
    }

    parser_error_at_current(parser, "Expected expression");

    // Error recovery
    LiteralValue value;
    value.int_value = 0;
    return create_literal_expr(value, create_primitive_type(TYPE_NIL));
}

Expr *parse_call(Parser *parser, Expr *callee)
{
    Expr **arguments = NULL;
    int arg_count = 0;
    int capacity = 0;

    if (!check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            if (arg_count >= 255)
            {
                parser_error_at_current(parser, "Cannot have more than 255 arguments");
            }

            Expr *arg = parse_expression(parser);

            if (arg_count >= capacity)
            {
                capacity = capacity == 0 ? 8 : capacity * 2;
                arguments = realloc(arguments, sizeof(Expr *) * capacity);
            }

            arguments[arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");

    return create_call_expr(callee, arguments, arg_count);
}

Expr *parse_array_access(Parser *parser, Expr *array)
{
    Expr *index = parse_expression(parser);
    consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after index");

    return create_array_access_expr(array, index);
}

// Parse statements

Stmt *parse_statement(Parser *parser)
{
    // Skip any leading newlines
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        // Just consume newlines
    }

    if (parser_is_at_end(parser))
    {
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        return parse_var_declaration(parser);
    }

    if (parser_match(parser, TOKEN_IF))
    {
        return parse_if_statement(parser);
    }

    if (parser_match(parser, TOKEN_WHILE))
    {
        return parse_while_statement(parser);
    }

    if (parser_match(parser, TOKEN_FOR))
    {
        return parse_for_statement(parser);
    }

    if (parser_match(parser, TOKEN_RETURN))
    {
        return parse_return_statement(parser);
    }

    if (parser_match(parser, TOKEN_LEFT_BRACE))
    {
        return parse_block_statement(parser);
    }

    return parse_expression_statement(parser);
}

Stmt *parse_declaration(Parser *parser)
{
    // Skip newlines before declarations
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        // Just consume newlines
    }

    if (parser_is_at_end(parser))
    {
        // Return NULL or some sentinel value to indicate EOF
        parser_error(parser, "Unexpected end of file");
        return NULL;
    }

    if (parser_match(parser, TOKEN_VAR))
    {
        return parse_var_declaration(parser);
    }

    if (parser_match(parser, TOKEN_FN))
    {
        return parse_function_declaration(parser);
    }

    if (parser_match(parser, TOKEN_IMPORT))
    {
        return parse_import_statement(parser);
    }

    return parse_statement(parser);
}

Stmt *parse_var_declaration(Parser *parser)
{
    Token name;
    if (check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
    }
    else
    {
        parser_error_at_current(parser, "Expected variable name");
        // Error recovery
        name = parser->current;
        parser_advance(parser);
    }

    consume(parser, TOKEN_COLON, "Expected ':' after variable name");

    Type *type = parse_type(parser);

    Expr *initializer = NULL;
    if (parser_match(parser, TOKEN_EQUAL))
    {
        initializer = parse_expression(parser);
    }

    // Accept either semicolon or newline
    if (!parser_match(parser, TOKEN_SEMICOLON) &&
        !parser_match(parser, TOKEN_NEWLINE))
    {
        consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after variable declaration");
    }

    // Add to symbol table
    add_symbol(parser->symbol_table, name, type);

    return create_var_decl_stmt(name, type, initializer);
}

Stmt *parse_function_declaration(Parser *parser)
{
    Token name;
    if (check(parser, TOKEN_IDENTIFIER))
    {
        name = parser->current;
        parser_advance(parser);
    }
    else
    {
        parser_error_at_current(parser, "Expected function name");
        // Error recovery
        name = parser->current;
        parser_advance(parser);
    }

    // Parameters
    Parameter *params = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        if (!check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                if (param_count >= 255)
                {
                    parser_error_at_current(parser, "Cannot have more than 255 parameters");
                }

                Token param_name;
                if (check(parser, TOKEN_IDENTIFIER))
                {
                    param_name = parser->current;
                    parser_advance(parser);
                }
                else
                {
                    parser_error_at_current(parser, "Expected parameter name");
                    // Error recovery
                    param_name = parser->current;
                    parser_advance(parser);
                }

                consume(parser, TOKEN_COLON, "Expected ':' after parameter name");

                Type *param_type = parse_type(parser);

                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                    params = realloc(params, sizeof(Parameter) * param_capacity);
                }

                params[param_count].name = param_name;
                params[param_count].type = param_type;
                param_count++;

            } while (parser_match(parser, TOKEN_COMMA));
        }

        consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }

    // Return type
    Type *return_type = create_primitive_type(TYPE_VOID); // Default to void
    if (parser_match(parser, TOKEN_COLON))
    {
        return_type = parse_type(parser);
    }

    // Create function type for symbol table
    Type **param_types = malloc(sizeof(Type *) * param_count);
    for (int i = 0; i < param_count; i++)
    {
        param_types[i] = params[i].type;
    }
    Type *function_type = create_function_type(return_type, param_types, param_count);

    // Add to symbol table
    add_symbol(parser->symbol_table, name, function_type);

    // Create a new scope for function body
    // Use the new begin_function_scope instead of push_scope
    begin_function_scope(parser->symbol_table);

    // Add parameters to symbol table with SYMBOL_PARAM kind
    for (int i = 0; i < param_count; i++)
    {
        add_symbol_with_kind(parser->symbol_table, params[i].name, params[i].type, SYMBOL_PARAM);
    }

    // Function body
    // Function body
    consume(parser, TOKEN_ARROW, "Expected '=>' before function body");

    skip_newlines(parser);

    Stmt **body = NULL;
    int body_count = 0;
    int body_capacity = 0;

    // For all function bodies, collect statements until a clear terminator
    int done = 0;
    body_capacity = 4; // Start with space for a few statements
    body = malloc(sizeof(Stmt *) * body_capacity);

    // Parse the first statement
    Stmt *stmt = parse_declaration(parser);
    if (stmt != NULL)
    {
        body[body_count++] = stmt;
    }

    // Continue parsing statements until we find a clear function terminator
    while (!done && !parser_is_at_end(parser))
    {
        // Skip newlines
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            // Just consume newlines
        }

        // Check if we've reached a token that would indicate the end of this function
        if (check(parser, TOKEN_FN) || check(parser, TOKEN_EOF))
        {
            done = 1;
            continue;
        }

        // Parse the next statement
        stmt = parse_declaration(parser);
        if (stmt == NULL)
        {
            continue;
        }

        // Add it to the function body
        if (body_count >= body_capacity)
        {
            body_capacity *= 2;
            body = realloc(body, sizeof(Stmt *) * body_capacity);
        }

        body[body_count++] = stmt;
    }

    // Restore outer scope
    pop_scope(parser->symbol_table);

    return create_function_stmt(name, params, param_count, return_type, body, body_count);
}

Stmt *parse_return_statement(Parser *parser)
{
    Token keyword = parser->previous;

    Expr *value = NULL;
    if (!check(parser, TOKEN_SEMICOLON) &&
        !check(parser, TOKEN_NEWLINE) &&
        !parser_is_at_end(parser))
    {
        value = parse_expression(parser);
    }

    // Accept either semicolon or newline as statement terminator
    if (!parser_match(parser, TOKEN_SEMICOLON) &&
        !check(parser, TOKEN_NEWLINE) &&
        !parser_is_at_end(parser))
    {
        consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after return value");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
        // We already consumed the semicolon
    }

    return create_return_stmt(keyword, value);
}

Stmt *parse_if_statement(Parser *parser)
{
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'if'");
    Expr *condition = parse_expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after condition");
    consume(parser, TOKEN_ARROW, "Expected '=>' before 'if' body");

    // Skip any newlines after the arrow
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        // Just consume the newline token
    }

    // Don't push a new scope here unless it's a block
    Stmt *then_branch = parse_statement(parser);
    Stmt *else_branch = NULL;

    // Skip newlines before checking for else
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        // Just consume newlines
    }

    if (parser_match(parser, TOKEN_ELSE))
    {
        // Skip any newlines after the else
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            // Just consume the newline token
        }

        // Don't push a new scope here unless it's a block
        else_branch = parse_statement(parser);
    }

    return create_if_stmt(condition, then_branch, else_branch);
}

Stmt *parse_while_statement(Parser *parser)
{
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'while'");
    Expr *condition = parse_expression(parser);
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after condition");
    consume(parser, TOKEN_ARROW, "Expected '=>' before 'while' body");

    // Skip any newlines after the arrow
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        // Just consume the newline token
    }

    // Don't push a new scope unless it's a block
    Stmt *body = parse_statement(parser);

    return create_while_stmt(condition, body);
}

Stmt *parse_for_statement(Parser *parser)
{
    consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'for'");

    // Initializer
    Stmt *initializer;
    if (parser_match(parser, TOKEN_SEMICOLON))
    {
        initializer = NULL;
    }
    else if (parser_match(parser, TOKEN_VAR))
    {
        initializer = parse_var_declaration(parser);
    }
    else
    {
        initializer = parse_expression_statement(parser);
    }

    // Condition
    Expr *condition = NULL;
    if (!check(parser, TOKEN_SEMICOLON))
    {
        condition = parse_expression(parser);
    }
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after loop condition");

    // Increment
    Expr *increment = NULL;
    if (!check(parser, TOKEN_RIGHT_PAREN))
    {
        increment = parse_expression(parser);
    }
    consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after for clauses");
    consume(parser, TOKEN_ARROW, "Expected '=>' before 'for' body");

    // Skip any newlines after the arrow
    skip_newlines(parser);

    // Body
    Stmt *body = parse_statement(parser);

    return create_for_stmt(initializer, condition, increment, body);
}

Stmt *parse_block_statement(Parser *parser)
{
    Stmt **statements = NULL;
    int count = 0;
    int capacity = 0;

    // Create a new scope
    push_scope(parser->symbol_table);

    // Parse statements until we reach a level of dedentation
    // or another token that would indicate the end of a block
    while (!parser_is_at_end(parser))
    {
        // Skip newlines between statements
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            // Just consume newlines
        }

        // If we hit the end or dedentation, break
        if (parser_is_at_end(parser) || check(parser, TOKEN_DEDENT))
        {
            break;
        }

        Stmt *stmt = parse_declaration(parser);
        if (stmt == NULL)
        {
            continue; // Skip NULL statements
        }

        if (count >= capacity)
        {
            capacity = capacity == 0 ? 8 : capacity * 2;
            statements = realloc(statements, sizeof(Stmt *) * capacity);
        }

        statements[count++] = stmt;
    }

    // Restore outer scope
    pop_scope(parser->symbol_table);

    return create_block_stmt(statements, count);
}

Stmt *parse_expression_statement(Parser *parser)
{
    Expr *expr = parse_expression(parser);

    // Accept either semicolon or newline as statement terminator
    if (!parser_match(parser, TOKEN_SEMICOLON) &&
        !check(parser, TOKEN_NEWLINE) &&
        !parser_is_at_end(parser))
    {
        consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after expression");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
        // We already consumed the semicolon
    }

    return create_expr_stmt(expr);
}

Stmt *parse_import_statement(Parser *parser)
{
    Token module_name;
    if (check(parser, TOKEN_IDENTIFIER))
    {
        module_name = parser->current;
        parser_advance(parser);
    }
    else
    {
        parser_error_at_current(parser, "Expected module name");
        // Error recovery
        module_name = parser->current;
        parser_advance(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expected ';' after import statement");

    return create_import_stmt(module_name);
}

// Parse program
Module *parse(Parser *parser, const char *filename)
{
    Module *module = malloc(sizeof(Module));
    init_module(module, filename);

    while (!parser_is_at_end(parser))
    {
        // Skip any leading newlines between declarations
        while (parser_match(parser, TOKEN_NEWLINE))
        {
            // Just consume newlines
        }

        // If we reached EOF after skipping newlines, break the loop
        if (parser_is_at_end(parser))
        {
            break;
        }

        Stmt *stmt = parse_declaration(parser);
        module_add_statement(module, stmt);

        if (parser->panic_mode)
        {
            synchronize(parser);
        }
    }

    if (parser->had_error)
    {
        free_module(module);
        free(module);
        return NULL;
    }

    return module;
}