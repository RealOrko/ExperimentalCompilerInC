/**
 * type_checker.c
 * Implementation of the type checker
 */

#include "type_checker.h"
#include "debug.h"
#include "lexer.h"
#include "parser.h"
#include <string.h>
#include <ctype.h>

static int had_type_error = 0;

static void type_error(const char *msg)
{
    DEBUG_ERROR("Type error: %s", msg);
    had_type_error = 1;
}

static void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type);

static bool is_numeric_type(Type *type)
{
    return type && (type->kind == TYPE_INT || type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE);
}

static bool is_comparison_operator(TokenType op)
{
    return op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL || op == TOKEN_LESS || op == TOKEN_LESS_EQUAL || op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL;
}

static bool is_arithmetic_operator(TokenType op)
{
    return op == TOKEN_PLUS || op == TOKEN_MINUS || op == TOKEN_STAR || op == TOKEN_SLASH || op == TOKEN_MODULO;
}

static bool is_printable_type(Type *type)
{
    return type && (type->kind == TYPE_INT || type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE || type->kind == TYPE_CHAR || type->kind == TYPE_STRING || type->kind == TYPE_BOOL);
}

static Type *type_check_binary(Expr *expr, SymbolTable *table)
{
    Type *left = type_check_expr(expr->as.binary.left, table);
    Type *right = type_check_expr(expr->as.binary.right, table);
    if (left == NULL || right == NULL)
        return NULL;
    if (!ast_type_equals(left, right))
    {
        type_error("Type mismatch in binary expression");
        return NULL;
    }
    if (is_comparison_operator(expr->as.binary.operator))
    {
        return ast_create_primitive_type(TYPE_BOOL);
    }
    else if (is_arithmetic_operator(expr->as.binary.operator))
    {
        if (is_numeric_type(left))
        {
            return ast_clone_type(left);
        }
        else if (left->kind == TYPE_STRING && expr->as.binary.operator == TOKEN_PLUS)
        {
            return ast_clone_type(left);
        }
        else
        {
            type_error("Invalid types for arithmetic operator");
            return NULL;
        }
    }
    else
    {
        type_error("Invalid binary operator");
        return NULL;
    }
}

static Type *type_check_unary(Expr *expr, SymbolTable *table)
{
    Type *operand = type_check_expr(expr->as.unary.operand, table);
    if (operand == NULL)
        return NULL;
    if (expr->as.unary.operator == TOKEN_MINUS)
    {
        if (!is_numeric_type(operand))
        {
            type_error("Unary minus on non-numeric");
            return NULL;
        }
        return ast_clone_type(operand);
    }
    else if (expr->as.unary.operator == TOKEN_BANG)
    {
        if (operand->kind != TYPE_BOOL)
        {
            type_error("Unary ! on non-bool");
            return NULL;
        }
        return ast_clone_type(operand);
    }
    type_error("Invalid unary operator");
    return NULL;
}

static Type *type_check_interpolated(Expr *expr, SymbolTable *table)
{
    for (int i = 0; i < expr->as.interpol.part_count; i++)
    {
        Type *part_type = type_check_expr(expr->as.interpol.parts[i], table);
        if (part_type == NULL)
            return NULL;
        if (!is_printable_type(part_type))
        {
            type_error("Non-printable type in interpolated string");
            return NULL;
        }
    }
    return ast_create_primitive_type(TYPE_STRING);
}

static Type *type_check_literal(Expr *expr, SymbolTable *table)
{
    (void)table; // Unused
    return ast_clone_type(expr->as.literal.type);
}

static Type *type_check_variable(Expr *expr, SymbolTable *table)
{
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
    if (sym == NULL)
    {
        type_error("Undefined variable");
        return NULL;
    }
    return ast_clone_type(sym->type);
}

