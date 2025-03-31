/**
 * ast.c
 * Implementation of AST functions with memory safety improvements
 */

#include "ast.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void pretty_print_ast(Stmt *stmt, int indent_level)
{
    if (stmt == NULL)
    {
        return;
    }

    switch (stmt->type)
    {
    case STMT_EXPR:
        DEBUG_VERBOSE_INDENT(indent_level, "ExpressionStmt:");
        pretty_print_expr(stmt->as.expression.expression, indent_level + 1);
        break;

    case STMT_VAR_DECL:
        DEBUG_VERBOSE_INDENT(indent_level, "VarDecl: %.*s (type: %s)",
                          stmt->as.var_decl.name.length,
                          stmt->as.var_decl.name.start,
                          type_to_string(stmt->as.var_decl.type));
        if (stmt->as.var_decl.initializer)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Initializer:");
            pretty_print_expr(stmt->as.var_decl.initializer, indent_level + 2);
        }
        break;

    case STMT_FUNCTION:
        DEBUG_VERBOSE_INDENT(indent_level, "Function: %.*s (return: %s)",
                          stmt->as.function.name.length,
                          stmt->as.function.name.start,
                          type_to_string(stmt->as.function.return_type));
        if (stmt->as.function.param_count > 0)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Parameters:");
            for (int i = 0; i < stmt->as.function.param_count; i++)
            {
                DEBUG_VERBOSE_INDENT(indent_level + 1, "%.*s: %s",
                                  stmt->as.function.params[i].name.length,
                                  stmt->as.function.params[i].name.start,
                                  type_to_string(stmt->as.function.params[i].type));
            }
        }
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        for (int i = 0; i < stmt->as.function.body_count; i++)
        {
            pretty_print_ast(stmt->as.function.body[i], indent_level + 2);
        }
        break;

    case STMT_RETURN:
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Return:");
        if (stmt->as.return_stmt.value)
        {
            pretty_print_expr(stmt->as.return_stmt.value, indent_level + 1);
        }
        break;

    case STMT_BLOCK:
        DEBUG_VERBOSE_INDENT(indent_level, "Block:");
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            pretty_print_ast(stmt->as.block.statements[i], indent_level + 1);
        }
        break;

    case STMT_IF:
        DEBUG_VERBOSE_INDENT(indent_level, "If:");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Condition:");
        pretty_print_expr(stmt->as.if_stmt.condition, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Then:");
        pretty_print_ast(stmt->as.if_stmt.then_branch, indent_level + 2);
        if (stmt->as.if_stmt.else_branch)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Else:");
            pretty_print_ast(stmt->as.if_stmt.else_branch, indent_level + 2);
        }
        break;

    case STMT_WHILE:
        DEBUG_VERBOSE_INDENT(indent_level, "While:");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Condition:");
        pretty_print_expr(stmt->as.while_stmt.condition, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        pretty_print_ast(stmt->as.while_stmt.body, indent_level + 2);
        break;

    case STMT_FOR:
        DEBUG_VERBOSE_INDENT(indent_level, "For:");
        if (stmt->as.for_stmt.initializer)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Initializer:");
            pretty_print_ast(stmt->as.for_stmt.initializer, indent_level + 2);
        }
        if (stmt->as.for_stmt.condition)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Condition:");
            pretty_print_expr(stmt->as.for_stmt.condition, indent_level + 2);
        }
        if (stmt->as.for_stmt.increment)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Increment:");
            pretty_print_expr(stmt->as.for_stmt.increment, indent_level + 2);
        }
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        pretty_print_ast(stmt->as.for_stmt.body, indent_level + 2);
        break;

    case STMT_IMPORT:
        DEBUG_VERBOSE_INDENT(indent_level, "Import: %.*s",
                          stmt->as.import.module_name.length,
                          stmt->as.import.module_name.start);
        break;
    }
}

