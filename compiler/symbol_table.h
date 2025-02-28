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
} SymbolTable;

// debug
void debug_print_symbol_table(SymbolTable *table, const char *where);

// Symbol table functions
SymbolTable *create_symbol_table();
void free_symbol_table(SymbolTable *table);

// Scope functions
void push_scope(SymbolTable *table);
void pop_scope(SymbolTable *table);
void begin_function_scope(SymbolTable *table); // New function for initializing function scope offsets

// Symbol management
Type *clone_type(Type *type);
void add_symbol(SymbolTable *table, Token name, Type *type);
void add_symbol_with_kind(SymbolTable *table, Token name, Type *type, SymbolKind kind); // New function
Symbol *lookup_symbol(SymbolTable *table, Token name);
Symbol *lookup_symbol_current(SymbolTable *table, Token name);
int get_symbol_offset(SymbolTable *table, Token name); // New function

#endif // SYMBOL_TABLE_H