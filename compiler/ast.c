#include "arena.h"
#include "ast.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

void ast_print_stmt(Stmt *stmt, int indent_level)
{
    if (stmt == NULL)
    {
        return;
    }

    switch (stmt->type)
    {
    case STMT_EXPR:
        DEBUG_VERBOSE_INDENT(indent_level, "ExpressionStmt:");
        ast_print_expr(stmt->as.expression.expression, indent_level + 1);
        break;

    case STMT_VAR_DECL:
        DEBUG_VERBOSE_INDENT(indent_level, "VarDecl: %.*s (type: %s)",
                             stmt->as.var_decl.name.length,
                             stmt->as.var_decl.name.start,
                             ast_type_to_string(stmt->as.var_decl.type));
        if (stmt->as.var_decl.initializer)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Initializer:");
            ast_print_expr(stmt->as.var_decl.initializer, indent_level + 2);
        }
        break;

    case STMT_FUNCTION:
        DEBUG_VERBOSE_INDENT(indent_level, "Function: %.*s (return: %s)",
                             stmt->as.function.name.length,
                             stmt->as.function.name.start,
                             ast_type_to_string(stmt->as.function.return_type));
        if (stmt->as.function.param_count > 0)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Parameters:");
            for (int i = 0; i < stmt->as.function.param_count; i++)
            {
                DEBUG_VERBOSE_INDENT(indent_level + 1, "%.*s: %s",
                                     stmt->as.function.params[i].name.length,
                                     stmt->as.function.params[i].name.start,
                                     ast_type_to_string(stmt->as.function.params[i].type));
            }
        }
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        for (int i = 0; i < stmt->as.function.body_count; i++)
        {
            ast_print_stmt(stmt->as.function.body[i], indent_level + 2);
        }
        break;

    case STMT_RETURN:
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Return:");
        if (stmt->as.return_stmt.value)
        {
            ast_print_expr(stmt->as.return_stmt.value, indent_level + 1);
        }
        break;

    case STMT_BLOCK:
        DEBUG_VERBOSE_INDENT(indent_level, "Block:");
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            ast_print_stmt(stmt->as.block.statements[i], indent_level + 1);
        }
        break;

    case STMT_IF:
        DEBUG_VERBOSE_INDENT(indent_level, "If:");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Condition:");
        ast_print_expr(stmt->as.if_stmt.condition, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Then:");
        ast_print_stmt(stmt->as.if_stmt.then_branch, indent_level + 2);
        if (stmt->as.if_stmt.else_branch)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Else:");
            ast_print_stmt(stmt->as.if_stmt.else_branch, indent_level + 2);
        }
        break;

    case STMT_WHILE:
        DEBUG_VERBOSE_INDENT(indent_level, "While:");
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Condition:");
        ast_print_expr(stmt->as.while_stmt.condition, indent_level + 2);
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        ast_print_stmt(stmt->as.while_stmt.body, indent_level + 2);
        break;

    case STMT_FOR:
        DEBUG_VERBOSE_INDENT(indent_level, "For:");
        if (stmt->as.for_stmt.initializer)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Initializer:");
            ast_print_stmt(stmt->as.for_stmt.initializer, indent_level + 2);
        }
        if (stmt->as.for_stmt.condition)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Condition:");
            ast_print_expr(stmt->as.for_stmt.condition, indent_level + 2);
        }
        if (stmt->as.for_stmt.increment)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Increment:");
            ast_print_expr(stmt->as.for_stmt.increment, indent_level + 2);
        }
        DEBUG_VERBOSE_INDENT(indent_level + 1, "Body:");
        ast_print_stmt(stmt->as.for_stmt.body, indent_level + 2);
        break;

    case STMT_IMPORT:
        DEBUG_VERBOSE_INDENT(indent_level, "Import: %.*s",
                             stmt->as.import.module_name.length,
                             stmt->as.import.module_name.start);
        break;
    }
}

