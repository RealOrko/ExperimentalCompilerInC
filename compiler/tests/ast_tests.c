// tests/ast_tests.c

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../arena.h"
#include "../ast.h"
#include "../debug.h"
#include "../token.h"

static void setup_arena(Arena *arena)
{
    arena_init(arena, 4096);
}

static void cleanup_arena(Arena *arena)
{
    arena_free(arena);
}

// Helper to create a dummy token
static Token create_dummy_token(Arena *arena, const char *str)
{
    Token tok;
    tok.start = arena_strdup(arena, str);
    tok.length = strlen(str);
    tok.type = TOKEN_IDENTIFIER;
    tok.line = 1;
    tok.filename = "test.sn";
    return tok;
}

// Test Type functions
void test_ast_create_primitive_type()
{
    printf("Testing ast_create_primitive_type...\n");
    Arena arena;
    setup_arena(&arena);

    Type *t_int = ast_create_primitive_type(&arena, TYPE_INT);
    assert(t_int != NULL);
    assert(t_int->kind == TYPE_INT);

    Type *t_void = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(t_void != NULL);
    assert(t_void->kind == TYPE_VOID);

    cleanup_arena(&arena);
}

void test_ast_create_array_type()
{
    printf("Testing ast_create_array_type...\n");
    Arena arena;
    setup_arena(&arena);

    Type *elem = ast_create_primitive_type(&arena, TYPE_INT);
    Type *arr = ast_create_array_type(&arena, elem);
    assert(arr != NULL);
    assert(arr->kind == TYPE_ARRAY);
    assert(arr->as.array.element_type == elem);

    // Edge case: NULL element
    Type *arr_null = ast_create_array_type(&arena, NULL);
    assert(arr_null != NULL);
    assert(arr_null->kind == TYPE_ARRAY);
    assert(arr_null->as.array.element_type == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_function_type()
{
    printf("Testing ast_create_function_type...\n");
    Arena arena;
    setup_arena(&arena);

    Type *ret = ast_create_primitive_type(&arena, TYPE_VOID);
    Type *params[2];
    params[0] = ast_create_primitive_type(&arena, TYPE_INT);
    params[1] = ast_create_primitive_type(&arena, TYPE_STRING);
    Type *fn = ast_create_function_type(&arena, ret, params, 2);
    assert(fn != NULL);
    assert(fn->kind == TYPE_FUNCTION);
    assert(ast_type_equals(fn->as.function.return_type, ret));
    assert(fn->as.function.param_count == 2);
    assert(ast_type_equals(fn->as.function.param_types[0], params[0]));
    assert(ast_type_equals(fn->as.function.param_types[1], params[1]));

    // Empty params
    Type *fn_empty = ast_create_function_type(&arena, ret, NULL, 0);
    assert(fn_empty != NULL);
    assert(fn_empty->as.function.param_count == 0);
    assert(fn_empty->as.function.param_types == NULL);

    // NULL return
    Type *fn_null_ret = ast_create_function_type(&arena, NULL, params, 2);
    assert(fn_null_ret != NULL);
    assert(fn_null_ret->as.function.return_type == NULL);

    cleanup_arena(&arena);
}

void test_ast_clone_type()
{
    printf("Testing ast_clone_type...\n");
    Arena arena;
    setup_arena(&arena);

    // Primitive
    Type *orig_prim = ast_create_primitive_type(&arena, TYPE_BOOL);
    Type *clone_prim = ast_clone_type(&arena, orig_prim);
    assert(clone_prim != NULL);
    assert(clone_prim != orig_prim);
    assert(clone_prim->kind == TYPE_BOOL);

    // Array
    Type *elem = ast_create_primitive_type(&arena, TYPE_CHAR);
    Type *orig_arr = ast_create_array_type(&arena, elem);
    Type *clone_arr = ast_clone_type(&arena, orig_arr);
    assert(clone_arr != NULL);
    assert(clone_arr != orig_arr);
    assert(clone_arr->kind == TYPE_ARRAY);
    assert(clone_arr->as.array.element_type != elem);
    assert(clone_arr->as.array.element_type->kind == TYPE_CHAR);

    // Function
    Type *ret = ast_create_primitive_type(&arena, TYPE_INT);
    Type *params[1] = {ast_create_primitive_type(&arena, TYPE_DOUBLE)};
    Type *orig_fn = ast_create_function_type(&arena, ret, params, 1);
    Type *clone_fn = ast_clone_type(&arena, orig_fn);
    assert(clone_fn != NULL);
    assert(clone_fn != orig_fn);
    assert(clone_fn->kind == TYPE_FUNCTION);
    assert(clone_fn->as.function.return_type->kind == TYPE_INT);
    assert(clone_fn->as.function.param_count == 1);
    assert(clone_fn->as.function.param_types[0]->kind == TYPE_DOUBLE);

    // NULL
    assert(ast_clone_type(&arena, NULL) == NULL);

    cleanup_arena(&arena);
}

void test_ast_type_equals()
{
    printf("Testing ast_type_equals...\n");
    Arena arena;
    setup_arena(&arena);

    Type *t1 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *t2 = ast_create_primitive_type(&arena, TYPE_INT);
    Type *t3 = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(ast_type_equals(t1, t2) == 1);
    assert(ast_type_equals(t1, t3) == 0);

    // Arrays
    Type *arr1 = ast_create_array_type(&arena, t1);
    Type *arr2 = ast_create_array_type(&arena, t2);
    Type *arr3 = ast_create_array_type(&arena, t3);
    assert(ast_type_equals(arr1, arr2) == 1);
    assert(ast_type_equals(arr1, arr3) == 0);

    // Functions
    Type *params1[2] = {t1, t3};
    Type *fn1 = ast_create_function_type(&arena, t1, params1, 2);
    Type *params2[2] = {t2, t3};
    Type *fn2 = ast_create_function_type(&arena, t2, params2, 2);
    Type *params3[1] = {t1};
    Type *fn3 = ast_create_function_type(&arena, t1, params3, 1);
    assert(ast_type_equals(fn1, fn2) == 1);
    assert(ast_type_equals(fn1, fn3) == 0);

    // NULL cases
    assert(ast_type_equals(NULL, NULL) == 1);
    assert(ast_type_equals(t1, NULL) == 0);

    cleanup_arena(&arena);
}

void test_ast_type_to_string()
{
    printf("Testing ast_type_to_string...\n");
    Arena arena;
    setup_arena(&arena);

    assert(strcmp(ast_type_to_string(ast_create_primitive_type(&arena, TYPE_INT)), "int") == 0);
    assert(strcmp(ast_type_to_string(ast_create_primitive_type(&arena, TYPE_DOUBLE)), "double") == 0);
    assert(strcmp(ast_type_to_string(ast_create_primitive_type(&arena, TYPE_VOID)), "void") == 0);

    Type *arr = ast_create_array_type(&arena, ast_create_primitive_type(&arena, TYPE_CHAR));
    assert(strcmp(ast_type_to_string(arr), "array<char>") == 0);

    Type *params[1] = {ast_create_primitive_type(&arena, TYPE_BOOL)};
    Type *fn = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_STRING), params, 1);
    assert(strcmp(ast_type_to_string(fn), "fn(bool) -> string") == 0);

    // Unknown kind
    Type *unknown = arena_alloc(&arena, sizeof(Type));
    unknown->kind = -1; // Invalid
    assert(strcmp(ast_type_to_string(unknown), "unknown") == 0);

    assert(ast_type_to_string(NULL) == NULL);

    cleanup_arena(&arena);
}

