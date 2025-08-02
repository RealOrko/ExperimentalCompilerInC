#ifndef CODE_GEN_H
#define CODE_GEN_H

#include "arena.h"
#include "ast.h"
#include "symbol_table.h"
#include <stdio.h>

typedef struct
{
    int label_count;
    Arena *arena;
    SymbolTable *symbol_table;
    FILE *output;
    char *current_function;
    Type *current_return_type;
} CodeGen;

void code_gen_init(Arena *arena, CodeGen *gen, SymbolTable *symbol_table, const char *output_file);
void code_gen_cleanup(CodeGen *gen);
int code_gen_new_label(CodeGen *gen);
void code_gen_module(CodeGen *gen, Module *module);
void code_gen_statement(CodeGen *gen, Stmt *stmt);
void code_gen_block(CodeGen *gen, BlockStmt *stmt);
void code_gen_function(CodeGen *gen, FunctionStmt *stmt);
void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt);
void code_gen_if_statement(CodeGen *gen, IfStmt *stmt);
void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt);
void code_gen_for_statement(CodeGen *gen, ForStmt *stmt);

#endif