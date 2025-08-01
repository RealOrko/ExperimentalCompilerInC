#include "type_checker.h"
#include "debug.h"
#include "lexer.h"
#include "parser.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static int had_type_error = 0;

static void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type);

static Type *type_check_expr(Expr *expr, SymbolTable *table);

static void type_error(Token *token, const char *msg)
{
    char error_buffer[256];
    if (token && token->line > 0 && token->filename) {
        snprintf(error_buffer, sizeof(error_buffer), "%s:%d: Type error: %s", 
                 token->filename, token->line, msg);
    } else {
        snprintf(error_buffer, sizeof(error_buffer), "Type error: %s", msg);
    }
    DEBUG_ERROR("%s", error_buffer);
    had_type_error = 1;
}

static bool is_numeric_type(Type *type)
{
    return type && (type->kind == TYPE_INT || type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE);
}

static bool is_comparison_operator(TokenType op)
{
    return op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL || op == TOKEN_LESS || 
           op == TOKEN_LESS_EQUAL || op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL;
}

static bool is_arithmetic_operator(TokenType op)
{
    return op == TOKEN_MINUS || op == TOKEN_STAR || op == TOKEN_SLASH || op == TOKEN_MODULO;
}

static bool is_printable_type(Type *type)
{
    return type && (type->kind == TYPE_INT || type->kind == TYPE_LONG || 
                    type->kind == TYPE_DOUBLE || type->kind == TYPE_CHAR || 
                    type->kind == TYPE_STRING || type->kind == TYPE_BOOL);
}

static Type *type_check_binary(Expr *expr, SymbolTable *table)
{
    Type *left = type_check_expr(expr->as.binary.left, table);
    Type *right = type_check_expr(expr->as.binary.right, table);
    if (left == NULL || right == NULL)
    {
        type_error(expr->token, "Invalid operand in binary expression");
        return NULL;
    }
    TokenType op = expr->as.binary.operator;
    if (is_comparison_operator(op))
    {
        if (!ast_type_equals(left, right))
        {
            type_error(expr->token, "Type mismatch in comparison");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_BOOL);
    }
    else if (is_arithmetic_operator(op))
    {
        if (!ast_type_equals(left, right) || !is_numeric_type(left))
        {
            type_error(expr->token, "Invalid types for arithmetic operator");
            return NULL;
        }
        return ast_clone_type(table->arena, left);
    }
    else if (op == TOKEN_PLUS)
    {
        if (is_numeric_type(left) && ast_type_equals(left, right))
        {
            return ast_clone_type(table->arena, left);
        }
        else if (left->kind == TYPE_STRING && is_printable_type(right))
        {
            return ast_clone_type(table->arena, left);
        }
        else if (is_printable_type(left) && right->kind == TYPE_STRING)
        {
            return ast_clone_type(table->arena, right);
        }
        else
        {
            type_error(expr->token, "Invalid types for + operator");
            return NULL;
        }
    }
    else
    {
        type_error(expr->token, "Invalid binary operator");
        return NULL;
    }
}

static Type *type_check_unary(Expr *expr, SymbolTable *table)
{
    Type *operand = type_check_expr(expr->as.unary.operand, table);
    if (operand == NULL)
    {
        type_error(expr->token, "Invalid operand in unary expression");
        return NULL;
    }
    if (expr->as.unary.operator == TOKEN_MINUS)
    {
        if (!is_numeric_type(operand))
        {
            type_error(expr->token, "Unary minus on non-numeric");
            return NULL;
        }
        return ast_clone_type(table->arena, operand);
    }
    else if (expr->as.unary.operator == TOKEN_BANG)
    {
        if (operand->kind != TYPE_BOOL)
        {
            type_error(expr->token, "Unary ! on non-bool");
            return NULL;
        }
        return ast_clone_type(table->arena, operand);
    }
    type_error(expr->token, "Invalid unary operator");
    return NULL;
}

static Type *type_check_interpolated(Expr *expr, SymbolTable *table)
{
    for (int i = 0; i < expr->as.interpol.part_count; i++)
    {
        Type *part_type = type_check_expr(expr->as.interpol.parts[i], table);
        if (part_type == NULL)
        {
            type_error(expr->token, "Invalid expression in interpolated string part");
            return NULL;
        }
        if (!is_printable_type(part_type))
        {
            type_error(expr->token, "Non-printable type in interpolated string");
            return NULL;
        }
    }
    return ast_create_primitive_type(table->arena, TYPE_STRING);
}

static Type *type_check_literal(Expr *expr, SymbolTable *table)
{
    (void)table;
    return ast_clone_type(table->arena, expr->as.literal.type);
}

static Type *type_check_variable(Expr *expr, SymbolTable *table)
{
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
    if (sym == NULL)
    {
        type_error(&expr->as.variable.name, "Undefined variable");
        return NULL;
    }
    if (sym->type == NULL)
    {
        type_error(&expr->as.variable.name, "Symbol has no type");
        return NULL;
    }
    return ast_clone_type(table->arena, sym->type);
}