// Test Expr creation
void test_ast_create_binary_expr()
{
    printf("Testing ast_create_binary_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *left = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *right = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *bin = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, loc);
    assert(bin != NULL);
    assert(bin->type == EXPR_BINARY);
    assert(bin->as.binary.left == left);
    assert(bin->as.binary.right == right);
    assert(bin->as.binary.operator == TOKEN_PLUS);
    assert(bin->token == loc);
    assert(bin->expr_type == NULL);

    // NULL left/right
    Expr *bin_null = ast_create_binary_expr(&arena, NULL, TOKEN_PLUS, right, loc);
    assert(bin_null == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_unary_expr()
{
    printf("Testing ast_create_unary_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *operand = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 5}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *un = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, loc);
    assert(un != NULL);
    assert(un->type == EXPR_UNARY);
    assert(un->as.unary.operator == TOKEN_MINUS);
    assert(un->as.unary.operand == operand);
    assert(un->token == loc);

    // NULL operand
    Expr *un_null = ast_create_unary_expr(&arena, TOKEN_MINUS, NULL, loc);
    assert(un_null == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_literal_expr()
{
    printf("Testing ast_create_literal_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    LiteralValue val = {.int_value = 42};
    Type *typ = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *lit = ast_create_literal_expr(&arena, val, typ, false, loc);
    assert(lit != NULL);
    assert(lit->type == EXPR_LITERAL);
    assert(lit->as.literal.value.int_value == 42);
    assert(lit->as.literal.type == typ);
    assert(lit->as.literal.is_interpolated == false);
    assert(lit->token == loc);

    // Interpolated
    Expr *lit_interp = ast_create_literal_expr(&arena, val, typ, true, loc);
    assert(lit_interp->as.literal.is_interpolated == true);

    cleanup_arena(&arena);
}

void test_ast_create_variable_expr()
{
    printf("Testing ast_create_variable_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token name = create_dummy_token(&arena, "varname");
    Token *loc = ast_clone_token(&arena, &name);
    Expr *var = ast_create_variable_expr(&arena, name, loc);
    assert(var != NULL);
    assert(var->type == EXPR_VARIABLE);
    assert(strcmp(var->as.variable.name.start, "varname") == 0);
    assert(var->as.variable.name.length == 7);
    assert(var->token == loc);

    cleanup_arena(&arena);
}

void test_ast_create_assign_expr()
{
    printf("Testing ast_create_assign_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token name = create_dummy_token(&arena, "x");
    Token *loc = ast_clone_token(&arena, &name);
    Expr *val = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 10}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *ass = ast_create_assign_expr(&arena, name, val, loc);
    assert(ass != NULL);
    assert(ass->type == EXPR_ASSIGN);
    assert(strcmp(ass->as.assign.name.start, "x") == 0);
    assert(ass->as.assign.value == val);
    assert(ass->token == loc);

    // NULL value
    Expr *ass_null = ast_create_assign_expr(&arena, name, NULL, loc);
    assert(ass_null == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_call_expr()
{
    printf("Testing ast_create_call_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *callee = ast_create_variable_expr(&arena, create_dummy_token(&arena, "func"), loc);
    Expr *args[2];
    args[0] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    args[1] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *call = ast_create_call_expr(&arena, callee, args, 2, loc);
    assert(call != NULL);
    assert(call->type == EXPR_CALL);
    assert(call->as.call.callee == callee);
    assert(call->as.call.arg_count == 2);
    assert(call->as.call.arguments[0] == args[0]);
    assert(call->as.call.arguments[1] == args[1]);
    assert(call->token == loc);

    // Empty args
    Expr *call_empty = ast_create_call_expr(&arena, callee, NULL, 0, loc);
    assert(call_empty != NULL);
    assert(call_empty->as.call.arg_count == 0);

    // NULL callee
    Expr *call_null = ast_create_call_expr(&arena, NULL, args, 2, loc);
    assert(call_null == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_array_expr()
{
    printf("Testing ast_create_array_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *elems[3];
    for (int i = 0; i < 3; i++)
    {
        LiteralValue val = {.int_value = i};
        elems[i] = ast_create_literal_expr(&arena, val, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    }
    Expr *arr = ast_create_array_expr(&arena, elems, 3, loc);
    assert(arr != NULL);
    assert(arr->type == EXPR_ARRAY);
    assert(arr->as.array.element_count == 3);
    assert(arr->as.array.elements[0] == elems[0]);
    assert(arr->token == loc);

    // Empty array
    Expr *arr_empty = ast_create_array_expr(&arena, NULL, 0, loc);
    assert(arr_empty != NULL);
    assert(arr_empty->as.array.element_count == 0);

    cleanup_arena(&arena);
}

void test_ast_create_array_access_expr()
{
    printf("Testing ast_create_array_access_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *arr = ast_create_variable_expr(&arena, create_dummy_token(&arena, "arr"), loc);
    Expr *idx = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 0}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *acc = ast_create_array_access_expr(&arena, arr, idx, loc);
    assert(acc != NULL);
    assert(acc->type == EXPR_ARRAY_ACCESS);
    assert(acc->as.array_access.array == arr);
    assert(acc->as.array_access.index == idx);
    assert(acc->token == loc);

    // NULL array or index
    assert(ast_create_array_access_expr(&arena, NULL, idx, loc) == NULL);
    assert(ast_create_array_access_expr(&arena, arr, NULL, loc) == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_increment_expr()
{
    printf("Testing ast_create_increment_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *op = ast_create_variable_expr(&arena, create_dummy_token(&arena, "x"), loc);
    Expr *inc = ast_create_increment_expr(&arena, op, loc);
    assert(inc != NULL);
    assert(inc->type == EXPR_INCREMENT);
    assert(inc->as.operand == op);
    assert(inc->token == loc);

    // NULL operand
    assert(ast_create_increment_expr(&arena, NULL, loc) == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_decrement_expr()
{
    printf("Testing ast_create_decrement_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *op = ast_create_variable_expr(&arena, create_dummy_token(&arena, "x"), loc);
    Expr *dec = ast_create_decrement_expr(&arena, op, loc);
    assert(dec != NULL);
    assert(dec->type == EXPR_DECREMENT);
    assert(dec->as.operand == op);
    assert(dec->token == loc);

    // NULL operand
    assert(ast_create_decrement_expr(&arena, NULL, loc) == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_interpolated_expr()
{
    printf("Testing ast_create_interpolated_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *parts[2];
    parts[0] = ast_create_literal_expr(&arena, (LiteralValue){.string_value = "hello"}, ast_create_primitive_type(&arena, TYPE_STRING), false, loc);
    parts[1] = ast_create_variable_expr(&arena, create_dummy_token(&arena, "world"), loc);
    Expr *interp = ast_create_interpolated_expr(&arena, parts, 2, loc);
    assert(interp != NULL);
    assert(interp->type == EXPR_INTERPOLATED);
    assert(interp->as.interpol.part_count == 2);
    assert(interp->as.interpol.parts[0] == parts[0]);
    assert(interp->as.interpol.parts[1] == parts[1]);
    assert(interp->token == loc);

    // Empty parts
    Expr *interp_empty = ast_create_interpolated_expr(&arena, NULL, 0, loc);
    assert(interp_empty != NULL);
    assert(interp_empty->as.interpol.part_count == 0);

    cleanup_arena(&arena);
}

void test_ast_create_comparison_expr()
{
    printf("Testing ast_create_comparison_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *left = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *right = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *comp = ast_create_comparison_expr(&arena, left, right, TOKEN_GREATER, loc);
    assert(comp != NULL);
    assert(comp->type == EXPR_BINARY); // Since it's a wrapper for binary
    assert(comp->as.binary.operator == TOKEN_GREATER);

    // NULL left/right
    assert(ast_create_comparison_expr(&arena, NULL, right, TOKEN_GREATER, loc) == NULL);

    cleanup_arena(&arena);
}

// Test Stmt creation
void test_ast_create_expr_stmt()
{
    printf("Testing ast_create_expr_stmt...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *expr = ast_create_variable_expr(&arena, create_dummy_token(&arena, "x"), loc);
    Stmt *stmt = ast_create_expr_stmt(&arena, expr, loc);
    assert(stmt != NULL);
    assert(stmt->type == STMT_EXPR);
    assert(stmt->as.expression.expression == expr);
    assert(stmt->token == loc);

    // NULL expr
    assert(ast_create_expr_stmt(&arena, NULL, loc) == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_var_decl_stmt()
{
    printf("Testing ast_create_var_decl_stmt...\n");
    Arena arena;
    setup_arena(&arena);

    Token name = create_dummy_token(&arena, "var");
    Token *loc = ast_clone_token(&arena, &name);
    Type *typ = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Expr *init = ast_create_literal_expr(&arena, (LiteralValue){.double_value = 3.14}, typ, false, loc);
    Stmt *decl = ast_create_var_decl_stmt(&arena, name, typ, init, loc);
    assert(decl != NULL);
    assert(decl->type == STMT_VAR_DECL);
    assert(strcmp(decl->as.var_decl.name.start, "var") == 0);
    assert(decl->as.var_decl.type == typ);
    assert(decl->as.var_decl.initializer == init);
    assert(decl->token == loc);

    // No initializer
    Stmt *decl_no_init = ast_create_var_decl_stmt(&arena, name, typ, NULL, loc);
    assert(decl_no_init != NULL);
    assert(decl_no_init->as.var_decl.initializer == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_function_stmt()
{
    printf("Testing ast_create_function_stmt...\n");
    Arena arena;
    setup_arena(&arena);

    Token name = create_dummy_token(&arena, "func");
    Token *loc = ast_clone_token(&arena, &name);
    Parameter params[1];
    params[0].name = create_dummy_token(&arena, "p");
    params[0].type = ast_create_primitive_type(&arena, TYPE_INT);
    Type *ret = ast_create_primitive_type(&arena, TYPE_VOID);
    Stmt *body[1];
    body[0] = ast_create_return_stmt(&arena, create_dummy_token(&arena, "return"), NULL, loc);
    Stmt *fn = ast_create_function_stmt(&arena, name, params, 1, ret, body, 1, loc);
    assert(fn != NULL);
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "func") == 0);
    assert(fn->as.function.param_count == 1);
    assert(strcmp(fn->as.function.params[0].name.start, "p") == 0);
    assert(fn->as.function.return_type == ret);
    assert(fn->as.function.body_count == 1);
    assert(fn->as.function.body[0] == body[0]);
    assert(fn->token == loc);

    // Empty params and body
    Stmt *fn_empty = ast_create_function_stmt(&arena, name, NULL, 0, ret, NULL, 0, loc);
    assert(fn_empty != NULL);
    assert(fn_empty->as.function.param_count == 0);
    assert(fn_empty->as.function.body_count == 0);

    cleanup_arena(&arena);
}

void test_ast_create_return_stmt()
{
    printf("Testing ast_create_return_stmt...\n");
    Arena arena;
    setup_arena(&arena);

    Token kw = create_dummy_token(&arena, "return");
    Token *loc = ast_clone_token(&arena, &kw);
    Expr *val = ast_create_literal_expr(&arena, (LiteralValue){.bool_value = true}, ast_create_primitive_type(&arena, TYPE_BOOL), false, loc);
    Stmt *ret = ast_create_return_stmt(&arena, kw, val, loc);
    assert(ret != NULL);
    assert(ret->type == STMT_RETURN);
    assert(strcmp(ret->as.return_stmt.keyword.start, "return") == 0);
    assert(ret->as.return_stmt.value == val);
    assert(ret->token == loc);

    // No value
    Stmt *ret_no_val = ast_create_return_stmt(&arena, kw, NULL, loc);
    assert(ret_no_val != NULL);
    assert(ret_no_val->as.return_stmt.value == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_block_stmt()
{
    printf("Testing ast_create_block_stmt...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Stmt *stmts[2];
    stmts[0] = ast_create_expr_stmt(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "x"), loc), loc);
    stmts[1] = ast_create_expr_stmt(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "y"), loc), loc);
    Stmt *block = ast_create_block_stmt(&arena, stmts, 2, loc);
    assert(block != NULL);
    assert(block->type == STMT_BLOCK);
    assert(block->as.block.count == 2);
    assert(block->as.block.statements[0] == stmts[0]);
    assert(block->as.block.statements[1] == stmts[1]);
    assert(block->token == loc);

    // Empty block
    Stmt *block_empty = ast_create_block_stmt(&arena, NULL, 0, loc);
    assert(block_empty != NULL);
    assert(block_empty->as.block.count == 0);

    cleanup_arena(&arena);
}

void test_ast_create_if_stmt()
{
    printf("Testing ast_create_if_stmt...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *cond = ast_create_literal_expr(&arena, (LiteralValue){.bool_value = true}, ast_create_primitive_type(&arena, TYPE_BOOL), false, loc);
    Stmt *then = ast_create_block_stmt(&arena, NULL, 0, loc);
    Stmt *els = ast_create_block_stmt(&arena, NULL, 0, loc);
    Stmt *if_stmt = ast_create_if_stmt(&arena, cond, then, els, loc);
    assert(if_stmt != NULL);
    assert(if_stmt->type == STMT_IF);
    assert(if_stmt->as.if_stmt.condition == cond);
    assert(if_stmt->as.if_stmt.then_branch == then);
    assert(if_stmt->as.if_stmt.else_branch == els);
    assert(if_stmt->token == loc);

    // No else
    Stmt *if_no_else = ast_create_if_stmt(&arena, cond, then, NULL, loc);
    assert(if_no_else != NULL);
    assert(if_no_else->as.if_stmt.else_branch == NULL);

    // NULL cond or then
    assert(ast_create_if_stmt(&arena, NULL, then, els, loc) == NULL);
    assert(ast_create_if_stmt(&arena, cond, NULL, els, loc) == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_while_stmt()
{
    printf("Testing ast_create_while_stmt...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *cond = ast_create_literal_expr(&arena, (LiteralValue){.bool_value = true}, ast_create_primitive_type(&arena, TYPE_BOOL), false, loc);
    Stmt *body = ast_create_block_stmt(&arena, NULL, 0, loc);
    Stmt *wh = ast_create_while_stmt(&arena, cond, body, loc);
    assert(wh != NULL);
    assert(wh->type == STMT_WHILE);
    assert(wh->as.while_stmt.condition == cond);
    assert(wh->as.while_stmt.body == body);
    assert(wh->token == loc);

    // NULL cond or body
    assert(ast_create_while_stmt(&arena, NULL, body, loc) == NULL);
    assert(ast_create_while_stmt(&arena, cond, NULL, loc) == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_for_stmt()
{
    printf("Testing ast_create_for_stmt...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Stmt *init = ast_create_var_decl_stmt(&arena, create_dummy_token(&arena, "i"), ast_create_primitive_type(&arena, TYPE_INT), NULL, loc);
    Expr *cond = ast_create_literal_expr(&arena, (LiteralValue){.bool_value = true}, ast_create_primitive_type(&arena, TYPE_BOOL), false, loc);
    Expr *inc = ast_create_increment_expr(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "i"), loc), loc);
    Stmt *body = ast_create_block_stmt(&arena, NULL, 0, loc);
    Stmt *fr = ast_create_for_stmt(&arena, init, cond, inc, body, loc);
    assert(fr != NULL);
    assert(fr->type == STMT_FOR);
    assert(fr->as.for_stmt.initializer == init);
    assert(fr->as.for_stmt.condition == cond);
    assert(fr->as.for_stmt.increment == inc);
    assert(fr->as.for_stmt.body == body);
    assert(fr->token == loc);

    // Optional parts
    Stmt *fr_partial = ast_create_for_stmt(&arena, NULL, NULL, NULL, body, loc);
    assert(fr_partial != NULL);
    assert(fr_partial->as.for_stmt.initializer == NULL);
    assert(fr_partial->as.for_stmt.condition == NULL);
    assert(fr_partial->as.for_stmt.increment == NULL);

    // NULL body
    assert(ast_create_for_stmt(&arena, init, cond, inc, NULL, loc) == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_import_stmt()
{
    printf("Testing ast_create_import_stmt...\n");
    Arena arena;
    setup_arena(&arena);

    Token mod = create_dummy_token(&arena, "module");
    Token *loc = ast_clone_token(&arena, &mod);
    Stmt *imp = ast_create_import_stmt(&arena, mod, loc);
    assert(imp != NULL);
    assert(imp->type == STMT_IMPORT);
    assert(strcmp(imp->as.import.module_name.start, "module") == 0);
    assert(imp->token == loc);

    cleanup_arena(&arena);
}

// Test Module
void test_ast_init_module()
{
    printf("Testing ast_init_module...\n");
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");
    assert(mod.count == 0);
    assert(mod.capacity == 8);
    assert(mod.statements != NULL);
    assert(strcmp(mod.filename, "test.sn") == 0);

    // NULL module
    ast_init_module(&arena, NULL, "test.sn"); // Should do nothing

    cleanup_arena(&arena);
}

void test_ast_module_add_statement()
{
    printf("Testing ast_module_add_statement...\n");
    Arena arena;
    setup_arena(&arena);

    Module mod;
    ast_init_module(&arena, &mod, "test.sn");

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Stmt *s1 = ast_create_expr_stmt(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "x"), loc), loc);
    ast_module_add_statement(&arena, &mod, s1);
    assert(mod.count == 1);
    assert(mod.statements[0] == s1);

    // Add more to trigger resize
    for (int i = 1; i < 10; i++)
    {
        Stmt *s = ast_create_expr_stmt(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "y"), loc), loc);
        ast_module_add_statement(&arena, &mod, s);
    }
    assert(mod.count == 10);
    assert(mod.capacity >= 10);

    // NULL module or stmt
    ast_module_add_statement(&arena, NULL, s1);   // Nothing
    ast_module_add_statement(&arena, &mod, NULL); // Nothing

    cleanup_arena(&arena);
}

// Test cloning token
void test_ast_clone_token()
{
    printf("Testing ast_clone_token...\n");
    Arena arena;
    setup_arena(&arena);

    Token orig = create_dummy_token(&arena, "token");
    Token *clone = ast_clone_token(&arena, &orig);
    assert(clone != NULL);
    assert(clone != &orig);
    assert(strcmp(clone->start, "token") == 0);
    assert(clone->length == 5);
    assert(clone->type == TOKEN_IDENTIFIER);
    assert(clone->line == 1);
    assert(strcmp(clone->filename, "test.sn") == 0);

    // NULL
    assert(ast_clone_token(&arena, NULL) == NULL);

    cleanup_arena(&arena);
}

// Printing functions: Test no crash, perhaps capture output if needed, but for now, just call
void test_ast_print()
{
    printf("Testing ast_print_stmt and ast_print_expr (no crash)...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *expr = ast_create_binary_expr(&arena,
                                        ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc),
                                        TOKEN_PLUS,
                                        ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc),
                                        loc);
    ast_print_expr(expr, 0);

    Stmt *stmt = ast_create_if_stmt(&arena,
                                    expr,
                                    ast_create_block_stmt(&arena, NULL, 0, loc),
                                    NULL,
                                    loc);
    ast_print_stmt(stmt, 0);

    // NULL
    ast_print_expr(NULL, 0);
    ast_print_stmt(NULL, 0);

    cleanup_arena(&arena);
}