void pretty_print_expr(Expr *expr, int indent_level)
{
    if (expr == NULL)
    {
        return;
    }

    switch (expr->type)
    {
    case EXPR_BINARY:
        DEBUG_VERBOSE_INDENT(indent_level, "Binary: %d", expr->as.binary.operator);
        pretty_print_expr(expr->as.binary.left, indent_level + 1);
        pretty_print_expr(expr->as.binary.right, indent_level + 1);
        break;

    case EXPR_UNARY:
        DEBUG_VERBOSE_INDENT(indent_level, "Unary: %d", expr->as.unary.operator);
        pretty_print_expr(expr->as.unary.operand, indent_level + 1);
        break;

    case EXPR_LITERAL:
        DEBUG_VERBOSE_INDENT(indent_level, "Literal: ");
        switch (expr->as.literal.type->kind)
        {
        case TYPE_INT:
            DEBUG_VERBOSE_INDENT(indent_level, "%ld", expr->as.literal.value.int_value);
            break;
        case TYPE_DOUBLE:
            DEBUG_VERBOSE_INDENT(indent_level, "%f", expr->as.literal.value.double_value);
            break;
        case TYPE_CHAR:
            DEBUG_VERBOSE_INDENT(indent_level, "'%c'", expr->as.literal.value.char_value);
            break;
        case TYPE_STRING:
            DEBUG_VERBOSE_INDENT(indent_level, "\"%s\"", expr->as.literal.value.string_value);
            break;
        case TYPE_BOOL:
            DEBUG_VERBOSE_INDENT(indent_level, "%s", expr->as.literal.value.bool_value ? "true" : "false");
            break;
        default:
            DEBUG_VERBOSE_INDENT(indent_level, "unknown");
        }
        DEBUG_VERBOSE_INDENT(indent_level, " (%s)", type_to_string(expr->as.literal.type));
        break;

    case EXPR_VARIABLE:
        DEBUG_VERBOSE_INDENT(indent_level, "Variable: %.*s",
                          expr->as.variable.name.length,
                          expr->as.variable.name.start);
        break;

    case EXPR_ASSIGN:
        DEBUG_VERBOSE_INDENT(indent_level, "Assign: %.*s",
                          expr->as.assign.name.length,
                          expr->as.assign.name.start);
        pretty_print_expr(expr->as.assign.value, indent_level + 1);
        break;

    case EXPR_CALL:
        DEBUG_VERBOSE_INDENT(indent_level, "Call:");
        pretty_print_expr(expr->as.call.callee, indent_level + 1);
        if (expr->as.call.arg_count > 0)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Arguments:");
            for (int i = 0; i < expr->as.call.arg_count; i++)
            {
                pretty_print_expr(expr->as.call.arguments[i], indent_level + 2);
            }
        }
        break;

    case EXPR_ARRAY:
        DEBUG_VERBOSE_INDENT(indent_level, "Array:");
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            pretty_print_expr(expr->as.array.elements[i], indent_level + 1);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        DEBUG_VERBOSE_INDENT(indent_level, "ArrayAccess:");
        pretty_print_expr(expr->as.array_access.array, indent_level + 1);
        pretty_print_expr(expr->as.array_access.index, indent_level + 1);
        break;

    case EXPR_INCREMENT:
        DEBUG_VERBOSE_INDENT(indent_level, "Increment:");
        pretty_print_expr(expr->as.operand, indent_level + 1);
        break;

    case EXPR_DECREMENT:
        DEBUG_VERBOSE_INDENT(indent_level, "Decrement:");
        pretty_print_expr(expr->as.operand, indent_level + 1);
        break;
    }
}

Expr *create_comparison_expr(Expr *left, Expr *right, TokenType comparison_type)
{
    if (left == NULL || right == NULL)
    {
        DEBUG_ERROR("Cannot create comparison with NULL expressions");
        return NULL;
    }

    return create_binary_expr(left, comparison_type, right);
}

void mark_type_non_freeable(Type *type)
{
    if (type == NULL)
        return;

    type->should_free = 0;

    // Also mark nested types
    if (type->kind == TYPE_ARRAY && type->as.array.element_type)
    {
        mark_type_non_freeable(type->as.array.element_type);
    }
    else if (type->kind == TYPE_FUNCTION)
    {
        if (type->as.function.return_type)
            mark_type_non_freeable(type->as.function.return_type);

        for (int i = 0; i < type->as.function.param_count; i++)
        {
            if (type->as.function.param_types[i])
                mark_type_non_freeable(type->as.function.param_types[i]);
        }
    }
}

