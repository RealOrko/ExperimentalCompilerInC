/**
 * symbol_table.c
 * Implementation of the symbol table
 */

 #include "symbol_table.h"
 #include <stdlib.h>
 #include <string.h>
 
 SymbolTable* create_symbol_table() {
     SymbolTable* table = malloc(sizeof(SymbolTable));
     table->current = NULL;
     
     // Start with a global scope
     push_scope(table);
     
     return table;
 }
 
 void free_scope(Scope* scope) {
     if (scope == NULL) return;
     
     Symbol* symbol = scope->symbols;
     while (symbol != NULL) {
         Symbol* next = symbol->next;
         free_type(symbol->type);
         free(symbol);
         symbol = next;
     }
     
     free(scope);
 }
 
 void free_symbol_table(SymbolTable* table) {
     if (table == NULL) return;
     
     Scope* scope = table->current;
     while (scope != NULL) {
         Scope* enclosing = scope->enclosing;
         free_scope(scope);
         scope = enclosing;
     }
     
     free(table);
 }
 
 void push_scope(SymbolTable* table) {
     Scope* scope = malloc(sizeof(Scope));
     scope->symbols = NULL;
     scope->enclosing = table->current;
     table->current = scope;
 }
 
 void pop_scope(SymbolTable* table) {
     if (table->current == NULL) return;
     
     Scope* scope = table->current;
     table->current = scope->enclosing;
     
     free_scope(scope);
 }
 
 static int tokens_equal(Token a, Token b) {
     if (a.length != b.length) return 0;
     return memcmp(a.start, b.start, a.length) == 0;
 }
 
 void add_symbol(SymbolTable* table, Token name, Type* type) {
     // Check if symbol already exists in current scope
     Symbol* existing = lookup_symbol_current(table, name);
     if (existing != NULL) {
         // Symbol already exists, just update its type
         free_type(existing->type);
         existing->type = type;
         return;
     }
     
     // Create new symbol
     Symbol* symbol = malloc(sizeof(Symbol));
     symbol->name = name;
     symbol->type = type;
     
     // Add to current scope
     symbol->next = table->current->symbols;
     table->current->symbols = symbol;
 }
 
 Symbol* lookup_symbol_current(SymbolTable* table, Token name) {
     if (table->current == NULL) return NULL;
     
     Symbol* symbol = table->current->symbols;
     while (symbol != NULL) {
         if (tokens_equal(symbol->name, name)) {
             return symbol;
         }
         symbol = symbol->next;
     }
     
     return NULL;
 }
 
 Symbol* lookup_symbol(SymbolTable* table, Token name) {
     Scope* scope = table->current;
     while (scope != NULL) {
         Symbol* symbol = scope->symbols;
         while (symbol != NULL) {
             if (tokens_equal(symbol->name, name)) {
                 return symbol;
             }
             symbol = symbol->next;
         }
         
         scope = scope->enclosing;
     }
     
     return NULL;
 }