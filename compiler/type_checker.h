/**
 * type_checker.h
 * Type checker for the compiler
 */

#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "ast.h"
#include "symbol_table.h"

typedef struct
{
    SymbolTable *symbol_table;
    int had_error;
    Type *current_function_return_type;
    int in_loop;
} TypeChecker;

typedef struct {
    TypeKind from;
    TypeKind to;
    int is_convertible;
    TypeKind promotion_result;  // What type it converts to
} TypeConversionRule;

// Initialize and cleanup type checker
void init_type_checker(TypeChecker *checker, SymbolTable *symbol_table);
void type_checker_cleanup(TypeChecker *checker);

// Type check a module
int type_check_module(TypeChecker *checker, Module *module);

// Type check statements
void type_check_statement(TypeChecker *checker, Stmt *stmt);
void type_check_expression_statement(TypeChecker *checker, ExprStmt *stmt);
void type_check_var_declaration(TypeChecker *checker, VarDeclStmt *stmt);
void type_check_function(TypeChecker *checker, FunctionStmt *stmt);
void type_check_return_statement(TypeChecker *checker, ReturnStmt *stmt);
void type_check_block(TypeChecker *checker, BlockStmt *stmt);
void type_check_if_statement(TypeChecker *checker, IfStmt *stmt);
void type_check_while_statement(TypeChecker *checker, WhileStmt *stmt);
void type_check_for_statement(TypeChecker *checker, ForStmt *stmt);
void type_check_import_statement(TypeChecker *checker, ImportStmt *stmt);

// Type check expressions
Type *type_check_expression(TypeChecker *checker, Expr *expr);
Type *type_check_binary_expression(TypeChecker *checker, BinaryExpr *expr);
Type *type_check_unary_expression(TypeChecker *checker, UnaryExpr *expr);
Type *type_check_literal_expression(TypeChecker *checker, LiteralExpr *expr);
Type *type_check_variable_expression(TypeChecker *checker, VariableExpr *expr);
Type *type_check_assign_expression(TypeChecker *checker, AssignExpr *expr);
Type *type_check_call_expression(TypeChecker *checker, CallExpr *expr);
Type *type_check_array_expression(TypeChecker *checker, ArrayExpr *expr);
Type *type_check_array_access_expression(TypeChecker *checker, ArrayAccessExpr *expr);
Type *type_check_increment_expression(TypeChecker *checker, Expr *expr);
Type *type_check_decrement_expression(TypeChecker *checker, Expr *expr);

// Type conversion helpers
int can_convert(Type *from, Type *to);
Type *common_type(Type *a, Type *b);
void error(TypeChecker *checker, const char *message);

#endif // TYPE_CHECKER_H