void ast_print_expr(Expr *expr, int indent_level)
{
    if (expr == NULL)
    {
        return;
    }

    switch (expr->type)
    {
    case EXPR_BINARY:
        DEBUG_VERBOSE_INDENT(indent_level, "Binary: %d", expr->as.binary.operator);
        ast_print_expr(expr->as.binary.left, indent_level + 1);
        ast_print_expr(expr->as.binary.right, indent_level + 1);
        break;

    case EXPR_UNARY:
        DEBUG_VERBOSE_INDENT(indent_level, "Unary: %d", expr->as.unary.operator);
        ast_print_expr(expr->as.unary.operand, indent_level + 1);
        break;

    case EXPR_LITERAL:
        DEBUG_VERBOSE_INDENT(indent_level, "Literal%s: ", expr->as.literal.is_interpolated ? " (interpolated)" : "");
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
        DEBUG_VERBOSE_INDENT(indent_level, " (%s)", ast_type_to_string(expr->as.literal.type));
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
        ast_print_expr(expr->as.assign.value, indent_level + 1);
        break;

    case EXPR_CALL:
        DEBUG_VERBOSE_INDENT(indent_level, "Call:");
        ast_print_expr(expr->as.call.callee, indent_level + 1);
        if (expr->as.call.arg_count > 0)
        {
            DEBUG_VERBOSE_INDENT(indent_level + 1, "Arguments:");
            for (int i = 0; i < expr->as.call.arg_count; i++)
            {
                ast_print_expr(expr->as.call.arguments[i], indent_level + 2);
            }
        }
        break;

    case EXPR_ARRAY:
        DEBUG_VERBOSE_INDENT(indent_level, "Array:");
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            ast_print_expr(expr->as.array.elements[i], indent_level + 1);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        DEBUG_VERBOSE_INDENT(indent_level, "ArrayAccess:");
        ast_print_expr(expr->as.array_access.array, indent_level + 1);
        ast_print_expr(expr->as.array_access.index, indent_level + 1);
        break;

    case EXPR_INCREMENT:
        DEBUG_VERBOSE_INDENT(indent_level, "Increment:");
        ast_print_expr(expr->as.operand, indent_level + 1);
        break;

    case EXPR_DECREMENT:
        DEBUG_VERBOSE_INDENT(indent_level, "Decrement:");
        ast_print_expr(expr->as.operand, indent_level + 1);
        break;

    case EXPR_INTERPOLATED:
        DEBUG_VERBOSE_INDENT(indent_level, "Interpolated String:");
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            ast_print_expr(expr->as.interpol.parts[i], indent_level + 1);
        }
        break;
    }
}

Token *ast_clone_token(Arena *arena, const Token *src)
{
    if (src == NULL)
    {
        return NULL;
    }

    Token *cloned = arena_alloc(arena, sizeof(Token));
    if (cloned == NULL)
    {
        DEBUG_ERROR("Out of memory when cloning token");
        exit(1);
    }

    cloned->type = src->type;
    cloned->start = arena_strndup(arena, src->start, src->length);
    cloned->length = src->length;
    cloned->line = src->line;
    cloned->filename = src->filename;

    return cloned;
}

Expr *ast_create_comparison_expr(Arena *arena, Expr *left, Expr *right, TokenType comparison_type, const Token *loc_token)
{
    if (left == NULL || right == NULL)
    {
        DEBUG_ERROR("Cannot create comparison with NULL expressions");
        return NULL;
    }

    return ast_create_binary_expr(arena, left, comparison_type, right, loc_token);
}

Type *ast_clone_type(Arena *arena, Type *type)
{
    if (type == NULL)
        return NULL;

    Type *clone = arena_alloc(arena, sizeof(Type));
    if (clone == NULL)
    {
        DEBUG_ERROR("Out of memory when cloning type");
        exit(1);
    }
    memset(clone, 0, sizeof(Type));

    clone->kind = type->kind;

    switch (type->kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_DOUBLE:
    case TYPE_CHAR:
    case TYPE_STRING:
    case TYPE_BOOL:
    case TYPE_VOID:
    case TYPE_NIL:
        break;

    case TYPE_ARRAY:
        clone->as.array.element_type = ast_clone_type(arena, type->as.array.element_type);
        break;

    case TYPE_FUNCTION:
        clone->as.function.return_type = ast_clone_type(arena, type->as.function.return_type);
        clone->as.function.param_count = type->as.function.param_count;

        if (type->as.function.param_count > 0)
        {
            clone->as.function.param_types = arena_alloc(arena, sizeof(Type *) * type->as.function.param_count);
            if (clone->as.function.param_types == NULL)
            {
                DEBUG_ERROR("Out of memory when cloning function param types");
                exit(1);
            }
            for (int i = 0; i < type->as.function.param_count; i++)
            {
                clone->as.function.param_types[i] = ast_clone_type(arena, type->as.function.param_types[i]);
            }
        }
        else
        {
            clone->as.function.param_types = NULL;
        }
        break;
    }

    return clone;
}

Type *ast_create_primitive_type(Arena *arena, TypeKind kind)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = kind;
    return type;
}

