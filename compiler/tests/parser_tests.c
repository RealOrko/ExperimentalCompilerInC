// Updated parser_tests.c with additional debugging for the two failing tests
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

static void setup_parser(Arena *arena, Lexer *lexer, Parser *parser, SymbolTable *symbol_table, const char *source)
{
    arena_init(arena, 4096);
    lexer_init(arena, lexer, source, "test.sn");
    symbol_table_init(arena, symbol_table);
    parser_init(arena, parser, lexer, symbol_table);
}

static void cleanup_parser(Arena *arena, Lexer *lexer, Parser *parser, SymbolTable *symbol_table)
{
    parser_cleanup(parser);
    lexer_cleanup(lexer);
    symbol_table_cleanup(symbol_table);
    arena_free(arena);
}

void test_empty_program_parsing()
{
    printf("Testing parser_execute empty program...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    setup_parser(&arena, &lexer, &parser, &symbol_table, "");

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 0);
    assert(strcmp(module->filename, "test.sn") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_var_decl_parsing()
{
    printf("Testing parser_execute variable declaration...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "var x:int = 42\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *stmt = module->statements[0];
    assert(stmt->type == STMT_VAR_DECL);
    assert(strcmp(stmt->as.var_decl.name.start, "x") == 0);
    assert(stmt->as.var_decl.type->kind == TYPE_INT);
    assert(stmt->as.var_decl.initializer->type == EXPR_LITERAL);
    assert(stmt->as.var_decl.initializer->as.literal.value.int_value == 42);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_function_no_params_parsing()
{
    printf("Testing parser_execute function no params...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn main():void =>\n"
        "  print(\"hello\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "main") == 0);
    assert(fn->as.function.param_count == 0);
    assert(fn->as.function.return_type->kind == TYPE_VOID);
    assert(fn->as.function.body_count == 1);
    Stmt *print_stmt = fn->as.function.body[0];
    assert(print_stmt->type == STMT_EXPR);
    assert(print_stmt->as.expression.expression->type == EXPR_CALL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.callee->as.variable.name.start, "print") == 0);
    assert(print_stmt->as.expression.expression->as.call.arg_count == 1);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->type == EXPR_LITERAL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.literal.value.string_value, "hello\n") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_if_statement_parsing()
{
    printf("Testing parser_execute if statement...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "if x > 0 =>\n"
        "  print(\"positive\\n\")\n"
        "else =>\n"
        "  print(\"non-positive\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    // Added debugging logs to inspect parser state and AST
    if (module)
    {
        if (module->count > 0)
        {
            ast_print_stmt(&arena, module->statements[0], 0);
            Stmt *if_stmt = module->statements[0];
            if (if_stmt->type == STMT_IF)
            {
                if (if_stmt->as.if_stmt.condition->type == EXPR_BINARY)
                {
                }
                if (if_stmt->as.if_stmt.then_branch->type == STMT_BLOCK)
                {
                }
                if (if_stmt->as.if_stmt.else_branch->type == STMT_BLOCK)
                {
                }
            }
        }
        else
        {
            DEBUG_WARNING("No statements parsed in module.");
        }
    }
    else
    {
        DEBUG_ERROR("Module is NULL after parsing.");
    }

    assert(module != NULL);
    assert(module->count == 1);
    Stmt *if_stmt = module->statements[0];
    assert(if_stmt->type == STMT_IF);
    assert(if_stmt->as.if_stmt.condition->type == EXPR_BINARY);
    assert(if_stmt->as.if_stmt.condition->as.binary.operator == TOKEN_GREATER);
    assert(strcmp(if_stmt->as.if_stmt.condition->as.binary.left->as.variable.name.start, "x") == 0);
    assert(if_stmt->as.if_stmt.condition->as.binary.right->as.literal.value.int_value == 0);
    assert(if_stmt->as.if_stmt.then_branch->type == STMT_BLOCK);
    assert(if_stmt->as.if_stmt.then_branch->as.block.count == 1);
    assert(if_stmt->as.if_stmt.else_branch->type == STMT_BLOCK);
    assert(if_stmt->as.if_stmt.else_branch->as.block.count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_while_loop_parsing()
{
    printf("Testing parser_execute while loop...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "while i < 10 =>\n"
        "  i = i + 1\n"
        "  print(i)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *while_stmt = module->statements[0];
    assert(while_stmt->type == STMT_WHILE);
    assert(while_stmt->as.while_stmt.condition->type == EXPR_BINARY);
    assert(while_stmt->as.while_stmt.condition->as.binary.operator == TOKEN_LESS);
    assert(strcmp(while_stmt->as.while_stmt.condition->as.binary.left->as.variable.name.start, "i") == 0);
    assert(while_stmt->as.while_stmt.condition->as.binary.right->as.literal.value.int_value == 10);
    assert(while_stmt->as.while_stmt.body->type == STMT_BLOCK);
    assert(while_stmt->as.while_stmt.body->as.block.count == 2);
    Stmt *assign = while_stmt->as.while_stmt.body->as.block.statements[0];
    assert(assign->type == STMT_EXPR);
    assert(assign->as.expression.expression->type == EXPR_ASSIGN);
    assert(strcmp(assign->as.expression.expression->as.assign.name.start, "i") == 0);
    assert(assign->as.expression.expression->as.assign.value->type == EXPR_BINARY);
    assert(assign->as.expression.expression->as.assign.value->as.binary.operator == TOKEN_PLUS);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_for_loop_parsing()
{
    printf("Testing parser_execute for loop...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "for var j:int = 0; j < 5; j++ =>\n"
        "  print(j)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *for_stmt = module->statements[0];
    assert(for_stmt->type == STMT_FOR);
    assert(for_stmt->as.for_stmt.initializer->type == STMT_VAR_DECL);
    assert(strcmp(for_stmt->as.for_stmt.initializer->as.var_decl.name.start, "j") == 0);
    assert(for_stmt->as.for_stmt.initializer->as.var_decl.type->kind == TYPE_INT);
    assert(for_stmt->as.for_stmt.initializer->as.var_decl.initializer->as.literal.value.int_value == 0);
    assert(for_stmt->as.for_stmt.condition->type == EXPR_BINARY);
    assert(for_stmt->as.for_stmt.condition->as.binary.operator == TOKEN_LESS);
    assert(for_stmt->as.for_stmt.increment->type == EXPR_INCREMENT);
    assert(strcmp(for_stmt->as.for_stmt.increment->as.operand->as.variable.name.start, "j") == 0);
    assert(for_stmt->as.for_stmt.body->type == STMT_BLOCK);
    assert(for_stmt->as.for_stmt.body->as.block.count == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_interpolated_string_parsing()
{
    printf("Testing parser_execute interpolated string...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source = "print($\"Value is {x} and {y * 2}\\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *print_stmt = module->statements[0];
    assert(print_stmt->type == STMT_EXPR);
    assert(print_stmt->as.expression.expression->type == EXPR_CALL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.callee->as.variable.name.start, "print") == 0);
    assert(print_stmt->as.expression.expression->as.call.arg_count == 1);
    Expr *arg = print_stmt->as.expression.expression->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 5);
    assert(arg->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[0]->as.literal.value.string_value, "Value is ") == 0);
    assert(arg->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(strcmp(arg->as.interpol.parts[1]->as.variable.name.start, "x") == 0);
    assert(arg->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[2]->as.literal.value.string_value, " and ") == 0);
    assert(arg->as.interpol.parts[3]->type == EXPR_BINARY);
    assert(arg->as.interpol.parts[3]->as.binary.operator == TOKEN_STAR);
    assert(strcmp(arg->as.interpol.parts[3]->as.binary.left->as.variable.name.start, "y") == 0);
    assert(arg->as.interpol.parts[3]->as.binary.right->as.literal.value.int_value == 2);
    assert(arg->as.interpol.parts[4]->type == EXPR_LITERAL);
    assert(strcmp(arg->as.interpol.parts[4]->as.literal.value.string_value, "\n") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_literal_types_parsing()
{
    printf("Testing parser_execute various literals...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "var i:int = 42\n"
        "var d:double = 3.14\n"
        "var c:char = 'a'\n"
        "var s:str = \"hello\"\n"
        "var b:bool = true\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    // Added debugging logs to inspect parser state and AST
    if (module)
    {
        if (module->count > 0)
        {
            ast_print_stmt(&arena, module->statements[4], 0);  // Print AST for the bool declaration
            Stmt *stmt5 = module->statements[4];
            if (stmt5->type == STMT_VAR_DECL)
            {
                if (stmt5->as.var_decl.initializer)
                {
                    if (stmt5->as.var_decl.initializer->type == EXPR_LITERAL)
                    {
                        printf("Literal type: %s\n", ast_type_to_string(&arena, stmt5->as.var_decl.initializer->as.literal.type));
                        printf("Bool value: %d\n", stmt5->as.var_decl.initializer->as.literal.value.bool_value);
                        if (stmt5->as.var_decl.initializer->token)
                        {
                            printf("Lexeme: %.*s\n", stmt5->as.var_decl.initializer->token->length, stmt5->as.var_decl.initializer->token->start);
                        }
                    }
                }
            }
        }
        else
        {
            DEBUG_WARNING("No statements parsed in module.");
        }
    }
    else
    {
        DEBUG_ERROR("Module is NULL after parsing.");
    }

    assert(module != NULL);
    assert(module->count == 5);

    Stmt *stmt1 = module->statements[0];
    assert(stmt1->type == STMT_VAR_DECL);
    assert(stmt1->as.var_decl.initializer->as.literal.value.int_value == 42);

    Stmt *stmt2 = module->statements[1];
    assert(stmt2->type == STMT_VAR_DECL);
    // Note: Floating point comparison may need epsilon, but assuming exact for test
    assert(stmt2->as.var_decl.initializer->as.literal.value.double_value == 3.14);

    Stmt *stmt3 = module->statements[2];
    assert(stmt3->type == STMT_VAR_DECL);
    assert(stmt3->as.var_decl.initializer->as.literal.value.char_value == 'a');

    Stmt *stmt4 = module->statements[3];
    assert(stmt4->type == STMT_VAR_DECL);
    assert(strcmp(stmt4->as.var_decl.initializer->as.literal.value.string_value, "hello") == 0);

    Stmt *stmt5 = module->statements[4];
    assert(stmt5->type == STMT_VAR_DECL);
    assert(stmt5->as.var_decl.initializer->as.literal.value.bool_value == 1);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_recursive_function_parsing()
{
    printf("Testing parser_execute recursive function...\n");

    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn factorial(n:int):int =>\n"
        "  if n <= 1 =>\n"
        "    return 1\n"
        "  return n * factorial(n - 1)\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 1);
    Stmt *fn = module->statements[0];
    assert(fn->type == STMT_FUNCTION);
    assert(strcmp(fn->as.function.name.start, "factorial") == 0);
    assert(fn->as.function.param_count == 1);
    assert(strcmp(fn->as.function.params[0].name.start, "n") == 0);
    assert(fn->as.function.params[0].type->kind == TYPE_INT);
    assert(fn->as.function.return_type->kind == TYPE_INT);
    assert(fn->as.function.body_count == 2);
    Stmt *if_stmt = fn->as.function.body[0];
    assert(if_stmt->type == STMT_IF);
    assert(if_stmt->as.if_stmt.condition->as.binary.operator == TOKEN_LESS_EQUAL);
    assert(if_stmt->as.if_stmt.then_branch->as.block.count == 1);
    assert(if_stmt->as.if_stmt.then_branch->as.block.statements[0]->type == STMT_RETURN);
    Stmt *return_stmt = fn->as.function.body[1];
    assert(return_stmt->type == STMT_RETURN);
    assert(return_stmt->as.return_stmt.value->type == EXPR_BINARY);
    assert(return_stmt->as.return_stmt.value->as.binary.operator == TOKEN_STAR);
    assert(return_stmt->as.return_stmt.value->as.binary.right->type == EXPR_CALL);
    assert(strcmp(return_stmt->as.return_stmt.value->as.binary.right->as.call.callee->as.variable.name.start, "factorial") == 0);

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_full_program_parsing()
{
    printf("Testing parser_execute full program...\n");
    Arena arena;
    Lexer lexer;
    Parser parser;
    SymbolTable symbol_table;
    const char *source =
        "fn factorial(n: int): int =>\n"
        "  print($\"factorial: n={n}\\n\")\n"
        "  if n <= 1 =>\n"
        "    print($\"factorial: n <= 1 returning 1\\n\")\n"
        "    return 1\n"
        "  var j: int = n * factorial(n - 1)\n"
        "  print($\"factorial: j={j}\\n\")\n"
        "  return j\n"
        "fn is_prime(num: int): bool =>\n"
        "  if num <= 1 =>\n"
        "    print($\"is_prime: num={num}\\n\")\n"
        "    return false\n"
        "  var i: int = 2\n"
        "  print($\"is_prime: i={i}\\n\")\n"
        "  while i * i <= num =>\n"
        "    if num % i == 0 =>\n"
        "      print($\"is_prime: num % i == 0, returning false\\n\")\n"
        "      return false\n"
        "    i = i + 1\n"
        "    print($\"is_prime: i={i} (after increment)\\n\")\n"
        "  return true\n"
        "fn repeat_string(text: str, count: int): str =>\n"
        "  var result: str = \"\"\n"
        "  for var j: int = 0; j < count; j++ =>\n"
        "    print($\"repeat_string: j={j}\\n\")\n"
        "    print($\"repeat_string: count={count}\\n\")\n"
        "    result = result + text\n"
        "  return result\n"
        "fn main(): void =>\n"
        "  print(\"Starting main method ... \\n\")\n"
        "  var num: int = 5\n"
        "  var fact: int = factorial(num)\n"
        "  print($\"Factorial of {num} is {fact}\\n\")\n"
        "  if is_prime(7) =>\n"
        "    print(\"7 is prime\\n\")\n"
        "  else =>\n"
        "    print(\"7 is not prime\\n\")\n"
        "  var repeated: str = repeat_string(\"hello \", 3)\n"
        "  print(repeated + \"world!\\n\")\n"
        "  var sum: int = 0\n"
        "  for var k: int = 1; k <= 10; k++ =>\n"
        "    sum = sum + k\n"
        "  print($\"Sum 1 to 10: {sum}\\n\")\n"
        "  var pi_approx: double = 3.14159\n"
        "  print($\"Pi approx: {pi_approx}\\n\")\n"
        "  var ch: char = 'A'\n"
        "  print($\"Char: {ch}\\n\")\n"
        "  var flag: bool = true\n"
        "  print($\"Flag: {flag}\\n\")\n"
        "  print(\"Complete main method ... \\n\")\n";
    setup_parser(&arena, &lexer, &parser, &symbol_table, source);

    Module *module = parser_execute(&parser, "test.sn");

    // Added debugging logs to inspect parser state and AST
    if (module)
    {
        for (int i = 0; i < module->count; i++)
        {
            ast_print_stmt(&arena, module->statements[i], 0);
        }
    }
    else
    {
        DEBUG_ERROR("Module is NULL after parsing.");
    }

    assert(module != NULL);
    assert(module->count == 4); // Four functions: factorial, is_prime, repeat_string, main

    // factorial
    Stmt *fact_fn = module->statements[0];
    assert(fact_fn->type == STMT_FUNCTION);
    assert(strcmp(fact_fn->as.function.name.start, "factorial") == 0);
    assert(fact_fn->as.function.param_count == 1);
    assert(fact_fn->as.function.return_type->kind == TYPE_INT);
    // Corrected body_count to 5 (print, if, var, print, return)
    assert(fact_fn->as.function.body_count == 5);
    assert(fact_fn->as.function.body[0]->type == STMT_EXPR); // print
    assert(fact_fn->as.function.body[1]->type == STMT_IF);
    assert(fact_fn->as.function.body[2]->type == STMT_VAR_DECL); // var j
    assert(fact_fn->as.function.body[3]->type == STMT_EXPR);     // print
    assert(fact_fn->as.function.body[4]->type == STMT_RETURN);   // return j
    // Added debug for factorial body

    // To keep it concise, assert key parts
    assert(fact_fn->as.function.body[0]->type == STMT_EXPR); // print
    assert(fact_fn->as.function.body[1]->type == STMT_IF);
    assert(fact_fn->as.function.body[2]->type == STMT_VAR_DECL); // var j
    assert(fact_fn->as.function.body[3]->type == STMT_EXPR);     // print
    assert(fact_fn->as.function.body[4]->type == STMT_RETURN);   // return j
    // Since exhaustive, but to avoid too long, assert module count and function names
    Stmt *prime_fn = module->statements[1];
    assert(strcmp(prime_fn->as.function.name.start, "is_prime") == 0);
    assert(prime_fn->as.function.return_type->kind == TYPE_BOOL);

    Stmt *repeat_fn = module->statements[2];
    assert(strcmp(repeat_fn->as.function.name.start, "repeat_string") == 0);
    assert(repeat_fn->as.function.return_type->kind == TYPE_STRING);

    Stmt *main_fn = module->statements[3];
    assert(strcmp(main_fn->as.function.name.start, "main") == 0);
    assert(main_fn->as.function.return_type->kind == TYPE_VOID);
    // Count body statements, e.g., 20+

    // Assert one interpolated in main
    Stmt *print_fact = main_fn->as.function.body[3]; // print Factorial of...
    assert(print_fact->type == STMT_EXPR);
    Expr *call = print_fact->as.expression.expression;
    assert(call->type == EXPR_CALL);
    Expr *arg = call->as.call.arguments[0];
    assert(arg->type == EXPR_INTERPOLATED);
    assert(arg->as.interpol.part_count == 5); // "Factorial of ", {num}, " is ", {fact}, "\n"

    cleanup_parser(&arena, &lexer, &parser, &symbol_table);
}

void test_simple_program_parsing()
{
    // Existing test, kept as is
    printf("Testing parser_execute simple program...\n");

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

    SymbolTable symbol_table;
    symbol_table_init(&arena, &symbol_table);

    Parser parser;
    parser_init(&arena, &parser, &lexer, &symbol_table);

    Module *module = parser_execute(&parser, "test.sn");

    // Basic assertions on the parsed module
    if (module)
    {
    }
    assert(module != NULL);
    assert(module->count == 2); // Two function declarations: add and main
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
    assert(add_fn->as.function.body_count == 1); // One return statement in body

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
    assert(main_fn->as.function.body_count == 2); // Var decl and print call

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
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.part_count == 3); // "The answer is ", {z}, "\n"
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[0]->type == EXPR_LITERAL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[0]->as.literal.value.string_value, "The answer is ") == 0);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[1]->type == EXPR_VARIABLE);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[1]->as.variable.name.start, "z") == 0);
    assert(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[2]->type == EXPR_LITERAL);
    assert(strcmp(print_stmt->as.expression.expression->as.call.arguments[0]->as.interpol.parts[2]->as.literal.value.string_value, "\n") == 0);

    // Cleanup
    parser_cleanup(&parser);
    lexer_cleanup(&lexer);
    symbol_table_cleanup(&symbol_table);
    arena_free(&arena);
}