Type *create_primitive_type(TypeKind kind)
{
    Type *type = malloc(sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    type->kind = kind;
    type->should_free = 1; // Default to freeable
    return type;
}

Type *create_array_type(Type *element_type)
{
    Type *type = malloc(sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    type->kind = TYPE_ARRAY;
    type->as.array.element_type = element_type;
    return type;
}

Type *create_function_type(Type *return_type, Type **param_types, int param_count)
{
    Type *type = malloc(sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    type->kind = TYPE_FUNCTION;
    type->as.function.return_type = return_type;
    type->as.function.param_types = param_types;
    type->as.function.param_count = param_count;
    return type;
}

int type_equals(Type *a, Type *b)
{
    if (a == b)
        return 1;
    if (a == NULL || b == NULL)
        return 0;
    if (a->kind != b->kind)
        return 0;

    switch (a->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_DOUBLE:
    case TYPE_CHAR:
    case TYPE_STRING:
    case TYPE_BOOL:
    case TYPE_VOID:
    case TYPE_NIL:
        return 1;

    case TYPE_ARRAY:
        return type_equals(a->as.array.element_type, b->as.array.element_type);

    case TYPE_FUNCTION:
    {
        if (!type_equals(a->as.function.return_type, b->as.function.return_type))
        {
            return 0;
        }

        if (a->as.function.param_count != b->as.function.param_count)
        {
            return 0;
        }

        for (int i = 0; i < a->as.function.param_count; i++)
        {
            if (a->as.function.param_types[i] == NULL || b->as.function.param_types[i] == NULL)
            {
                continue; // Skip NULL parameter types
            }
            if (!type_equals(a->as.function.param_types[i], b->as.function.param_types[i]))
            {
                return 0;
            }
        }

        return 1;
    }
    }

    return 0;
}

const char *type_to_string(Type *type)
{
    static char buffer[1024];

    if (type == NULL)
    {
        return "NULL";
    }

    switch (type->kind)
    {
    case TYPE_INT:
        return "int";
    case TYPE_LONG:
        return "long";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_CHAR:
        return "char";
    case TYPE_STRING:
        return "str";
    case TYPE_BOOL:
        return "bool";
    case TYPE_VOID:
        return "void";
    case TYPE_NIL:
        return "nil";

    case TYPE_ARRAY:
    {
        const char *element_type_str = type_to_string(type->as.array.element_type);
        snprintf(buffer, sizeof(buffer), "%s[]", element_type_str);
        return buffer;
    }

    case TYPE_FUNCTION:
    {
        const char *return_type_str = type_to_string(type->as.function.return_type);

        char *ptr = buffer;
        int remaining = sizeof(buffer);
        int n;

        n = snprintf(ptr, remaining, "fn(");
        ptr += n;
        remaining -= n;

        for (int i = 0; i < type->as.function.param_count; i++)
        {
            if (i > 0)
            {
                n = snprintf(ptr, remaining, ", ");
                ptr += n;
                remaining -= n;
            }

            const char *param_type_str = type_to_string(type->as.function.param_types[i]);
            n = snprintf(ptr, remaining, "%s", param_type_str);
            ptr += n;
            remaining -= n;
        }

        snprintf(ptr, remaining, "):%s", return_type_str);
        return buffer;
    }
    }

    return "UNKNOWN";
}

void free_type(Type *type)
{
    if (type == NULL)
        return;

    // Skip freeing if marked as non-freeable
    if (!type->should_free)
        return;

    switch (type->kind)
    {
    case TYPE_ARRAY:
        if (type->as.array.element_type != NULL)
        {
            free_type(type->as.array.element_type);
            type->as.array.element_type = NULL;
        }
        break;

    case TYPE_FUNCTION:
        if (type->as.function.return_type != NULL)
        {
            free_type(type->as.function.return_type);
            type->as.function.return_type = NULL;
        }

        if (type->as.function.param_types != NULL)
        {
            for (int i = 0; i < type->as.function.param_count; i++)
            {
                if (type->as.function.param_types[i] != NULL)
                {
                    free_type(type->as.function.param_types[i]);
                    type->as.function.param_types[i] = NULL;
                }
            }
            free(type->as.function.param_types);
            type->as.function.param_types = NULL;
        }
        break;

    default:
        break;
    }

    free(type);
}

// Expression functions

Expr *create_binary_expr(Expr *left, TokenType operator, Expr * right)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.operator= operator;
    expr->expr_type = NULL; // Will be set during type checking
    return expr;
}

Expr *create_unary_expr(TokenType operator, Expr * operand)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_UNARY;
    expr->as.unary.operator= operator;
    expr->as.unary.operand = operand;
    expr->expr_type = NULL; // Will be set during type checking
    return expr;
}

Expr *create_literal_expr(LiteralValue value, Type *type)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_LITERAL;
    expr->as.literal.value = value;
    expr->as.literal.type = type;
    expr->expr_type = type;
    return expr;
}

