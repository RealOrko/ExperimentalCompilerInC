/**
 * parser.h
 * Parser for the compiler
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "ast.h"
#include "symbol_table.h"

typedef struct
{
    Lexer *lexer;
    Token current;
    Token previous;
    int had_error;
    int panic_mode;
    SymbolTable *symbol_table;
} Parser;

// Parser initialization and cleanup
void init_parser(Parser *parser, Lexer *lexer);
void parser_cleanup(Parser *parser);

// Error handling
void parser_error(Parser *parser, const char *message);
void parser_error_at_current(Parser *parser, const char *message);
void parser_error_at(Parser *parser, Token *token, const char *message);

// Token handling
void parser_advance(Parser *parser);
void consume(Parser *parser, TokenType type, const char *message);
int check(Parser *parser, TokenType type);
int parser_match(Parser *parser, TokenType type);

// Parse type
Type *parse_type(Parser *parser);

// Parse expressions
Expr *parse_expression(Parser *parser);
Expr *parse_assignment(Parser *parser);
Expr *parse_logical_or(Parser *parser);
Expr *parse_logical_and(Parser *parser);
Expr *parse_equality(Parser *parser);
Expr *parse_comparison(Parser *parser);
Expr *parse_term(Parser *parser);
Expr *parse_factor(Parser *parser);
Expr *parse_unary(Parser *parser);
Expr *parse_postfix(Parser *parser);
Expr *parse_primary(Parser *parser);
Expr *parse_call(Parser *parser, Expr *callee);
Expr *parse_array_access(Parser *parser, Expr *array);

// Parse statements
Stmt *parse_statement(Parser *parser);
Stmt *parse_declaration(Parser *parser);
Stmt *parse_var_declaration(Parser *parser);
Stmt *parse_function_declaration(Parser *parser);
Stmt *parse_return_statement(Parser *parser);
Stmt *parse_if_statement(Parser *parser);
Stmt *parse_while_statement(Parser *parser);
Stmt *parse_for_statement(Parser *parser);
Stmt *parse_block_statement(Parser *parser);
Stmt *parse_expression_statement(Parser *parser);
Stmt *parse_import_statement(Parser *parser);

// Parse program
Module *parse(Parser *parser, const char *filename);

#endif // PARSER_H