static Type *type_check_assign(Expr *expr, SymbolTable *table)
{
    Type *value_type = type_check_expr(expr->as.assign.value, table);
    if (value_type == NULL)
        return NULL;
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.assign.name);
    if (sym == NULL)
    {
        type_error("Undefined variable for assignment");
        return NULL;
    }
    if (!ast_type_equals(sym->type, value_type))
    {
        type_error("Type mismatch in assignment");
        return NULL;
    }
    return ast_clone_type(sym->type);
}

static Type *type_check_call(Expr *expr, SymbolTable *table)
{
    Type *callee_type = type_check_expr(expr->as.call.callee, table);
    if (callee_type == NULL)
        return NULL;
    bool is_print = false;
    if (expr->as.call.callee->type == EXPR_VARIABLE)
    {
        Token name = expr->as.call.callee->as.variable.name;
        char name_str[256];
        strncpy(name_str, name.start, name.length);
        name_str[name.length] = '\0';
        if (strcmp(name_str, "print") == 0)
        {
            is_print = true;
        }
    }
    if (is_print)
    {
        if (expr->as.call.arg_count != 1)
        {
            type_error("print takes exactly one argument");
            return NULL;
        }
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL)
            return NULL;
        if (!is_printable_type(arg_type))
        {
            type_error("Unsupported type for print");
            return NULL;
        }
        return ast_create_primitive_type(TYPE_VOID);
    }
    else
    {
        if (callee_type->kind != TYPE_FUNCTION)
        {
            type_error("Callee is not a function");
            return NULL;
        }
        if (callee_type->as.function.param_count != expr->as.call.arg_count)
        {
            type_error("Argument count mismatch in call");
            return NULL;
        }
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            Type *arg_type = type_check_expr(expr->as.call.arguments[i], table);
            if (arg_type == NULL)
                return NULL;
            if (!ast_type_equals(arg_type, callee_type->as.function.param_types[i]))
            {
                type_error("Argument type mismatch in call");
                return NULL;
            }
        }
        return ast_clone_type(callee_type->as.function.return_type);
    }
}

Type *type_check_expr(Expr *expr, SymbolTable *table)
{
    if (expr == NULL)
        return NULL;
    if (expr->expr_type)
        return expr->expr_type;
    Type *t = NULL;
    switch (expr->type)
    {
    case EXPR_BINARY:
        t = type_check_binary(expr, table);
        break;
    case EXPR_UNARY:
        t = type_check_unary(expr, table);
        break;
    case EXPR_LITERAL:
        t = type_check_literal(expr, table);
        break;
    case EXPR_VARIABLE:
        t = type_check_variable(expr, table);
        break;
    case EXPR_ASSIGN:
        t = type_check_assign(expr, table);
        break;
    case EXPR_CALL:
        t = type_check_call(expr, table);
        break;
    case EXPR_ARRAY:
        // TODO: Implement if arrays are used
        t = ast_create_array_type(ast_create_primitive_type(TYPE_NIL));
        break;
    case EXPR_ARRAY_ACCESS:
        // TODO: Implement if arrays are used
        t = ast_create_primitive_type(TYPE_NIL);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
    {
        Type *operand_type = type_check_expr(expr->as.operand, table);
        t = ast_clone_type(operand_type);
        if (operand_type == NULL || !is_numeric_type(operand_type))
        {
            type_error("Increment/decrement on non-numeric type");
            ast_free_type(t);
            t = NULL;
        }
        break;
    }
    case EXPR_INTERPOLATED:
        t = type_check_interpolated(expr, table);
        break;
    }
    expr->expr_type = t;
    return t;
}

static void type_check_var_decl(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    (void)return_type;
    Type *init_type;
    if (stmt->as.var_decl.initializer)
    {
        init_type = type_check_expr(stmt->as.var_decl.initializer, table);
        if (init_type == NULL)
            return;
    }
    else
    {
        init_type = ast_create_primitive_type(TYPE_NIL);
    }
    if (!ast_type_equals(init_type, stmt->as.var_decl.type))
    {
        type_error("Initializer type does not match variable type");
    }
    if (!stmt->as.var_decl.initializer)
    {
        ast_free_type(init_type);
    }
}

