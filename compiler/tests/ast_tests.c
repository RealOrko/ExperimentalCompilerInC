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

static int tokens_equal(const Token *a, const Token *b)
{
    if (a == NULL && b == NULL)
        return 1;
    if (a == NULL || b == NULL)
        return 0;
    return a->type == b->type &&
           a->length == b->length &&
           a->line == b->line &&
           strcmp(a->start, b->start) == 0 &&
           strcmp(a->filename, b->filename) == 0;
}

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

    // Test all primitive kinds
    Type *t_int = ast_create_primitive_type(&arena, TYPE_INT);
    assert(t_int != NULL);
    assert(t_int->kind == TYPE_INT);

    Type *t_long = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(t_long != NULL);
    assert(t_long->kind == TYPE_LONG);

    Type *t_double = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    assert(t_double != NULL);
    assert(t_double->kind == TYPE_DOUBLE);

    Type *t_char = ast_create_primitive_type(&arena, TYPE_CHAR);
    assert(t_char != NULL);
    assert(t_char->kind == TYPE_CHAR);

    Type *t_string = ast_create_primitive_type(&arena, TYPE_STRING);
    assert(t_string != NULL);
    assert(t_string->kind == TYPE_STRING);

    Type *t_bool = ast_create_primitive_type(&arena, TYPE_BOOL);
    assert(t_bool != NULL);
    assert(t_bool->kind == TYPE_BOOL);

    Type *t_void = ast_create_primitive_type(&arena, TYPE_VOID);
    assert(t_void != NULL);
    assert(t_void->kind == TYPE_VOID);

    Type *t_nil = ast_create_primitive_type(&arena, TYPE_NIL);
    assert(t_nil != NULL);
    assert(t_nil->kind == TYPE_NIL);

    Type *t_any = ast_create_primitive_type(&arena, TYPE_ANY);
    assert(t_any != NULL);
    assert(t_any->kind == TYPE_ANY);

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

    // Nested array
    Type *nested_arr = ast_create_array_type(&arena, arr);
    assert(nested_arr != NULL);
    assert(nested_arr->kind == TYPE_ARRAY);
    assert(nested_arr->as.array.element_type == arr);
    assert(nested_arr->as.array.element_type->as.array.element_type == elem);

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
    // Complex params: array type
    Type *arr_param = ast_create_array_type(&arena, params[0]);
    Type *complex_params[1] = {arr_param};
    Type *complex_fn = ast_create_function_type(&arena, ret, complex_params, 1);
    assert(complex_fn != NULL);
    assert(complex_fn->as.function.param_count == 1);
    assert(ast_type_equals(complex_fn->as.function.param_types[0], arr_param));
    // Empty params
    Type *fn_empty = ast_create_function_type(&arena, ret, NULL, 0);
    assert(fn_empty != NULL);
    assert(fn_empty->as.function.param_count == 0);
    assert(fn_empty->as.function.param_types == NULL);
    // NULL return
    Type *fn_null_ret = ast_create_function_type(&arena, NULL, params, 2);
    assert(fn_null_ret != NULL);
    assert(fn_null_ret->as.function.return_type == NULL);
    // NULL params with count > 0 (should handle gracefully by returning NULL)
    Type *fn_null_params = ast_create_function_type(&arena, ret, NULL, 2);
    assert(fn_null_params == NULL);
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

    // Nested array
    Type *nested_orig = ast_create_array_type(&arena, orig_arr);
    Type *nested_clone = ast_clone_type(&arena, nested_orig);
    assert(nested_clone != NULL);
    assert(nested_clone->as.array.element_type->kind == TYPE_ARRAY);
    assert(nested_clone->as.array.element_type->as.array.element_type->kind == TYPE_CHAR);

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

    // Function with complex param
    Type *complex_params[1] = {orig_arr};
    Type *complex_orig_fn = ast_create_function_type(&arena, ret, complex_params, 1);
    Type *complex_clone_fn = ast_clone_type(&arena, complex_orig_fn);
    assert(complex_clone_fn != NULL);
    assert(complex_clone_fn->as.function.param_types[0]->kind == TYPE_ARRAY);

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

    // All primitives
    Type *t_long = ast_create_primitive_type(&arena, TYPE_LONG);
    assert(ast_type_equals(t1, t_long) == 0);

    // Arrays
    Type *arr1 = ast_create_array_type(&arena, t1);
    Type *arr2 = ast_create_array_type(&arena, t2);
    Type *arr3 = ast_create_array_type(&arena, t3);
    assert(ast_type_equals(arr1, arr2) == 1);
    assert(ast_type_equals(arr1, arr3) == 0);

    // Nested arrays
    Type *nested1 = ast_create_array_type(&arena, arr1);
    Type *nested2 = ast_create_array_type(&arena, arr2);
    Type *nested3 = ast_create_array_type(&arena, arr1); // Same as nested1
    assert(ast_type_equals(nested1, nested2) == 1);
    assert(ast_type_equals(nested1, arr1) == 0); // Different depth
    assert(ast_type_equals(nested1, nested3) == 1);

    // Functions
    Type *params1[2] = {t1, t3};
    Type *fn1 = ast_create_function_type(&arena, t1, params1, 2);
    Type *params2[2] = {t2, t3};
    Type *fn2 = ast_create_function_type(&arena, t2, params2, 2);
    Type *params3[1] = {t1};
    Type *fn3 = ast_create_function_type(&arena, t1, params3, 1);
    assert(ast_type_equals(fn1, fn2) == 1);
    assert(ast_type_equals(fn1, fn3) == 0);

    // Function with different return
    Type *fn_diff_ret = ast_create_function_type(&arena, t3, params1, 2);
    assert(ast_type_equals(fn1, fn_diff_ret) == 0);

    // Function with different param count
    Type *fn_diff_count = ast_create_function_type(&arena, t1, params1, 1);
    assert(ast_type_equals(fn1, fn_diff_count) == 0);

    // Function with different param types
    Type *params_diff[2] = {t1, t1};
    Type *fn_diff_params = ast_create_function_type(&arena, t1, params_diff, 2);
    assert(ast_type_equals(fn1, fn_diff_params) == 0);

    // Empty functions
    Type *empty1 = ast_create_function_type(&arena, t1, NULL, 0);
    Type *empty2 = ast_create_function_type(&arena, t1, NULL, 0);
    assert(ast_type_equals(empty1, empty2) == 1);

    // NULL cases
    assert(ast_type_equals(NULL, NULL) == 1);
    assert(ast_type_equals(t1, NULL) == 0);
    assert(ast_type_equals(NULL, t1) == 0);

    cleanup_arena(&arena);
}