Expr *create_variable_expr(Token name)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_VARIABLE;
    expr->as.variable.name = name;
    expr->expr_type = NULL; // Will be set during type checking
    return expr;
}

Expr *create_assign_expr(Token name, Expr *value)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_ASSIGN;
    expr->as.assign.name = name;
    expr->as.assign.value = value;
    expr->expr_type = NULL; // Will be set during type checking
    return expr;
}

Expr *create_call_expr(Expr *callee, Expr **arguments, int arg_count)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_CALL;
    expr->as.call.callee = callee;
    expr->as.call.arguments = arguments;
    expr->as.call.arg_count = arg_count;
    expr->expr_type = NULL; // Will be set during type checking
    return expr;
}

Expr *create_array_expr(Expr **elements, int element_count)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_ARRAY;
    expr->as.array.elements = elements;
    expr->as.array.element_count = element_count;
    expr->expr_type = NULL; // Will be set during type checking
    return expr;
}

Expr *create_array_access_expr(Expr *array, Expr *index)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_ARRAY_ACCESS;
    expr->as.array_access.array = array;
    expr->as.array_access.index = index;
    expr->expr_type = NULL; // Will be set during type checking
    return expr;
}

Expr *create_increment_expr(Expr *operand)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_INCREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL; // Will be set during type checking
    return expr;
}

Expr *create_decrement_expr(Expr *operand)
{
    Expr *expr = malloc(sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->type = EXPR_DECREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL; // Will be set during type checking
    return expr;
}

void free_expr(Expr *expr)
{
    if (expr == NULL)
        return;

    switch (expr->type)
    {
    case EXPR_BINARY:
        if (expr->as.binary.left != NULL)
        {
            free_expr(expr->as.binary.left);
            expr->as.binary.left = NULL;
        }
        if (expr->as.binary.right != NULL)
        {
            free_expr(expr->as.binary.right);
            expr->as.binary.right = NULL;
        }
        break;

    case EXPR_UNARY:
        if (expr->as.unary.operand != NULL)
        {
            free_expr(expr->as.unary.operand);
            expr->as.unary.operand = NULL;
        }
        break;

    case EXPR_LITERAL:
        if (expr->as.literal.type != NULL)
        {
            free_type(expr->as.literal.type);
            expr->as.literal.type = NULL;
        }
        break;

    case EXPR_VARIABLE:
        // Nothing to free
        break;

    case EXPR_ASSIGN:
        if (expr->as.assign.value != NULL)
        {
            free_expr(expr->as.assign.value);
            expr->as.assign.value = NULL;
        }
        break;

    case EXPR_CALL:
        if (expr->as.call.callee != NULL)
        {
            free_expr(expr->as.call.callee);
            expr->as.call.callee = NULL;
        }

        if (expr->as.call.arguments != NULL)
        {
            for (int i = 0; i < expr->as.call.arg_count; i++)
            {
                if (expr->as.call.arguments[i] != NULL)
                {
                    free_expr(expr->as.call.arguments[i]);
                    expr->as.call.arguments[i] = NULL;
                }
            }
            free(expr->as.call.arguments);
            expr->as.call.arguments = NULL;
        }
        break;

    case EXPR_ARRAY:
        if (expr->as.array.elements != NULL)
        {
            for (int i = 0; i < expr->as.array.element_count; i++)
            {
                if (expr->as.array.elements[i] != NULL)
                {
                    free_expr(expr->as.array.elements[i]);
                    expr->as.array.elements[i] = NULL;
                }
            }
            free(expr->as.array.elements);
            expr->as.array.elements = NULL;
        }
        break;

    case EXPR_ARRAY_ACCESS:
        if (expr->as.array_access.array != NULL)
        {
            free_expr(expr->as.array_access.array);
            expr->as.array_access.array = NULL;
        }
        if (expr->as.array_access.index != NULL)
        {
            free_expr(expr->as.array_access.index);
            expr->as.array_access.index = NULL;
        }
        break;

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        if (expr->as.operand != NULL)
        {
            free_expr(expr->as.operand);
            expr->as.operand = NULL;
        }
        break;
    }

    if (expr->expr_type != NULL)
    {
        free_type(expr->expr_type);
        expr->expr_type = NULL;
    }

    free(expr);
}

// Statement functions

Stmt *create_expr_stmt(Expr *expression)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->type = STMT_EXPR;
    stmt->as.expression.expression = expression;
    return stmt;
}

