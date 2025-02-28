/**
 * code_gen.h
 * Code generator for the compiler
 */

#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "ast.h"
#include "symbol_table.h"
#include <stdio.h>

typedef struct
{
    int label_count;
    SymbolTable *symbol_table;
    FILE *output;
    char *current_function;      // Current function name
    Type *current_return_type;   // Current function return type
    
    // Function context stack for handling nested expressions
    int function_stack_capacity;
    int function_stack_size;
    char **function_stack;       // Stack of function names
} CodeGen;

// Initialize and cleanup code generator
void init_code_gen(CodeGen *gen, SymbolTable *symbol_table, const char *output_file);
void code_gen_cleanup(CodeGen *gen);

// Generate assembly from AST
void generate_module(CodeGen *gen, Module *module);

// Generate statements
void generate_statement(CodeGen *gen, Stmt *stmt);
void generate_expression_statement(CodeGen *gen, ExprStmt *stmt);
void generate_var_declaration(CodeGen *gen, VarDeclStmt *stmt);
void generate_function(CodeGen *gen, FunctionStmt *stmt);
void generate_return_statement(CodeGen *gen, ReturnStmt *stmt);
void generate_block(CodeGen *gen, BlockStmt *stmt);
void generate_if_statement(CodeGen *gen, IfStmt *stmt);
void generate_while_statement(CodeGen *gen, WhileStmt *stmt);
void generate_for_statement(CodeGen *gen, ForStmt *stmt);

// Generate expressions
void generate_expression(CodeGen *gen, Expr *expr);
void generate_binary_expression(CodeGen *gen, BinaryExpr *expr);
void generate_unary_expression(CodeGen *gen, UnaryExpr *expr);
void generate_literal_expression(CodeGen *gen, LiteralExpr *expr);
void generate_variable_expression(CodeGen *gen, VariableExpr *expr);
void generate_assign_expression(CodeGen *gen, AssignExpr *expr);
void generate_call_expression(CodeGen *gen, CallExpr *expr);
void generate_array_expression(CodeGen *gen, ArrayExpr *expr);
void generate_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr);
void generate_increment_expression(CodeGen *gen, Expr *expr);
void generate_decrement_expression(CodeGen *gen, Expr *expr);

// Create a new label
int new_label(CodeGen *gen);

// Generate x86-64 assembly
void generate_string_literal(CodeGen *gen, const char *string, int label);
void generate_data_section(CodeGen *gen);
void generate_text_section(CodeGen *gen);
void generate_prologue(CodeGen *gen, const char *function_name);
void generate_epilogue(CodeGen *gen);

#endif // CODE_GEN_H