static void type_check_function(Stmt *stmt, SymbolTable *table)
{
    symbol_table_push_scope(table);
    for (int i = 0; i < stmt->as.function.param_count; i++)
    {
        symbol_table_add_symbol_with_kind(table, stmt->as.function.params[i].name,
                                          stmt->as.function.params[i].type, SYMBOL_PARAM);
    }
    for (int i = 0; i < stmt->as.function.body_count; i++)
    {
        type_check_stmt(stmt->as.function.body[i], table, stmt->as.function.return_type);
    }
    symbol_table_pop_scope(table);
}

static void type_check_return(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    Type *value_type;
    if (stmt->as.return_stmt.value)
    {
        value_type = type_check_expr(stmt->as.return_stmt.value, table);
        if (value_type == NULL)
            return;
    }
    else
    {
        value_type = ast_create_primitive_type(TYPE_VOID);
    }
    if (!ast_type_equals(value_type, return_type))
    {
        type_error("Return type does not match function return type");
    }
    if (!stmt->as.return_stmt.value)
    {
        ast_free_type(value_type);
    }
}

static void type_check_block(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    symbol_table_push_scope(table);
    for (int i = 0; i < stmt->as.block.count; i++)
    {
        type_check_stmt(stmt->as.block.statements[i], table, return_type);
    }
    symbol_table_pop_scope(table);
}

static void type_check_if(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    Type *cond_type = type_check_expr(stmt->as.if_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error("If condition must be boolean");
    }
    // Do not free cond_type; owned by AST
    type_check_stmt(stmt->as.if_stmt.then_branch, table, return_type);
    if (stmt->as.if_stmt.else_branch)
    {
        type_check_stmt(stmt->as.if_stmt.else_branch, table, return_type);
    }
}

static void type_check_while(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    Type *cond_type = type_check_expr(stmt->as.while_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error("While condition must be boolean");
    }
    // Do not free cond_type; owned by AST
    type_check_stmt(stmt->as.while_stmt.body, table, return_type);
}

static void type_check_for(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    symbol_table_push_scope(table);
    if (stmt->as.for_stmt.initializer)
    {
        type_check_stmt(stmt->as.for_stmt.initializer, table, return_type);
    }
    if (stmt->as.for_stmt.condition)
    {
        Type *cond_type = type_check_expr(stmt->as.for_stmt.condition, table);
        if (cond_type && cond_type->kind != TYPE_BOOL)
        {
            type_error("For condition must be boolean");
        }
        // Do not free cond_type; owned by AST
    }
    if (stmt->as.for_stmt.increment)
    {
        type_check_expr(stmt->as.for_stmt.increment, table);
    }
    type_check_stmt(stmt->as.for_stmt.body, table, return_type);
    symbol_table_pop_scope(table);
}

static void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    if (stmt == NULL)
        return;
    switch (stmt->type)
    {
    case STMT_EXPR:
        type_check_expr(stmt->as.expression.expression, table);
        break;
    case STMT_VAR_DECL:
        type_check_var_decl(stmt, table, return_type);
        // Add the variable to the current scope after checking initializer
        symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name,
                                          stmt->as.var_decl.type, SYMBOL_LOCAL);
        break;
    case STMT_FUNCTION:
        type_check_function(stmt, table);
        break;
    case STMT_RETURN:
        type_check_return(stmt, table, return_type);
        break;
    case STMT_BLOCK:
        type_check_block(stmt, table, return_type);
        break;
    case STMT_IF:
        type_check_if(stmt, table, return_type);
        break;
    case STMT_WHILE:
        type_check_while(stmt, table, return_type);
        break;
    case STMT_FOR:
        type_check_for(stmt, table, return_type);
        break;
    case STMT_IMPORT:
        // No type checking needed for imports (yet)
        break;
    }
}

int type_check_module(Module *module, SymbolTable *table)
{
    had_type_error = 0;
    for (int i = 0; i < module->count; i++)
    {
        type_check_stmt(module->statements[i], table, NULL);
    }
    return !had_type_error;
}