Stmt *create_var_decl_stmt(Token name, Type *type, Expr *initializer)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->type = STMT_VAR_DECL;
    stmt->as.var_decl.name = name;
    stmt->as.var_decl.type = type;
    stmt->as.var_decl.initializer = initializer;
    return stmt;
}

Stmt *create_function_stmt(Token name, Parameter *params, int param_count,
                           Type *return_type, Stmt **body, int body_count)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->type = STMT_FUNCTION;
    stmt->as.function.name = name;
    stmt->as.function.params = params;
    stmt->as.function.param_count = param_count;
    stmt->as.function.return_type = return_type;
    stmt->as.function.body = body;
    stmt->as.function.body_count = body_count;
    return stmt;
}

Stmt *create_return_stmt(Token keyword, Expr *value)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->type = STMT_RETURN;
    stmt->as.return_stmt.keyword = keyword;
    stmt->as.return_stmt.value = value;
    return stmt;
}

Stmt *create_block_stmt(Stmt **statements, int count)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->type = STMT_BLOCK;
    stmt->as.block.statements = statements;
    stmt->as.block.count = count;
    return stmt;
}

Stmt *create_if_stmt(Expr *condition, Stmt *then_branch, Stmt *else_branch)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->type = STMT_IF;
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_branch = then_branch;
    stmt->as.if_stmt.else_branch = else_branch;
    return stmt;
}

Stmt *create_while_stmt(Expr *condition, Stmt *body)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->type = STMT_WHILE;
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = body;
    return stmt;
}

Stmt *create_for_stmt(Stmt *initializer, Expr *condition, Expr *increment, Stmt *body)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->type = STMT_FOR;
    stmt->as.for_stmt.initializer = initializer;
    stmt->as.for_stmt.condition = condition;
    stmt->as.for_stmt.increment = increment;
    stmt->as.for_stmt.body = body;
    return stmt;
}

Stmt *create_import_stmt(Token module_name)
{
    Stmt *stmt = malloc(sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->type = STMT_IMPORT;
    stmt->as.import.module_name = module_name;
    return stmt;
}

void free_stmt(Stmt *stmt)
{
    if (stmt == NULL)
        return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        if (stmt->as.expression.expression != NULL)
        {
            free_expr(stmt->as.expression.expression);
            stmt->as.expression.expression = NULL;
        }
        break;

    case STMT_VAR_DECL:
        if (stmt->as.var_decl.type != NULL)
        {
            free_type(stmt->as.var_decl.type);
            stmt->as.var_decl.type = NULL;
        }
        if (stmt->as.var_decl.initializer != NULL)
        {
            free_expr(stmt->as.var_decl.initializer);
            stmt->as.var_decl.initializer = NULL;
        }
        break;

    case STMT_FUNCTION:
        if (stmt->as.function.return_type != NULL)
        {
            free_type(stmt->as.function.return_type);
            stmt->as.function.return_type = NULL;
        }

        if (stmt->as.function.params != NULL)
        {
            for (int i = 0; i < stmt->as.function.param_count; i++)
            {
                if (stmt->as.function.params[i].type != NULL)
                {
                    free_type(stmt->as.function.params[i].type);
                    stmt->as.function.params[i].type = NULL;
                }
            }
            free(stmt->as.function.params);
            stmt->as.function.params = NULL;
        }

        if (stmt->as.function.body != NULL)
        {
            for (int i = 0; i < stmt->as.function.body_count; i++)
            {
                if (stmt->as.function.body[i] != NULL)
                {
                    free_stmt(stmt->as.function.body[i]);
                    stmt->as.function.body[i] = NULL;
                }
            }
            free(stmt->as.function.body);
            stmt->as.function.body = NULL;
        }
        break;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value != NULL)
        {
            free_expr(stmt->as.return_stmt.value);
            stmt->as.return_stmt.value = NULL;
        }
        break;

    case STMT_BLOCK:
        if (stmt->as.block.statements != NULL)
        {
            for (int i = 0; i < stmt->as.block.count; i++)
            {
                if (stmt->as.block.statements[i] != NULL)
                {
                    free_stmt(stmt->as.block.statements[i]);
                    stmt->as.block.statements[i] = NULL;
                }
            }
            free(stmt->as.block.statements);
            stmt->as.block.statements = NULL;
        }
        break;

    case STMT_IF:
        if (stmt->as.if_stmt.condition != NULL)
        {
            free_expr(stmt->as.if_stmt.condition);
            stmt->as.if_stmt.condition = NULL;
        }
        if (stmt->as.if_stmt.then_branch != NULL)
        {
            free_stmt(stmt->as.if_stmt.then_branch);
            stmt->as.if_stmt.then_branch = NULL;
        }
        if (stmt->as.if_stmt.else_branch != NULL)
        {
            free_stmt(stmt->as.if_stmt.else_branch);
            stmt->as.if_stmt.else_branch = NULL;
        }
        break;

    case STMT_WHILE:
        if (stmt->as.while_stmt.condition != NULL)
        {
            free_expr(stmt->as.while_stmt.condition);
            stmt->as.while_stmt.condition = NULL;
        }
        if (stmt->as.while_stmt.body != NULL)
        {
            free_stmt(stmt->as.while_stmt.body);
            stmt->as.while_stmt.body = NULL;
        }
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer != NULL)
        {
            free_stmt(stmt->as.for_stmt.initializer);
            stmt->as.for_stmt.initializer = NULL;
        }
        if (stmt->as.for_stmt.condition != NULL)
        {
            free_expr(stmt->as.for_stmt.condition);
            stmt->as.for_stmt.condition = NULL;
        }
        if (stmt->as.for_stmt.increment != NULL)
        {
            free_expr(stmt->as.for_stmt.increment);
            stmt->as.for_stmt.increment = NULL;
        }
        if (stmt->as.for_stmt.body != NULL)
        {
            free_stmt(stmt->as.for_stmt.body);
            stmt->as.for_stmt.body = NULL;
        }
        break;

    case STMT_IMPORT:
        // Nothing to free
        break;
    }

    free(stmt);
}