Type *ast_create_array_type(Arena *arena, Type *element_type)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = TYPE_ARRAY;
    type->as.array.element_type = element_type;
    return type;
}

Type *ast_create_function_type(Arena *arena, Type *return_type, Type **param_types, int param_count)
{
    Type *type = arena_alloc(arena, sizeof(Type));
    if (type == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(type, 0, sizeof(Type));
    type->kind = TYPE_FUNCTION;

    type->as.function.return_type = ast_clone_type(arena, return_type);

    type->as.function.param_count = param_count;
    if (param_count > 0)
    {
        type->as.function.param_types = arena_alloc(arena, sizeof(Type *) * param_count);
        if (type->as.function.param_types == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        for (int i = 0; i < param_count; i++)
        {
            type->as.function.param_types[i] = ast_clone_type(arena, param_types[i]);
        }
    }
    else
    {
        type->as.function.param_types = NULL;
    }

    return type;
}

int ast_type_equals(Type *a, Type *b)
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
        return ast_type_equals(a->as.array.element_type, b->as.array.element_type);

    case TYPE_FUNCTION:
    {
        if (!ast_type_equals(a->as.function.return_type, b->as.function.return_type))
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
                continue;
            }
            if (!ast_type_equals(a->as.function.param_types[i], b->as.function.param_types[i]))
            {
                return 0;
            }
        }

        return 1;
    }
    }

    return 0;
}

