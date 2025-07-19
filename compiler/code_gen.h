/**
 * code_gen.h
 * Code generator for the compiler
 */

#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "ast.h"
#include "symbol_table.h"
#include <stdio.h>

typedef struct StringLiteral
{
    const char *string;
    int label;
    struct StringLiteral *next;
} StringLiteral;

static StringLiteral *string_literals = NULL;

typedef struct
{
    int label_count;
    SymbolTable *symbol_table;
    FILE *output;
    char *current_function;    // Current function name
    Type *current_return_type; // Current function return type

    // Function context stack for handling nested expressions
    int function_stack_capacity;
    int function_stack_size;
    char **function_stack; // Stack of function names
} CodeGen;

// Initialize and cleanup code generator
void code_gen_init(CodeGen *gen, SymbolTable *symbol_table, const char *output_file);
void code_gen_cleanup(CodeGen *gen);

// Generate assembly from AST
void code_gen_module(CodeGen *gen, Module *module);

// Generate statements
void code_gen_statement(CodeGen *gen, Stmt *stmt);
void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt);
void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt);
void code_gen_function(CodeGen *gen, FunctionStmt *stmt);
void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt);
void code_gen_block(CodeGen *gen, BlockStmt *stmt);
void code_gen_if_statement(CodeGen *gen, IfStmt *stmt);
void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt);
void code_gen_for_statement(CodeGen *gen, ForStmt *stmt);

// Generate expressions
void code_gen_expression(CodeGen *gen, Expr *expr);
void code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr);
void code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr);
void code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr);
void code_gen_variable_expression(CodeGen *gen, VariableExpr *expr);
void code_gen_assign_expression(CodeGen *gen, AssignExpr *expr);
void code_gen_call_expression(CodeGen *gen, Expr *expr);
void code_gen_array_expression(CodeGen *gen, ArrayExpr *expr);
void code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr);
void code_gen_increment_expression(CodeGen *gen, Expr *expr);
void code_gen_decrement_expression(CodeGen *gen, Expr *expr);

// Create a new label
int code_gen_new_label(CodeGen *gen);

// Generate x86-64 assembly
void code_gen_string_literal(CodeGen *gen, const char *string, int label);
void code_gen_data_section(CodeGen *gen);
void code_gen_text_section(CodeGen *gen);
void code_gen_prologue(CodeGen *gen, const char *function_name);
void code_gen_epilogue(CodeGen *gen);

#endif // CODE_GEN_H