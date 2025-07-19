/**
 * ast.h
 * Abstract Syntax Tree node definitions
 */

#ifndef AST_H
#define AST_H

#include "token.h"
#include <stddef.h>
#include <stdbool.h>

// Forward declarations for circular types
typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Type Type;

// Type definitions
typedef enum
{
    TYPE_INT,
    TYPE_LONG,
    TYPE_DOUBLE,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_NIL
} TypeKind;

struct Type
{
    TypeKind kind;
    int should_free;

    union
    {
        // For array types
        struct
        {
            Type *element_type;
        } array;

        // For function types
        struct
        {
            Type *return_type;
            Type **param_types;
            int param_count;
        } function;
    } as;
};

// Expression types
typedef enum
{
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_LITERAL,
    EXPR_VARIABLE,
    EXPR_ASSIGN,
    EXPR_CALL,
    EXPR_ARRAY,
    EXPR_ARRAY_ACCESS,
    EXPR_INCREMENT,
    EXPR_DECREMENT
} ExprType;

// Binary expression
typedef struct
{
    Expr *left;
    Expr *right;
    TokenType operator;
} BinaryExpr;

// Unary expression
typedef struct
{
    Expr *operand;
    TokenType operator;
} UnaryExpr;

// Literal expression
typedef struct
{
    LiteralValue value;
    Type *type;
    bool is_interpolated;
} LiteralExpr;

// Variable expression
typedef struct
{
    Token name;
} VariableExpr;

// Assignment expression
typedef struct
{
    Token name;
    Expr *value;
} AssignExpr;

// Function call expression
typedef struct
{
    Expr *callee;
    Expr **arguments;
    int arg_count;
} CallExpr;

// Array expression
typedef struct
{
    Expr **elements;
    int element_count;
} ArrayExpr;

// Array access expression
typedef struct
{
    Expr *array;
    Expr *index;
} ArrayAccessExpr;

// Generic expression structure
struct Expr
{
    ExprType type;

    union
    {
        BinaryExpr binary;
        UnaryExpr unary;
        LiteralExpr literal;
        VariableExpr variable;
        AssignExpr assign;
        CallExpr call;
        ArrayExpr array;
        ArrayAccessExpr array_access;
        Expr *operand; // For increment/decrement
    } as;

    Type *expr_type; // Type of the expression after type checking
};

// Statement types
typedef enum
{
    STMT_EXPR,
    STMT_VAR_DECL,
    STMT_FUNCTION,
    STMT_RETURN,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_IMPORT
} StmtType;

// Expression statement
typedef struct
{
    Expr *expression;
} ExprStmt;

// Variable declaration
typedef struct
{
    Token name;
    Type *type;
    Expr *initializer;
} VarDeclStmt;

// Function parameter
typedef struct
{
    Token name;
    Type *type;
} Parameter;

// Function declaration
typedef struct
{
    Token name;
    Parameter *params;
    int param_count;
    Type *return_type;
    Stmt **body;
    int body_count;
} FunctionStmt;

// Return statement
typedef struct
{
    Token keyword;
    Expr *value;
} ReturnStmt;

// Block statement
typedef struct
{
    Stmt **statements;
    int count;
} BlockStmt;

// If statement
typedef struct
{
    Expr *condition;
    Stmt *then_branch;
    Stmt *else_branch;
} IfStmt;

// While statement
typedef struct
{
    Expr *condition;
    Stmt *body;
} WhileStmt;

// For statement
typedef struct
{
    Stmt *initializer;
    Expr *condition;
    Expr *increment;
    Stmt *body;
} ForStmt;

// Import statement
typedef struct
{
    Token module_name;
} ImportStmt;

// Generic statement structure
struct Stmt
{
    StmtType type;

    union
    {
        ExprStmt expression;
        VarDeclStmt var_decl;
        FunctionStmt function;
        ReturnStmt return_stmt;
        BlockStmt block;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        ImportStmt import;
    } as;
};

// AST module
typedef struct
{
    Stmt **statements;
    int count;
    int capacity;
    const char *filename;
} Module;

// Debug
void ast_print_stmt(Stmt *stmt, int indent_level);
void ast_print_expr(Expr *expr, int indent_level);

// Type functions
Type *ast_clone_type(Type *type);
void ast_mark_type_non_freeable(Type *type);
Type *ast_create_primitive_type(TypeKind kind);
Type *ast_create_array_type(Type *element_type);
Type *ast_create_function_type(Type *return_type, Type **param_types, int param_count);
int ast_type_equals(Type *a, Type *b);
const char *ast_type_to_string(Type *type);
void ast_free_type(Type *type);

// Expression functions
Expr *ast_create_binary_expr(Expr *left, TokenType operator, Expr * right);
Expr *ast_create_unary_expr(TokenType operator, Expr * operand);
Expr *ast_create_literal_expr(LiteralValue value, Type *type, bool is_interpolated);
Expr *ast_create_variable_expr(Token name);
Expr *ast_create_assign_expr(Token name, Expr *value);
Expr *ast_create_call_expr(Expr *callee, Expr **arguments, int arg_count);
Expr *ast_create_array_expr(Expr **elements, int element_count);
Expr *ast_create_array_access_expr(Expr *array, Expr *index);
Expr *ast_create_increment_expr(Expr *operand);
Expr *ast_create_decrement_expr(Expr *operand);
Expr *ast_create_interpolated_expr(Expr **parts, int part_count); // <-- Add this line
Expr *ast_create_comparison_expr(Expr *left, Expr *right, TokenType comparison_type);
void ast_free_expr(Expr *expr);

// Statement functions
Stmt *ast_create_expr_stmt(Expr *expression);
Stmt *ast_create_var_decl_stmt(Token name, Type *type, Expr *initializer);
Stmt *ast_create_function_stmt(Token name, Parameter *params, int param_count,
                               Type *return_type, Stmt **body, int body_count);
Stmt *ast_create_return_stmt(Token keyword, Expr *value);
Stmt *ast_create_block_stmt(Stmt **statements, int count);
Stmt *ast_create_if_stmt(Expr *condition, Stmt *then_branch, Stmt *else_branch);
Stmt *ast_create_while_stmt(Expr *condition, Stmt *body);
Stmt *ast_create_for_stmt(Stmt *initializer, Expr *condition, Expr *increment, Stmt *body);
Stmt *ast_create_import_stmt(Token module_name);
void ast_free_stmt(Stmt *stmt);

// Module functions
void ast_init_module(Module *module, const char *filename);
void ast_module_add_statement(Module *module, Stmt *stmt);
void ast_free_module(Module *module);

#endif // AST_H