const char *ast_type_to_string(Type *type)
{
    static char buffer[1024];

    if (type == NULL)
    {
        return "NULL";
    }

    if (type->kind < TYPE_INT || type->kind > TYPE_FUNCTION)
    {
        DEBUG_ERROR("Invalid TypeKind: %d", type->kind);
        return "INVALID";
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
        const char *element_type_str = ast_type_to_string(type->as.array.element_type);
        snprintf(buffer, sizeof(buffer), "%s[]", element_type_str);
        return buffer;
    }

    case TYPE_FUNCTION:
    {
        const char *return_type_str = ast_type_to_string(type->as.function.return_type);

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

            const char *param_type_str = ast_type_to_string(type->as.function.param_types[i]);
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

Expr *ast_create_binary_expr(Arena *arena, Expr *left, TokenType operator, Expr *right, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.operator = operator;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_unary_expr(Arena *arena, TokenType operator, Expr *operand, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_UNARY;
    expr->as.unary.operator = operator;
    expr->as.unary.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_literal_expr(Arena *arena, LiteralValue value, Type *type, bool is_interpolated, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.value = value;
    expr->as.literal.type = type;
    expr->as.literal.is_interpolated = is_interpolated;
    expr->expr_type = ast_clone_type(arena, type);
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_variable_expr(Arena *arena, Token name, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_VARIABLE;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.variable.name.start = new_start;
    expr->as.variable.name.length = name.length;
    expr->as.variable.name.line = name.line;
    expr->as.variable.name.type = name.type;
    expr->as.variable.name.filename = name.filename;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_assign_expr(Arena *arena, Token name, Expr *value, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ASSIGN;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.assign.name.start = new_start;
    expr->as.assign.name.length = name.length;
    expr->as.assign.name.line = name.line;
    expr->as.assign.name.type = name.type;
    expr->as.assign.name.filename = name.filename;
    expr->as.assign.value = value;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_call_expr(Arena *arena, Expr *callee, Expr **arguments, int arg_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_CALL;
    expr->as.call.callee = callee;
    expr->as.call.arguments = arguments;
    expr->as.call.arg_count = arg_count;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ARRAY;
    expr->as.array.elements = elements;
    expr->as.array.element_count = element_count;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_access_expr(Arena *arena, Expr *array, Expr *index, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ARRAY_ACCESS;
    expr->as.array_access.array = array;
    expr->as.array_access.index = index;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_increment_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_INCREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_decrement_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_DECREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_interpolated_expr(Arena *arena, Expr **parts, int part_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_INTERPOLATED;
    expr->as.interpol.parts = parts;
    expr->as.interpol.part_count = part_count;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Stmt *ast_create_expr_stmt(Arena *arena, Expr *expression, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_EXPR;
    stmt->as.expression.expression = expression;
    stmt->token = ast_clone_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_var_decl_stmt(Arena *arena, Token name, Type *type, Expr *initializer, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_VAR_DECL;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.var_decl.name.start = new_start;
    stmt->as.var_decl.name.length = name.length;
    stmt->as.var_decl.name.line = name.line;
    stmt->as.var_decl.name.type = name.type;
    stmt->as.var_decl.name.filename = name.filename;  // Added for location reporting.
    stmt->as.var_decl.type = type;
    stmt->as.var_decl.initializer = initializer;
    stmt->token = ast_clone_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_function_stmt(Arena *arena, Token name, Parameter *params, int param_count,
                               Type *return_type, Stmt **body, int body_count, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_FUNCTION;
    char *new_name_start = arena_strndup(arena, name.start, name.length);
    if (new_name_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.function.name.start = new_name_start;
    stmt->as.function.name.length = name.length;
    stmt->as.function.name.line = name.line;
    stmt->as.function.name.type = name.type;
    stmt->as.function.name.filename = name.filename;  // Added for location reporting.

    Parameter *new_params = arena_alloc(arena, sizeof(Parameter) * param_count);
    if (new_params == NULL && param_count > 0)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
    {
        char *new_param_start = arena_strndup(arena, params[i].name.start, params[i].name.length);
        if (new_param_start == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        new_params[i].name.start = new_param_start;
        new_params[i].name.length = params[i].name.length;
        new_params[i].name.line = params[i].name.line;
        new_params[i].name.type = params[i].name.type;
        new_params[i].name.filename = params[i].name.filename;  // Added for location reporting.
        new_params[i].type = params[i].type;
    }
    stmt->as.function.params = new_params;
    stmt->as.function.param_count = param_count;
    stmt->as.function.return_type = return_type;
    stmt->as.function.body = body;
    stmt->as.function.body_count = body_count;
    stmt->token = ast_clone_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_return_stmt(Arena *arena, Token keyword, Expr *value, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_RETURN;
    stmt->as.return_stmt.keyword.start = keyword.start;
    stmt->as.return_stmt.keyword.length = keyword.length;
    stmt->as.return_stmt.keyword.line = keyword.line;
    stmt->as.return_stmt.keyword.type = keyword.type;
    stmt->as.return_stmt.keyword.filename = keyword.filename;  // Added for location reporting.
    stmt->as.return_stmt.value = value;
    stmt->token = ast_clone_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_block_stmt(Arena *arena, Stmt **statements, int count, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_BLOCK;
    stmt->as.block.statements = statements;
    stmt->as.block.count = count;
    stmt->token = ast_clone_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_if_stmt(Arena *arena, Expr *condition, Stmt *then_branch, Stmt *else_branch, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_IF;
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_branch = then_branch;
    stmt->as.if_stmt.else_branch = else_branch;
    stmt->token = ast_clone_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_while_stmt(Arena *arena, Expr *condition, Stmt *body, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_WHILE;
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = body;
    stmt->token = ast_clone_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_for_stmt(Arena *arena, Stmt *initializer, Expr *condition, Expr *increment, Stmt *body, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_FOR;
    stmt->as.for_stmt.initializer = initializer;
    stmt->as.for_stmt.condition = condition;
    stmt->as.for_stmt.increment = increment;
    stmt->as.for_stmt.body = body;
    stmt->token = ast_clone_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_import_stmt(Arena *arena, Token module_name, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_IMPORT;
    char *new_start = arena_strndup(arena, module_name.start, module_name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.import.module_name.start = new_start;
    stmt->as.import.module_name.length = module_name.length;
    stmt->as.import.module_name.line = module_name.line;
    stmt->as.import.module_name.type = module_name.type;
    stmt->as.import.module_name.filename = module_name.filename;
    stmt->token = ast_clone_token(arena, loc_token);
    return stmt;
}

void ast_init_module(Arena *arena, Module *module, const char *filename)
{
    if (module == NULL)
        return;

    module->count = 0;
    module->capacity = 8;

    module->statements = arena_alloc(arena, sizeof(Stmt *) * module->capacity);
    if (module->statements == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(module->statements, 0, sizeof(Stmt *) * module->capacity);

    module->filename = filename;
}

void ast_module_add_statement(Arena *arena, Module *module, Stmt *stmt)
{
    if (module == NULL || stmt == NULL)
        return;

    if (module->count == module->capacity)
    {
        size_t new_capacity = module->capacity * 2;
        Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * new_capacity);
        if (new_statements == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        memcpy(new_statements, module->statements, sizeof(Stmt *) * module->capacity);
        memset(new_statements + module->capacity, 0, sizeof(Stmt *) * (new_capacity - module->capacity));
        module->statements = new_statements;
        module->capacity = new_capacity;
    }

    module->statements[module->count++] = stmt;
}