// Module functions

void init_module(Module *module, const char *filename)
{
    if (module == NULL)
        return;

    module->count = 0;
    module->capacity = 8;

    module->statements = malloc(sizeof(Stmt *) * module->capacity);
    if (module->statements == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }

    module->filename = filename;
}

void module_add_statement(Module *module, Stmt *stmt)
{
    if (module == NULL || stmt == NULL)
        return;

    if (module->count == module->capacity)
    {
        module->capacity *= 2;
        Stmt **new_statements = realloc(module->statements, sizeof(Stmt *) * module->capacity);
        if (new_statements == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        module->statements = new_statements;
    }

    module->statements[module->count++] = stmt;
}

void free_stmt_preserve_types(Stmt *stmt)
{
    if (stmt == NULL)
        return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        if (stmt->as.expression.expression != NULL)
        {
            free_expr_preserve_types(stmt->as.expression.expression);
            stmt->as.expression.expression = NULL;
        }
        break;

    case STMT_VAR_DECL:
        // Don't free the type here - the symbol table owns it
        // stmt->as.var_decl.type = NULL; // Just nullify

        if (stmt->as.var_decl.initializer != NULL)
        {
            free_expr_preserve_types(stmt->as.var_decl.initializer);
            stmt->as.var_decl.initializer = NULL;
        }
        break;

    default:
        // Handle all other statement types generically
        // (or leave empty if no action needed)
        break;
    }

    free(stmt);
}

void free_module(Module *module)
{
    if (module == NULL)
        return;

    if (module->statements != NULL)
    {
        for (int i = 0; i < module->count; i++)
        {
            if (module->statements[i] != NULL)
            {
                // Use a special flag to indicate we're in cleanup mode
                free_stmt_preserve_types(module->statements[i]);
                module->statements[i] = NULL;
            }
        }

        free(module->statements);
        module->statements = NULL;
    }

    module->count = 0;
    module->capacity = 0;
}

void free_expr_preserve_types(Expr *expr)
{
    if (expr == NULL)
        return;

    switch (expr->type)
    {
    case EXPR_BINARY:
        if (expr->as.binary.left != NULL)
        {
            free_expr_preserve_types(expr->as.binary.left);
            expr->as.binary.left = NULL;
        }
        if (expr->as.binary.right != NULL)
        {
            free_expr_preserve_types(expr->as.binary.right);
            expr->as.binary.right = NULL;
        }
        break;

        // Handle all other expression types similarly...
        // (same as free_expr but skip freeing types)
    }

    // Don't free the type here - the symbol table owns it
    // expr->expr_type = NULL; // Just nullify

    free(expr);
}