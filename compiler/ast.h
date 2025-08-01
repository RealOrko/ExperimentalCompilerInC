#ifndef AST_H
#define AST_H

#include "arena.h"
#include "token.h"
#include <stddef.h>
#include <stdbool.h>

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Type Type;

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

    union
    {
        struct
        {
            Type *element_type;
        } array;

        struct
        {
            Type *return_type;
            Type **param_types;
            int param_count;
        } function;
    } as;
};

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
    EXPR_DECREMENT,
    EXPR_INTERPOLATED
} ExprType;

typedef struct
{
    Expr *left;
    Expr *right;
    TokenType operator;
} BinaryExpr;

typedef struct
{
    Expr *operand;
    TokenType operator;
} UnaryExpr;

typedef struct
{
    LiteralValue value;
    Type *type;
    bool is_interpolated;
} LiteralExpr;

typedef struct
{
    Token name;
} VariableExpr;

typedef struct
{
    Token name;
    Expr *value;
} AssignExpr;

typedef struct
{
    Expr *callee;
    Expr **arguments;
    int arg_count;
} CallExpr;

typedef struct
{
    Expr **elements;
    int element_count;
} ArrayExpr;

typedef struct
{
    Expr *array;
    Expr *index;
} ArrayAccessExpr;

typedef struct
{
    Expr **parts;
    int part_count;
} InterpolExpr;

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
        Expr *operand;
        InterpolExpr interpol;
    } as;

    Type *expr_type;
};

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

typedef struct
{
    Expr *expression;
} ExprStmt;

typedef struct
{
    Token name;
    Type *type;
    Expr *initializer;
} VarDeclStmt;

typedef struct
{
    Token name;
    Type *type;
} Parameter;

typedef struct
{
    Token name;
    Parameter *params;
    int param_count;
    Type *return_type;
    Stmt **body;
    int body_count;
} FunctionStmt;

typedef struct
{
    Token keyword;
    Expr *value;
} ReturnStmt;

typedef struct
{
    Stmt **statements;
    int count;
} BlockStmt;

typedef struct
{
    Expr *condition;
    Stmt *then_branch;
    Stmt *else_branch;
} IfStmt;

typedef struct
{
    Expr *condition;
    Stmt *body;
} WhileStmt;

typedef struct
{
    Stmt *initializer;
    Expr *condition;
    Expr *increment;
    Stmt *body;
} ForStmt;

typedef struct
{
    Token module_name;
} ImportStmt;

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

typedef struct
{
    Stmt **statements;
    int count;
    int capacity;
    const char *filename;
} Module;

void ast_print_stmt(Stmt *stmt, int indent_level);
void ast_print_expr(Expr *expr, int indent_level);

Type *ast_clone_type(Arena *arena, Type *type);
void ast_mark_type_non_freeable(Type *type);
Type *ast_create_primitive_type(Arena *arena, TypeKind kind);
Type *ast_create_array_type(Arena *arena, Type *element_type);
Type *ast_create_function_type(Arena *arena, Type *return_type, Type **param_types, int param_count);
int ast_type_equals(Type *a, Type *b);
const char *ast_type_to_string(Type *type);
void ast_free_type(Type *type);
void ast_free_token(Token *token);

Expr *ast_create_binary_expr(Arena *arena, Expr *left, TokenType operator, Expr *right);
Expr *ast_create_unary_expr(Arena *arena, TokenType operator, Expr *operand);
Expr *ast_create_literal_expr(Arena *arena, LiteralValue value, Type *type, bool is_interpolated);
Expr *ast_create_variable_expr(Arena *arena, Token name);
Expr *ast_create_assign_expr(Arena *arena, Token name, Expr *value);
Expr *ast_create_call_expr(Arena *arena, Expr *callee, Expr **arguments, int arg_count);
Expr *ast_create_array_expr(Arena *arena, Expr **elements, int element_count);
Expr *ast_create_array_access_expr(Arena *arena, Expr *array, Expr *index);
Expr *ast_create_increment_expr(Arena *arena, Expr *operand);
Expr *ast_create_decrement_expr(Arena *arena, Expr *operand);
Expr *ast_create_interpolated_expr(Arena *arena, Expr **parts, int part_count);
Expr *ast_create_comparison_expr(Arena *arena, Expr *left, Expr *right, TokenType comparison_type);
void ast_free_expr(Expr *expr);

Stmt *ast_create_expr_stmt(Arena *arena, Expr *expression);
Stmt *ast_create_var_decl_stmt(Arena *arena, Token name, Type *type, Expr *initializer);
Stmt *ast_create_function_stmt(Arena *arena, Token name, Parameter *params, int param_count,
                               Type *return_type, Stmt **body, int body_count);
Stmt *ast_create_return_stmt(Arena *arena, Token keyword, Expr *value);
Stmt *ast_create_block_stmt(Arena *arena, Stmt **statements, int count);
Stmt *ast_create_if_stmt(Arena *arena, Expr *condition, Stmt *then_branch, Stmt *else_branch);
Stmt *ast_create_while_stmt(Arena *arena, Expr *condition, Stmt *body);
Stmt *ast_create_for_stmt(Arena *arena, Stmt *initializer, Expr *condition, Expr *increment, Stmt *body);
Stmt *ast_create_import_stmt(Arena *arena, Token module_name);
void ast_init_module(Arena *arena, Module *module, const char *filename);
void ast_module_add_statement(Arena *arena, Module *module, Stmt *stmt);
void ast_free_module(Module *module);

#endif