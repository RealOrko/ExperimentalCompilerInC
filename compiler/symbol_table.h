#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "token.h"
#include "ast.h"

#define OFFSET_ALIGNMENT 8
#define CALLEE_SAVED_SPACE 16  
#define LOCAL_BASE_OFFSET (8 + CALLEE_SAVED_SPACE)  
#define PARAM_BASE_OFFSET 16  
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct Type Type;

typedef enum
{
    SYMBOL_GLOBAL,
    SYMBOL_LOCAL,
    SYMBOL_PARAM
} SymbolKind;

typedef struct Symbol
{
    Token name;
    Type *type;
    SymbolKind kind;
    int offset;
    struct Symbol *next;
} Symbol;

typedef struct Scope
{
    Symbol *symbols;
    struct Scope *enclosing;
    int next_local_offset;
    int next_param_offset;
} Scope;

typedef struct {
    Scope *current;
    Scope *global_scope;
    Scope **scopes;
    int scopes_count;
    int scopes_capacity;
} SymbolTable;

void symbol_table_print(SymbolTable *table, const char *where);

SymbolTable *symbol_table_init();
void symbol_table_cleanup(SymbolTable *table);

void symbol_table_push_scope(SymbolTable *table);
void symbol_table_pop_scope(SymbolTable *table);
void symbol_table_begin_function_scope(SymbolTable *table);

void symbol_table_add_symbol(SymbolTable *table, Token name, Type *type);
void symbol_table_add_symbol_with_kind(SymbolTable *table, Token name, Type *type, SymbolKind kind);
Symbol *symbol_table_lookup_symbol(SymbolTable *table, Token name);
Symbol *symbol_table_lookup_symbol_current(SymbolTable *table, Token name);
int symbol_table_get_symbol_offset(SymbolTable *table, Token name);

#endif