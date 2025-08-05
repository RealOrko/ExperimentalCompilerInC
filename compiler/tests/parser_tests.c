// tests/parser_tests.c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../arena.h"
#include "../lexer.h"
#include "../parser.h"
#include "../ast.h"
#include "../debug.h"
#include "../symbol_table.h"

void test_simple_program_parsing() {
    Arena arena;
    arena_init(&arena, 4096);

    const char *source =
        "fn add(x:int, y:int):int =>\n"
        "  return x + y\n"
        "fn main():void =>\n"
        "  var z:int = add(6, 2)\n"
        "  print($\"The answer is {z}\\n\")\n";

    Lexer lexer;
    lexer_init(&arena, &lexer, source, "test.sn");

    Parser parser;
    parser_init(&arena, &parser, &lexer);

    Module *module = parser_execute(&parser, "test.sn");

    // Basic assertions on the parsed module
    assert(module != NULL);
    assert(module->count == 2);  // Two function declarations: add and main
    assert(strcmp(module->filename, "test.sn") == 0);

    // First statement: fn add
    Stmt *add_fn = module->statements[0];
    assert(add_fn != NULL);
    assert(add_fn->type == STMT_FUNCTION);
    assert(strcmp(add_fn->as.function.name.start, "add") == 0);
    assert(add_fn->as.function.param_count == 2);
    assert(strcmp(add_fn->as.function.params[0].name.start, "x") == 0);
    assert(add_fn->as.function.params[0].type->kind == TYPE_INT);
    assert(strcmp(add_fn->as.function.params[1].name.start, "y") == 0);
    assert(add_fn->as.function.params[1].type->kind == TYPE_INT);
    assert(add_fn->as.function.return_type->kind == TYPE_INT);
    assert(add_fn->as.function.body_count == 1);  // One return statement in body

    Stmt *add_body = add_fn->as.function.body[0];
    assert(add_body->type == STMT_RETURN);
    assert(add_body->as.return_stmt.value->type == EXPR_BINARY);
    assert(add_body->as.return_stmt.value->as.binary.operator == TOKEN_PLUS);
    assert(add_body->as.return_stmt.value->as.binary.left->type == EXPR_VARIABLE);
    assert(strcmp(add_body->as.return_stmt.value->as.binary.left->as.variable.name.start, "x") == 0);
    assert(add_body->as.return_stmt.value->as.binary.right->type == EXPR_VARIABLE);
    assert(strcmp(add_body->as.return_stmt.value->as.binary.right->as.variable.name.start, "y") == 0);

    // Second statement: fn main
    Stmt *main_fn = module->statements[1];
    assert(main_fn != NULL);
    assert(main_fn->type == STMT_FUNCTION);
    assert(strcmp(main_fn->as.function.name.start, "main") == 0);
    assert(main_fn->as.function.param_count == 0);
    assert(main_fn->as.function.return_type->kind == TYPE_VOID);
    assert(main_fn->as.function.body_count == 2);  // Var decl and print call

    Stmt *var_decl = main_fn->as.function.body[0];
    assert(var_decl->type == STMT_VAR_DECL);
    assert(strcmp(var_decl->as.var_decl.name.start, "z") == 0);
    assert(var_decl->as.var_decl.type->kind == TYPE_INT);
    assert(var_decl->as.var_decl.initializer->type == EXPR_CALL);
    assert(var_decl->as.var_decl.initializer->as.call.callee->type == EXPR_VARIABLE);
    assert(strcmp(var_decl->as.var_decl.initializer->as.call.callee->as.variable.name.start, "add") == 0);
    assert(var_decl->as.var_decl.initializer->as.call.arg_count == 2);
    assert(var_decl->as.var_decl.initializer->as.call.arguments[0]->type == EXPR_LITERAL);
    assert(var_decl->as.var_decl.initializer->as.call.arguments[0]->as.literal.value.int_value == 6);
    assert(var_decl->as.var_decl.initializer->as.call.arguments[1]->type == EXPR_LITERAL);
    assert(var_decl->as.var_decl.initializer->as.call.arguments[1]->as.literal.value.int_value == 2);

    Stmt *print_stmt = main_fn->as.function.body[1];
    assert(print_stmt->type == STMT_EXPR);
    assert(print_stmt->as.expression.expression->type == EXPR_CALL);
    assert(print_stmt->as.expression.expression->as.call.callee->type == EXPR_VARIABLE);
    assert(strcmp(print_stmt->as.expression.expression->as.call.callee->as.variable.name.start, "print") == 0);
    assert(print_stmt->as.expression.expression->as.call.arg_count == 1);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->type == EXPR_INTERPOLATED);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.part_count == 3);  // "The answer is ", {z}, "\n"
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[0]->as.literal.value.string_value, "The answer is ") == 0);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[1]->as.variable.name.start, "z") == 0);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[2]->as.literal.value.string_value, "\n") == 0);

    // Cleanup
    parser_cleanup(&parser);
    lexer_cleanup(&lexer);
    arena_free(&arena);
}
