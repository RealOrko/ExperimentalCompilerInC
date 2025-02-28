/**
 * symbol_table.h
 * Symbol table for tracking variables and functions
 */

 #ifndef SYMBOL_TABLE_H
 #define SYMBOL_TABLE_H
 
 #include "token.h"
 #include "ast.h"
 
 // Forward declaration for circular references
 typedef struct Type Type;
 
 // Symbol entry
 typedef struct Symbol {
     Token name;
     Type* type;
     struct Symbol* next;
 } Symbol;
 
 // Scope
 typedef struct Scope {
     Symbol* symbols;
     struct Scope* enclosing;
 } Scope;
 
 // Symbol table
 typedef struct {
     Scope* current;
 } SymbolTable;
 
 // Symbol table functions
 SymbolTable* create_symbol_table();
 void free_symbol_table(SymbolTable* table);
 
 // Scope functions
 void push_scope(SymbolTable* table);
 void pop_scope(SymbolTable* table);
 
 // Symbol management
 Type* clone_type(Type* type);
 void add_symbol(SymbolTable* table, Token name, Type* type);
 Symbol* lookup_symbol(SymbolTable* table, Token name);
 Symbol* lookup_symbol_current(SymbolTable* table, Token name);
 
 #endif // SYMBOL_TABLE_H