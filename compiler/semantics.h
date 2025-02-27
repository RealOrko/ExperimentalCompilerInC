/**
 * semantics.h - Semantic analyzer for the custom language
 */

 #ifndef SEMANTICS_H
 #define SEMANTICS_H
 
 #include "parser.h"
 
 // Symbol table entry
 typedef struct SymbolEntry {
     char* name;
     AstNode* node;
     struct SymbolEntry* next;
 } SymbolEntry;
 
 // Symbol table
 struct SymbolTable {
     SymbolEntry* entries;
     struct SymbolTable* parent;
 };
 
 // Create a new symbol table with given parent
 SymbolTable* create_symbol_table(SymbolTable* parent);
 
 // Define a symbol in the table
 void define_symbol(SymbolTable* table, const char* name, AstNode* node);
 
 // Look up a symbol in the table hierarchy
 SymbolEntry* resolve_symbol(SymbolTable* table, const char* name);
 
 // Free a symbol table and all its entries
 void symbol_table_free(SymbolTable* table);
 
 // Perform semantic analysis on an AST
 bool analyze_ast(AstNode* ast);
 
 #endif /* SEMANTICS_H */