void test_ast_type_to_string()
{
    printf("Testing ast_type_to_string...\n");
    Arena arena;
    setup_arena(&arena);

    // Primitives
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_INT)), "int") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_LONG)), "long") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_DOUBLE)), "double") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_CHAR)), "char") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_STRING)), "string") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_BOOL)), "bool") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_VOID)), "void") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_NIL)), "nil") == 0);
    assert(strcmp(ast_type_to_string(&arena, ast_create_primitive_type(&arena, TYPE_ANY)), "any") == 0);

    // Array
    Type *arr = ast_create_array_type(&arena, ast_create_primitive_type(&arena, TYPE_CHAR));
    const char *actual = ast_type_to_string(&arena, arr);
    assert(strcmp(actual, "array of char") == 0);
    assert(strcmp(ast_type_to_string(&arena, arr), "array of char") == 0);

    // Nested array
    Type *nested_arr = ast_create_array_type(&arena, arr);
    assert(strcmp(ast_type_to_string(&arena, nested_arr), "array of array of char") == 0);

    // Function
    Type *params[1] = {ast_create_primitive_type(&arena, TYPE_BOOL)};
    Type *fn = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_STRING), params, 1);
    assert(strcmp(ast_type_to_string(&arena, fn), "function(bool) -> string") == 0);

    // Function with multiple params
    Type *params_multi[2] = {ast_create_primitive_type(&arena, TYPE_INT), ast_create_primitive_type(&arena, TYPE_DOUBLE)};
    Type *fn_multi = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_VOID), params_multi, 2);
    assert(strcmp(ast_type_to_string(&arena, fn_multi), "function(int, double) -> void") == 0);

    // Function with array param
    Type *params_arr[1] = {arr};
    Type *fn_arr = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_INT), params_arr, 1);
    assert(strcmp(ast_type_to_string(&arena, fn_arr), "function(array of char) -> int") == 0);

    // Empty function
    Type *fn_empty = ast_create_function_type(&arena, ast_create_primitive_type(&arena, TYPE_VOID), NULL, 0);
    assert(strcmp(ast_type_to_string(&arena, fn_empty), "function() -> void") == 0);

    // Unknown kind
    Type *unknown = arena_alloc(&arena, sizeof(Type));
    unknown->kind = -1; // Invalid
    assert(strcmp(ast_type_to_string(&arena, unknown), "unknown") == 0);

    // NULL
    assert(ast_type_to_string(&arena, NULL) == NULL);

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
    assert(tokens_equal(bin->token, loc));
    assert(bin->expr_type == NULL);

    // Different operators
    Expr *bin_minus = ast_create_binary_expr(&arena, left, TOKEN_MINUS, right, loc);
    assert(bin_minus->as.binary.operator == TOKEN_MINUS);

    Expr *bin_mult = ast_create_binary_expr(&arena, left, TOKEN_STAR, right, loc);
    assert(bin_mult->as.binary.operator == TOKEN_STAR);

    // NULL left/right
    assert(ast_create_binary_expr(&arena, NULL, TOKEN_PLUS, right, loc) == NULL);
    assert(ast_create_binary_expr(&arena, left, TOKEN_PLUS, NULL, loc) == NULL);
    assert(ast_create_binary_expr(&arena, NULL, TOKEN_PLUS, NULL, loc) == NULL);

    // NULL loc (code allows it, but token is const Token*)
    Expr *bin_null_loc = ast_create_binary_expr(&arena, left, TOKEN_PLUS, right, NULL);
    assert(bin_null_loc != NULL);
    assert(bin_null_loc->token == NULL);

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
    assert(tokens_equal(un->token, loc));

    // Different operators
    Expr *un_not = ast_create_unary_expr(&arena, TOKEN_BANG, operand, loc);
    assert(un_not->as.unary.operator == TOKEN_BANG);

    // NULL operand
    assert(ast_create_unary_expr(&arena, TOKEN_MINUS, NULL, loc) == NULL);

    // NULL loc
    Expr *un_null_loc = ast_create_unary_expr(&arena, TOKEN_MINUS, operand, NULL);
    assert(un_null_loc != NULL);
    assert(un_null_loc->token == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_literal_expr()
{
    printf("Testing ast_create_literal_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);

    // Int
    LiteralValue val_int = {.int_value = 42};
    Type *typ_int = ast_create_primitive_type(&arena, TYPE_INT);
    Expr *lit_int = ast_create_literal_expr(&arena, val_int, typ_int, false, loc);
    assert(lit_int != NULL);
    assert(lit_int->type == EXPR_LITERAL);
    assert(lit_int->as.literal.value.int_value == 42);
    assert(lit_int->as.literal.type == typ_int);
    assert(lit_int->as.literal.is_interpolated == false);
    assert(tokens_equal(lit_int->token, loc));

    // Double
    LiteralValue val_double = {.double_value = 3.14};
    Type *typ_double = ast_create_primitive_type(&arena, TYPE_DOUBLE);
    Expr *lit_double = ast_create_literal_expr(&arena, val_double, typ_double, false, loc);
    assert(lit_double->as.literal.value.double_value == 3.14);
    assert(lit_double->as.literal.type == typ_double);

    // Char
    LiteralValue val_char = {.char_value = 'a'};
    Type *typ_char = ast_create_primitive_type(&arena, TYPE_CHAR);
    Expr *lit_char = ast_create_literal_expr(&arena, val_char, typ_char, false, loc);
    assert(lit_char->as.literal.value.char_value == 'a');

    // String
    LiteralValue val_string = {.string_value = "hello"};
    Type *typ_string = ast_create_primitive_type(&arena, TYPE_STRING);
    Expr *lit_string = ast_create_literal_expr(&arena, val_string, typ_string, false, loc);
    assert(strcmp(lit_string->as.literal.value.string_value, "hello") == 0);

    // Bool
    LiteralValue val_bool = {.bool_value = true};
    Type *typ_bool = ast_create_primitive_type(&arena, TYPE_BOOL);
    Expr *lit_bool = ast_create_literal_expr(&arena, val_bool, typ_bool, false, loc);
    assert(lit_bool->as.literal.value.bool_value == true);

    // Interpolated
    Expr *lit_interp = ast_create_literal_expr(&arena, val_int, typ_int, true, loc);
    assert(lit_interp->as.literal.is_interpolated == true);

    // NULL type (code requires type, but test if NULL)
    Expr *lit_null_type = ast_create_literal_expr(&arena, val_int, NULL, false, loc);
    assert(lit_null_type == NULL);

    // NULL loc
    Expr *lit_null_loc = ast_create_literal_expr(&arena, val_int, typ_int, false, NULL);
    assert(lit_null_loc != NULL);
    assert(lit_null_loc->token == NULL);

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
    assert(tokens_equal(var->token, loc));

    // Empty name (length 0)
    Token empty_name = create_dummy_token(&arena, "");
    Expr *var_empty = ast_create_variable_expr(&arena, empty_name, loc);
    assert(var_empty != NULL);
    assert(var_empty->as.variable.name.length == 0);

    // NULL loc
    Expr *var_null_loc = ast_create_variable_expr(&arena, name, NULL);
    assert(var_null_loc != NULL);
    assert(var_null_loc->token == NULL);

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
    assert(tokens_equal(ass->token, loc));

    // NULL value
    Expr *ass_null_val = ast_create_assign_expr(&arena, name, NULL, loc);
    assert(ass_null_val == NULL); // Code allows NULL value

    // Empty name
    Token empty_name = create_dummy_token(&arena, "");
    Expr *ass_empty = ast_create_assign_expr(&arena, empty_name, val, loc);
    assert(ass_empty != NULL);
    assert(ass_empty->as.assign.name.length == 0);

    // NULL loc
    Expr *ass_null_loc = ast_create_assign_expr(&arena, name, val, NULL);
    assert(ass_null_loc != NULL);
    assert(ass_null_loc->token == NULL);

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
    assert(tokens_equal(call->token, loc));

    // Empty args
    Expr *call_empty = ast_create_call_expr(&arena, callee, NULL, 0, loc);
    assert(call_empty != NULL);
    assert(call_empty->as.call.arg_count == 0);
    assert(call_empty->as.call.arguments == NULL);

    // NULL callee
    assert(ast_create_call_expr(&arena, NULL, args, 2, loc) == NULL);

    // NULL args with count > 0 (code sets arguments to passed, even if NULL)
    Expr *call_null_args = ast_create_call_expr(&arena, callee, NULL, 2, loc);
    assert(call_null_args != NULL);
    assert(call_null_args->as.call.arguments == NULL);
    assert(call_null_args->as.call.arg_count == 2);

    // NULL loc
    Expr *call_null_loc = ast_create_call_expr(&arena, callee, args, 2, NULL);
    assert(call_null_loc != NULL);
    assert(call_null_loc->token == NULL);

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
    elems[0] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 1}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    elems[1] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 2}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    elems[2] = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 3}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *arr = ast_create_array_expr(&arena, elems, 3, loc);
    assert(arr != NULL);
    assert(arr->type == EXPR_ARRAY);
    assert(arr->as.array.element_count == 3);
    assert(arr->as.array.elements[0] == elems[0]);
    assert(arr->as.array.elements[1] == elems[1]);
    assert(arr->as.array.elements[2] == elems[2]);
    assert(tokens_equal(arr->token, loc));
    assert(arr->expr_type == NULL);

    // Empty array
    Expr *arr_empty = ast_create_array_expr(&arena, NULL, 0, loc);
    assert(arr_empty != NULL);
    assert(arr_empty->as.array.element_count == 0);
    assert(arr_empty->as.array.elements == NULL);

    // NULL elems with count > 0
    Expr *arr_null_elems = ast_create_array_expr(&arena, NULL, 3, loc);
    assert(arr_null_elems != NULL);
    assert(arr_null_elems->as.array.elements == NULL);
    assert(arr_null_elems->as.array.element_count == 3);

    // NULL loc
    Expr *arr_null_loc = ast_create_array_expr(&arena, elems, 3, NULL);
    assert(arr_null_loc != NULL);
    assert(arr_null_loc->token == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_array_access_expr()
{
    printf("Testing ast_create_array_access_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *array = ast_create_variable_expr(&arena, create_dummy_token(&arena, "arr"), loc);
    Expr *index = ast_create_literal_expr(&arena, (LiteralValue){.int_value = 0}, ast_create_primitive_type(&arena, TYPE_INT), false, loc);
    Expr *access = ast_create_array_access_expr(&arena, array, index, loc);
    assert(access != NULL);
    assert(access->type == EXPR_ARRAY_ACCESS);
    assert(access->as.array_access.array == array);
    assert(access->as.array_access.index == index);
    assert(tokens_equal(access->token, loc));
    assert(access->expr_type == NULL);

    // NULL array or index
    assert(ast_create_array_access_expr(&arena, NULL, index, loc) == NULL);
    assert(ast_create_array_access_expr(&arena, array, NULL, loc) == NULL);
    assert(ast_create_array_access_expr(&arena, NULL, NULL, loc) == NULL);

    // NULL loc
    Expr *access_null_loc = ast_create_array_access_expr(&arena, array, index, NULL);
    assert(access_null_loc != NULL);
    assert(access_null_loc->token == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_increment_expr()
{
    printf("Testing ast_create_increment_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *operand = ast_create_variable_expr(&arena, create_dummy_token(&arena, "i"), loc);
    Expr *inc = ast_create_increment_expr(&arena, operand, loc);
    assert(inc != NULL);
    assert(inc->type == EXPR_INCREMENT);
    assert(inc->as.operand == operand);
    assert(tokens_equal(inc->token, loc));
    assert(inc->expr_type == NULL);

    // NULL operand
    assert(ast_create_increment_expr(&arena, NULL, loc) == NULL);

    // NULL loc
    Expr *inc_null_loc = ast_create_increment_expr(&arena, operand, NULL);
    assert(inc_null_loc != NULL);
    assert(inc_null_loc->token == NULL);

    cleanup_arena(&arena);
}

void test_ast_create_decrement_expr()
{
    printf("Testing ast_create_decrement_expr...\n");
    Arena arena;
    setup_arena(&arena);

    Token temp_token = create_dummy_token(&arena, "loc");
    Token *loc = ast_clone_token(&arena, &temp_token);
    Expr *operand = ast_create_variable_expr(&arena, create_dummy_token(&arena, "i"), loc);
    Expr *dec = ast_create_decrement_expr(&arena, operand, loc);
    assert(dec != NULL);
    assert(dec->type == EXPR_DECREMENT);
    assert(dec->as.operand == operand);
    assert(tokens_equal(dec->token, loc));
    assert(dec->expr_type == NULL);

    // NULL operand
    assert(ast_create_decrement_expr(&arena, NULL, loc) == NULL);

    // NULL loc
    Expr *dec_null_loc = ast_create_decrement_expr(&arena, operand, NULL);
    assert(dec_null_loc != NULL);
    assert(dec_null_loc->token == NULL);

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
    parts[0] = ast_create_literal_expr(&arena, (LiteralValue){.string_value = "hello "}, ast_create_primitive_type(&arena, TYPE_STRING), true, loc);
    parts[1] = ast_create_variable_expr(&arena, create_dummy_token(&arena, "name"), loc);
    Expr *interp = ast_create_interpolated_expr(&arena, parts, 2, loc);
    assert(interp != NULL);
    assert(interp->type == EXPR_INTERPOLATED);
    assert(interp->as.interpol.part_count == 2);
    assert(interp->as.interpol.parts[0] == parts[0]);
    assert(interp->as.interpol.parts[1] == parts[1]);
    assert(tokens_equal(interp->token, loc));
    assert(interp->expr_type == NULL);

    // Empty parts
    Expr *interp_empty = ast_create_interpolated_expr(&arena, NULL, 0, loc);
    assert(interp_empty != NULL);
    assert(interp_empty->as.interpol.part_count == 0);
    assert(interp_empty->as.interpol.parts == NULL);

    // NULL parts with count > 0
    Expr *interp_null_parts = ast_create_interpolated_expr(&arena, NULL, 2, loc);
    assert(interp_null_parts != NULL);
    assert(interp_null_parts->as.interpol.parts == NULL);
    assert(interp_null_parts->as.interpol.part_count == 2);

    // NULL loc
    Expr *interp_null_loc = ast_create_interpolated_expr(&arena, parts, 2, NULL);
    assert(interp_null_loc != NULL);
    assert(interp_null_loc->token == NULL);

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
    Expr *comp = ast_create_comparison_expr(&arena, left, right, TOKEN_EQUAL_EQUAL, loc);
    assert(comp != NULL);
    assert(comp->type == EXPR_BINARY); // Since it's alias to binary
    assert(comp->as.binary.left == left);
    assert(comp->as.binary.right == right);
    assert(comp->as.binary.operator == TOKEN_EQUAL_EQUAL);

    // Different comparison types
    Expr *comp_gt = ast_create_comparison_expr(&arena, left, right, TOKEN_GREATER, loc);
    assert(comp_gt->as.binary.operator == TOKEN_GREATER);

    // NULL left/right
    assert(ast_create_comparison_expr(&arena, NULL, right, TOKEN_EQUAL_EQUAL, loc) == NULL);
    assert(ast_create_comparison_expr(&arena, left, NULL, TOKEN_EQUAL_EQUAL, loc) == NULL);

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
    Stmt *estmt = ast_create_expr_stmt(&arena, expr, loc);
    assert(estmt != NULL);
    assert(estmt->type == STMT_EXPR);
    assert(estmt->as.expression.expression == expr);
    assert(tokens_equal(estmt->token, loc));

    // NULL expr
    assert(ast_create_expr_stmt(&arena, NULL, loc) == NULL);

    // NULL loc
    Stmt *estmt_null_loc = ast_create_expr_stmt(&arena, expr, NULL);
    assert(estmt_null_loc != NULL);
    assert(estmt_null_loc->token == NULL);

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
    assert(tokens_equal(decl->token, loc));

    // No initializer
    Stmt *decl_no_init = ast_create_var_decl_stmt(&arena, name, typ, NULL, loc);
    assert(decl_no_init != NULL);
    assert(decl_no_init->as.var_decl.initializer == NULL);

    // NULL type
    assert(ast_create_var_decl_stmt(&arena, name, NULL, init, loc) == NULL);

    // Empty name
    Token empty_name = create_dummy_token(&arena, "");
    Stmt *decl_empty = ast_create_var_decl_stmt(&arena, empty_name, typ, init, loc);
    assert(decl_empty != NULL);
    assert(decl_empty->as.var_decl.name.length == 0);

    // NULL loc
    Stmt *decl_null_loc = ast_create_var_decl_stmt(&arena, name, typ, init, NULL);
    assert(decl_null_loc != NULL);
    assert(decl_null_loc->token == NULL);

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
    assert(fn->as.function.params[0].type->kind == TYPE_INT);
    assert(fn->as.function.return_type == ret);
    assert(fn->as.function.body_count == 1);
    assert(fn->as.function.body[0] == body[0]);
    assert(tokens_equal(fn->token, loc));

    // Empty params and body
    Stmt *fn_empty = ast_create_function_stmt(&arena, name, NULL, 0, ret, NULL, 0, loc);
    assert(fn_empty != NULL);
    assert(fn_empty->as.function.param_count == 0);
    assert(fn_empty->as.function.params == NULL);
    assert(fn_empty->as.function.body_count == 0);
    assert(fn_empty->as.function.body == NULL);

    // NULL return type
    Stmt *fn_null_ret = ast_create_function_stmt(&arena, name, params, 1, NULL, body, 1, loc);
    assert(fn_null_ret != NULL);
    assert(fn_null_ret->as.function.return_type == NULL);

    // NULL params with count > 0 (code allocates and copies, but type could be NULL)
    Parameter null_params[1] = {{.name = create_dummy_token(&arena, "p"), .type = NULL}};
    Stmt *fn_null_param_type = ast_create_function_stmt(&arena, name, null_params, 1, ret, body, 1, loc);
    assert(fn_null_param_type != NULL);
    assert(fn_null_param_type->as.function.params[0].type == NULL);

    // Empty name
    Token empty_name = create_dummy_token(&arena, "");
    Stmt *fn_empty_name = ast_create_function_stmt(&arena, empty_name, params, 1, ret, body, 1, loc);
    assert(fn_empty_name != NULL);
    assert(fn_empty_name->as.function.name.length == 0);

    // NULL loc
    Stmt *fn_null_loc = ast_create_function_stmt(&arena, name, params, 1, ret, body, 1, NULL);
    assert(fn_null_loc != NULL);
    assert(fn_null_loc->token == NULL);

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
    assert(tokens_equal(ret->token, loc));

    // No value
    Stmt *ret_no_val = ast_create_return_stmt(&arena, kw, NULL, loc);
    assert(ret_no_val != NULL);
    assert(ret_no_val->as.return_stmt.value == NULL);

    // Empty keyword (though unlikely)
    Token empty_kw = create_dummy_token(&arena, "");
    Stmt *ret_empty_kw = ast_create_return_stmt(&arena, empty_kw, val, loc);
    assert(ret_empty_kw != NULL);
    assert(ret_empty_kw->as.return_stmt.keyword.length == 0);

    // NULL loc
    Stmt *ret_null_loc = ast_create_return_stmt(&arena, kw, val, NULL);
    assert(ret_null_loc != NULL);
    assert(ret_null_loc->token == NULL);

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
    assert(tokens_equal(block->token, loc));

    // Empty block
    Stmt *block_empty = ast_create_block_stmt(&arena, NULL, 0, loc);
    assert(block_empty != NULL);
    assert(block_empty->as.block.count == 0);
    assert(block_empty->as.block.statements == NULL);

    // NULL statements with count > 0
    Stmt *block_null_stmts = ast_create_block_stmt(&arena, NULL, 2, loc);
    assert(block_null_stmts != NULL);
    assert(block_null_stmts->as.block.statements == NULL);
    assert(block_null_stmts->as.block.count == 2);

    // NULL loc
    Stmt *block_null_loc = ast_create_block_stmt(&arena, stmts, 2, NULL);
    assert(block_null_loc != NULL);
    assert(block_null_loc->token == NULL);

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
    assert(tokens_equal(if_stmt->token, loc));

    // No else
    Stmt *if_no_else = ast_create_if_stmt(&arena, cond, then, NULL, loc);
    assert(if_no_else != NULL);
    assert(if_no_else->as.if_stmt.else_branch == NULL);

    // NULL cond or then
    assert(ast_create_if_stmt(&arena, NULL, then, els, loc) == NULL);
    assert(ast_create_if_stmt(&arena, cond, NULL, els, loc) == NULL);
    assert(ast_create_if_stmt(&arena, NULL, NULL, els, loc) == NULL);

    // NULL loc
    Stmt *if_null_loc = ast_create_if_stmt(&arena, cond, then, els, NULL);
    assert(if_null_loc != NULL);
    assert(if_null_loc->token == NULL);

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
    assert(tokens_equal(wh->token, loc));

    // NULL cond or body
    assert(ast_create_while_stmt(&arena, NULL, body, loc) == NULL);
    assert(ast_create_while_stmt(&arena, cond, NULL, loc) == NULL);
    assert(ast_create_while_stmt(&arena, NULL, NULL, loc) == NULL);

    // NULL loc
    Stmt *wh_null_loc = ast_create_while_stmt(&arena, cond, body, NULL);
    assert(wh_null_loc != NULL);
    assert(wh_null_loc->token == NULL);

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
    assert(tokens_equal(fr->token, loc));

    // Optional parts
    Stmt *fr_partial = ast_create_for_stmt(&arena, NULL, NULL, NULL, body, loc);
    assert(fr_partial != NULL);
    assert(fr_partial->as.for_stmt.initializer == NULL);
    assert(fr_partial->as.for_stmt.condition == NULL);
    assert(fr_partial->as.for_stmt.increment == NULL);

    // NULL body
    assert(ast_create_for_stmt(&arena, init, cond, inc, NULL, loc) == NULL);

    // NULL loc
    Stmt *fr_null_loc = ast_create_for_stmt(&arena, init, cond, inc, body, NULL);
    assert(fr_null_loc != NULL);
    assert(fr_null_loc->token == NULL);

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
    assert(imp->as.import.module_name.length == 6);
    assert(tokens_equal(imp->token, loc));

    // Empty module name
    Token empty_mod = create_dummy_token(&arena, "");
    Stmt *imp_empty = ast_create_import_stmt(&arena, empty_mod, loc);
    assert(imp_empty != NULL);
    assert(imp_empty->as.import.module_name.length == 0);

    // NULL loc
    Stmt *imp_null_loc = ast_create_import_stmt(&arena, mod, NULL);
    assert(imp_null_loc != NULL);
    assert(imp_null_loc->token == NULL);

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

    // NULL filename (code allows, but filename is const char*)
    Module mod_null_file;
    ast_init_module(&arena, &mod_null_file, NULL);
    assert(mod_null_file.filename == NULL);

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
    int old_capacity = mod.capacity;
    for (int i = 1; i < 10; i++)
    {
        Stmt *s = ast_create_expr_stmt(&arena, ast_create_variable_expr(&arena, create_dummy_token(&arena, "y"), loc), loc);
        ast_module_add_statement(&arena, &mod, s);
        assert(mod.statements[i] == s);
    }
    assert(mod.count == 10);
    assert(mod.capacity > old_capacity); // Resized

    // NULL module or stmt
    ast_module_add_statement(&arena, NULL, s1);   // Nothing
    ast_module_add_statement(&arena, &mod, NULL); // Nothing, count unchanged
    assert(mod.count == 10);

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

    // Empty string
    Token empty_orig = create_dummy_token(&arena, "");
    Token *empty_clone = ast_clone_token(&arena, &empty_orig);
    assert(empty_clone != NULL);
    assert(empty_clone->length == 0);
    assert(strcmp(empty_clone->start, "") == 0);

    // Different type
    Token diff_type = orig;
    diff_type.type = TOKEN_STRING_LITERAL;
    Token *clone_diff = ast_clone_token(&arena, &diff_type);
    assert(clone_diff->type == TOKEN_STRING_LITERAL);

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
    ast_print_expr(&arena, expr, 0);

    Stmt *stmt = ast_create_if_stmt(&arena,
                                    expr,
                                    ast_create_block_stmt(&arena, NULL, 0, loc),
                                    NULL,
                                    loc);
    ast_print_stmt(&arena, stmt, 0);

    // NULL
    ast_print_expr(&arena, NULL, 0);
    ast_print_stmt(&arena, NULL, 0);

    // More complex expr
    Expr *lit = ast_create_literal_expr(&arena, (LiteralValue){.string_value = "test"}, ast_create_primitive_type(&arena, TYPE_STRING), true, loc);
    ast_print_expr(&arena, lit, 0);

    // Complex stmt
    Stmt *func = ast_create_function_stmt(&arena, create_dummy_token(&arena, "func"), NULL, 0, ast_create_primitive_type(&arena, TYPE_VOID), NULL, 0, loc);
    ast_print_stmt(&arena, func, 0);

    cleanup_arena(&arena);
}