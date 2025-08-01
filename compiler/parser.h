#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "arena.h"
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
    char **interp_sources;
    int interp_count;
    int interp_capacity;
    Arena *arena;
} Parser;

void parser_init(Arena *arena, Parser *parser, Lexer *lexer);
void parser_cleanup(Parser *parser);

void parser_error(Parser *parser, const char *message);
void parser_error_at_current(Parser *parser, const char *message);
void parser_error_at(Parser *parser, Token *token, const char *message);

void parser_advance(Parser *parser);
void parser_consume(Parser *parser, TokenType type, const char *message);
int parser_check(Parser *parser, TokenType type);
int parser_match(Parser *parser, TokenType type);

Type *parser_type(Parser *parser);

Expr *parser_expression(Parser *parser);
Expr *parser_assignment(Parser *parser);
Expr *parser_logical_or(Parser *parser);
Expr *parser_logical_and(Parser *parser);
Expr *parser_equality(Parser *parser);
Expr *parser_comparison(Parser *parser);
Expr *parser_term(Parser *parser);
Expr *parser_factor(Parser *parser);
Expr *parser_unary(Parser *parser);
Expr *parser_postfix(Parser *parser);
Expr *parser_primary(Parser *parser);
Expr *parser_call(Parser *parser, Expr *callee);
Expr *parser_array_access(Parser *parser, Expr *array);

Stmt *parser_statement(Parser *parser);
Stmt *parser_declaration(Parser *parser);
Stmt *parser_var_declaration(Parser *parser);
Stmt *parser_function_declaration(Parser *parser);
Stmt *parser_return_statement(Parser *parser);
Stmt *parser_if_statement(Parser *parser);
Stmt *parser_while_statement(Parser *parser);
Stmt *parser_for_statement(Parser *parser);
Stmt *parser_block_statement(Parser *parser);
Stmt *parser_expression_statement(Parser *parser);
Stmt *parser_import_statement(Parser *parser);

Module *parser_execute(Parser *parser, const char *filename);

#endif