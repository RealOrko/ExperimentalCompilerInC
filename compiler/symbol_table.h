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
 
 // Symbol kind for memory allocation strategy
 typedef enum
 {
     SYMBOL_GLOBAL, // Global variable
     SYMBOL_LOCAL,  // Local variable
     SYMBOL_PARAM   // Function parameter
 } SymbolKind;
 
 // Symbol entry
 typedef struct Symbol
 {
     Token name;
     Type *type;
     SymbolKind kind; // Symbol kind (global, local, param)
     int offset;      // Stack offset (for locals and params)
     struct Symbol *next;
 } Symbol;
 
 // Scope
 typedef struct Scope
 {
     Symbol *symbols;
     struct Scope *enclosing;
     int next_local_offset; // Next available offset for local variables
     int next_param_offset; // Next available offset for parameters
 } Scope;
 
 // Symbol table
 typedef struct
 {
     Scope *current;
     Scope* global_scope;
 } SymbolTable;
 
 // debug
 void symbol_table_print(SymbolTable *table, const char *where);
 
 // Symbol table functions
 SymbolTable *symbol_table_init();
 void symbol_table_cleanup(SymbolTable *table);
 
 // Scope functions
 void symbol_table_push_scope(SymbolTable *table);
 void symbol_table_pop_scope(SymbolTable *table);
 void symbol_table_begin_function_scope(SymbolTable *table); // New function for initializing function scope offsets
 
 // Symbol management
 Type *symbol_table_clone_type(Type *type);
 void symbol_table_add_symbol(SymbolTable *table, Token name, Type *type);
 void symbol_table_add_symbol_with_kind(SymbolTable *table, Token name, Type *type, SymbolKind kind); // New function
 Symbol *symbol_table_lookup_symbol(SymbolTable *table, Token name);
 Symbol *symbol_table_lookup_symbol_current(SymbolTable *table, Token name);
 int symbol_table_get_symbol_offset(SymbolTable *table, Token name); // New function
 
 #endif // SYMBOL_TABLE_H