static Type *type_check_assign(Expr *expr, SymbolTable *table)
{
    Type *value_type = type_check_expr(expr->as.assign.value, table);
    if (value_type == NULL)
    {
        type_error(expr->token, "Invalid value in assignment");
        return NULL;
    }
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.assign.name);
    if (sym == NULL)
    {
        type_error(&expr->as.assign.name, "Undefined variable for assignment");
        return NULL;
    }
    if (!ast_type_equals(sym->type, value_type))
    {
        type_error(&expr->as.assign.name, "Type mismatch in assignment");
        return NULL;
    }
    return ast_clone_type(table->arena, sym->type);
}

static Type *type_check_call(Expr *expr, SymbolTable *table)
{
    Type *callee_type = type_check_expr(expr->as.call.callee, table);
    if (callee_type == NULL)
    {
        type_error(expr->token, "Invalid callee in function call");
        return NULL;
    }
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
            type_error(expr->token, "print takes exactly one argument");
            return NULL;
        }
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL)
        {
            type_error(expr->token, "Invalid argument in print call");
            return NULL;
        }
        if (!is_printable_type(arg_type))
        {
            type_error(expr->token, "Unsupported type for print");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_VOID);
    }
    bool is_to_string = false;
    if (expr->as.call.callee->type == EXPR_VARIABLE)
    {
        Token name = expr->as.call.callee->as.variable.name;
        char name_str[256];
        strncpy(name_str, name.start, name.length);
        name_str[name.length] = '\0';
        if (strcmp(name_str, "to_string") == 0)
        {
            is_to_string = true;
        }
    }
    if (is_to_string)
    {
        if (expr->as.call.arg_count != 1)
        {
            type_error(expr->token, "to_string takes exactly one argument");
            return NULL;
        }
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL)
        {
            type_error(expr->token, "Invalid argument in to_string call");
            return NULL;
        }
        if (!is_printable_type(arg_type))
        {
            type_error(expr->token, "Unsupported type for to_string");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_STRING);
    }
    else
    {
        if (callee_type->kind != TYPE_FUNCTION)
        {
            type_error(expr->token, "Callee is not a function");
            return NULL;
        }
        if (callee_type->as.function.param_count != expr->as.call.arg_count)
        {
            type_error(expr->token, "Argument count mismatch in call");
            return NULL;
        }
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            Type *arg_type = type_check_expr(expr->as.call.arguments[i], table);
            if (arg_type == NULL)
            {
                type_error(expr->token, "Invalid argument in function call");
                return NULL;
            }
            if (!ast_type_equals(arg_type, callee_type->as.function.param_types[i]))
            {
                type_error(expr->token, "Argument type mismatch in call");
                return NULL;
            }
        }
        return ast_clone_type(table->arena, callee_type->as.function.return_type);
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
        t = ast_create_array_type(table->arena, ast_create_primitive_type(table->arena, TYPE_NIL));
        break;
    case EXPR_ARRAY_ACCESS:
        t = ast_create_primitive_type(table->arena, TYPE_NIL);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
    {
        Type *operand_type = type_check_expr(expr->as.operand, table);
        t = ast_clone_type(table->arena, operand_type);
        if (operand_type == NULL || !is_numeric_type(operand_type))
        {
            type_error(expr->token, "Increment/decrement on non-numeric type");
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
        init_type = ast_create_primitive_type(table->arena, TYPE_NIL);
    }
    if (!ast_type_equals(init_type, stmt->as.var_decl.type))
    {
        type_error(&stmt->as.var_decl.name, "Initializer type does not match variable type");
    }
    symbol_table_add_symbol_with_kind(table, stmt->as.var_decl.name,
                                      stmt->as.var_decl.type, SYMBOL_LOCAL);
}

static void type_check_function(Stmt *stmt, SymbolTable *table)
{
    symbol_table_push_scope(table);
    
    for (int i = 0; i < stmt->as.function.param_count; i++)
    {
        Parameter param = stmt->as.function.params[i];
        symbol_table_add_symbol_with_kind(table, param.name, param.type, SYMBOL_PARAM);
    }
    
    table->current->next_local_offset = table->current->next_param_offset;
    
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
        value_type = ast_create_primitive_type(table->arena, TYPE_VOID);
    }
    if (!ast_type_equals(value_type, return_type))
    {
        type_error(stmt->token, "Return type does not match function return type");
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
        type_error(stmt->as.if_stmt.condition->token, "If condition must be boolean");
    }
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
        type_error(stmt->as.while_stmt.condition->token, "While condition must be boolean");
    }
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
            type_error(stmt->as.for_stmt.condition->token, "For condition must be boolean");
        }
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