/**
 * symbol_table.c
 * Implementation of the symbol table
 */

 #include "symbol_table.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 
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
 
/**
 * Create a deep clone of a Type structure
 * This ensures each component has its own copy to avoid double free issues
 */
Type* clone_type(Type* type) {
    if (type == NULL) return NULL;
    
    Type* clone = malloc(sizeof(Type));
    if (clone == NULL) {
        fprintf(stderr, "Error: Out of memory when cloning type\n");
        exit(1);
    }
    
    // Copy the kind
    clone->kind = type->kind;
    
    // Handle the specific type data based on kind
    switch (type->kind) {
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_DOUBLE:
        case TYPE_CHAR:
        case TYPE_STRING:
        case TYPE_BOOL:
        case TYPE_VOID:
        case TYPE_NIL:
            // For primitive types, no additional data to copy
            break;
            
        case TYPE_ARRAY:
            // For arrays, recursively clone the element type
            clone->as.array.element_type = clone_type(type->as.array.element_type);
            break;
            
        case TYPE_FUNCTION:
            // For functions, clone the return type and all parameter types
            clone->as.function.return_type = clone_type(type->as.function.return_type);
            clone->as.function.param_count = type->as.function.param_count;
            
            if (type->as.function.param_count > 0) {
                // Allocate and clone each parameter type
                clone->as.function.param_types = malloc(sizeof(Type*) * type->as.function.param_count);
                if (clone->as.function.param_types == NULL) {
                    fprintf(stderr, "Error: Out of memory when cloning function param types\n");
                    exit(1);
                }
                
                for (int i = 0; i < type->as.function.param_count; i++) {
                    clone->as.function.param_types[i] = clone_type(type->as.function.param_types[i]);
                }
            } else {
                clone->as.function.param_types = NULL;
            }
            break;
    }
    
    return clone;
}

/**
 * Add a symbol to the symbol table with a cloned type
 */
void add_symbol(SymbolTable* table, Token name, Type* type) {
    // Check if symbol already exists in current scope
    Symbol* existing = lookup_symbol_current(table, name);
    if (existing != NULL) {
        // Symbol already exists, just update its type
        free_type(existing->type);
        existing->type = clone_type(type);  // Use clone instead of original
        return;
    }
    
    // Create new symbol
    Symbol* symbol = malloc(sizeof(Symbol));
    if (symbol == NULL) {
        fprintf(stderr, "Error: Out of memory when creating symbol\n");
        exit(1);
    }
    
    symbol->name = name;
    symbol->type = clone_type(type);  // Use clone instead of original
    
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