/**
 * symbol_table.c
 * Implementation of the symbol table with stack offset tracking
 */

 #include "symbol_table.h"
 #include "debug.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 
 // x86_64 stack layout constants
 #define PARAM_BASE_OFFSET 16 // First parameter is at rbp-16
 #define LOCAL_BASE_OFFSET 8  // First local is at rbp-8
 #define OFFSET_ALIGNMENT 8   // All variables are 8-byte aligned
 
 // New helper function to determine type size
 int get_type_size(Type *type)
 {
     switch (type->kind)
     {
     case TYPE_INT:
     case TYPE_LONG:   return 8;
     case TYPE_DOUBLE: return 8;
     case TYPE_CHAR:   return 1;
     case TYPE_BOOL:   return 1;
     case TYPE_STRING: return 8;  // Pointer size
     default:          return 8;  // Default to pointer size
     }
 }
 
 // Helper to represent a token as a string safely
 static char *token_to_string(Token token)
 {
     static char buf[256];
     int len = token.length < 255 ? token.length : 255;
     strncpy(buf, token.start, len);
     buf[len] = '\0';
     return buf;
 }
 
 // Print the entire symbol table state for debugging
 void symbol_table_print(SymbolTable *table, const char *where)
 {
     DEBUG_VERBOSE("==== SYMBOL TABLE DUMP (%s) ====", where);
 
     if (!table || !table->current)
     {
         DEBUG_VERBOSE("  [Empty symbol table or no current scope]");
         return;
     }
 
     int scope_level = 0;
     Scope *scope = table->current;
 
     while (scope)
     {
         DEBUG_VERBOSE("  Scope Level %d:", scope_level);
         DEBUG_VERBOSE("    next_local_offset: %d, next_param_offset: %d",
                 scope->next_local_offset, scope->next_param_offset);
 
         Symbol *symbol = scope->symbols;
         if (!symbol)
         {
             DEBUG_VERBOSE("    [No symbols in this scope]");
         }
 
         while (symbol)
         {
             DEBUG_VERBOSE("    Symbol: '%s', Type: %s, Kind: %d, Offset: %d",
                     token_to_string(symbol->name),
                     ast_type_to_string(symbol->type),
                     symbol->kind,
                     symbol->offset);
             symbol = symbol->next;
         }
 
         scope = scope->enclosing;
         scope_level++;
     }
 
     DEBUG_VERBOSE("====================================");
 }
 
 SymbolTable *symbol_table_init()
 {
     SymbolTable *table = malloc(sizeof(SymbolTable));
     if (table == NULL)
     {
         DEBUG_ERROR("Out of memory creating symbol table");
         exit(1);
     }
     table->current = NULL;
 
     // Start with a global scope
     symbol_table_push_scope(table);
 
     return table;
 }
 
 void free_scope(Scope *scope)
 {
     if (scope == NULL)
         return;
 
     Symbol *symbol = scope->symbols;
     while (symbol != NULL)
     {
         Symbol *next = symbol->next;
         ast_free_type(symbol->type);
         free(symbol);
         symbol = next;
     }
 
     free(scope);
 }
 
 void symbol_table_cleanup(SymbolTable *table)
 {
     if (table == NULL)
         return;
 
     Scope *scope = table->current;
     while (scope != NULL)
     {
         Scope *enclosing = scope->enclosing;
         free_scope(scope);
         scope = enclosing;
     }
 
     free(table);
 }
 
 void symbol_table_push_scope(SymbolTable *table)
 {
     Scope *scope = malloc(sizeof(Scope));
     if (scope == NULL)
     {
         DEBUG_ERROR("Out of memory creating scope");
         exit(1);
     }
 
     scope->symbols = NULL;
     scope->enclosing = table->current;
     scope->next_local_offset = LOCAL_BASE_OFFSET;
     scope->next_param_offset = PARAM_BASE_OFFSET;
 
     table->current = scope;
 }
 
 // Initialize a function scope with proper offset tracking
 void symbol_table_begin_function_scope(SymbolTable *table)
 {
     symbol_table_push_scope(table);
     // Reset the offsets for a new function
     table->current->next_local_offset = LOCAL_BASE_OFFSET;
     table->current->next_param_offset = PARAM_BASE_OFFSET;
 }
 
 void symbol_table_pop_scope(SymbolTable *table)
 {
     if (table->current == NULL)
         return;
 
     Scope *scope = table->current;
     table->current = scope->enclosing;
 
     free_scope(scope);
 }
 
 static int tokens_equal(Token a, Token b)
 {
     if (a.length != b.length) {
         char a_str[256], b_str[256];
         int a_len = a.length < 255 ? a.length : 255;
         int b_len = b.length < 255 ? b.length : 255;
         
         strncpy(a_str, a.start, a_len);
         a_str[a_len] = '\0';
         strncpy(b_str, b.start, b_len);
         b_str[b_len] = '\0';
         
         DEBUG_VERBOSE("Token length mismatch: '%s'(%d) vs '%s'(%d)", 
                 a_str, a.length, b_str, b.length);
         return 0;
     }
     
     // Check if the tokens have the same memory address
     if (a.start == b.start) {
         DEBUG_VERBOSE("Token address match at %p", (void*)a.start);
         return 1;
     }
     
     // Memory compare
     int result = memcmp(a.start, b.start, a.length);
     
     char a_str[256], b_str[256];
     int a_len = a.length < 255 ? a.length : 255;
     int b_len = b.length < 255 ? b.length : 255;
     
     strncpy(a_str, a.start, a_len);
     a_str[a_len] = '\0';
     strncpy(b_str, b.start, b_len);
     b_str[b_len] = '\0';
     
     if (result == 0) {
         DEBUG_VERBOSE("Token content match: '%s' == '%s'", a_str, b_str);
         return 1;
     } else {
         DEBUG_VERBOSE("Token content mismatch: '%s' != '%s'", a_str, b_str);
         return 0;
     }
 }
 
 /**
  * Create a deep clone of a Type structure
  * This ensures each component has its own copy to avoid double free issues
  */
 Type *symbol_table_clone_type(Type *type)
 {
     if (type == NULL)
         return NULL;
 
     Type *clone = malloc(sizeof(Type));
     if (clone == NULL)
     {
         DEBUG_ERROR("Out of memory when cloning type");
         exit(1);
     }
 
     // Copy the kind
     clone->kind = type->kind;
     clone->should_free = type->should_free;
 
     // Handle the specific type data based on kind
     switch (type->kind)
     {
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
         clone->as.array.element_type = symbol_table_clone_type(type->as.array.element_type);
         break;
 
     case TYPE_FUNCTION:
         // For functions, clone the return type and all parameter types
         clone->as.function.return_type = symbol_table_clone_type(type->as.function.return_type);
         clone->as.function.param_count = type->as.function.param_count;
 
         if (type->as.function.param_count > 0)
         {
             // Allocate and clone each parameter type
             clone->as.function.param_types = malloc(sizeof(Type *) * type->as.function.param_count);
             if (clone->as.function.param_types == NULL)
             {
                 DEBUG_ERROR("Out of memory when cloning function param types");
                 exit(1);
             }
 
             for (int i = 0; i < type->as.function.param_count; i++)
             {
                 clone->as.function.param_types[i] = symbol_table_clone_type(type->as.function.param_types[i]);
             }
         }
         else
         {
             clone->as.function.param_types = NULL;
         }
         break;
     }
 
     return clone;
 }
 
 /**
  * Add a symbol to the symbol table with a specific kind
  */
 void symbol_table_add_symbol_with_kind(SymbolTable *table, Token name, Type *type, SymbolKind kind)
 {
     if (table->current == NULL)
     {
         DEBUG_ERROR("No active scope when adding symbol");
         return;
     }
 
     // Check if symbol already exists in current scope
     Symbol *existing = symbol_table_lookup_symbol_current(table, name);
     if (existing != NULL)
     {
         // Symbol already exists, just update its type
         ast_free_type(existing->type);
         existing->type = symbol_table_clone_type(type); // Use clone instead of original
         // Don't change the offset or kind of an existing symbol
         return;
     }
 
     // Create new symbol
     Symbol *symbol = malloc(sizeof(Symbol));
     if (symbol == NULL)
     {
         DEBUG_ERROR("Out of memory when creating symbol");
         exit(1);
     }
 
     symbol->name = name;
     symbol->type = symbol_table_clone_type(type); // Use clone instead of original
     symbol->kind = kind;
 
     // Determine offset based on symbol kind
     if (kind == SYMBOL_PARAM)
     {
         // Parameters are allocated from rbp-16 down
         symbol->offset = table->current->next_param_offset;
         table->current->next_param_offset += OFFSET_ALIGNMENT;
     }
     else if (kind == SYMBOL_LOCAL)
     {
         // Local variables are allocated from rbp-8 down
         symbol->offset = table->current->next_local_offset;
         table->current->next_local_offset += OFFSET_ALIGNMENT;
     }
     else
     {
         // Globals don't have stack offsets
         symbol->offset = 0;
     }
 
     // Debug output
     char temp[256];
     int name_len = name.length < 255 ? name.length : 255;
     strncpy(temp, name.start, name_len);
     temp[name_len] = '\0';
     DEBUG_VERBOSE("Added symbol '%s' with kind %d, offset %d",
             temp, kind, symbol->offset);
 
     // Add to current scope
     symbol->next = table->current->symbols;
     table->current->symbols = symbol;
 
     // Smarter offset calculation
     int base_offset = (kind == SYMBOL_PARAM) ? PARAM_BASE_OFFSET : LOCAL_BASE_OFFSET;
     symbol->offset = table->current->next_local_offset;
     
     // Align offsets based on type size
     int type_size = get_type_size(type);
     table->current->next_local_offset += ((type_size + 7) / 8) * 8;
 }
 
 /**
  * Add a symbol with default kind (local)
  */
 void symbol_table_add_symbol(SymbolTable *table, Token name, Type *type)
 {
     symbol_table_add_symbol_with_kind(table, name, type, SYMBOL_LOCAL);
 }
 
 Symbol *symbol_table_lookup_symbol_current(SymbolTable *table, Token name)
 {
     if (table->current == NULL)
         return NULL;
 
     Symbol *symbol = table->current->symbols;
     while (symbol != NULL)
     {
         if (tokens_equal(symbol->name, name))
         {
             return symbol;
         }
         symbol = symbol->next;
     }
 
     return NULL;
 }
 
 Symbol *symbol_table_lookup_symbol(SymbolTable *table, Token name)
 {
     if (!table || !table->current) {
         DEBUG_VERBOSE("Null table or current scope in lookup_symbol");
         return NULL;
     }
     
     // Extract name as string for debugging and fallback matching
     char name_str[256];
     int name_len = name.length < 255 ? name.length : 255;
     strncpy(name_str, name.start, name_len);
     name_str[name_len] = '\0';
 
     DEBUG_VERBOSE("Looking up symbol '%s' at address %p, length %d",
             name_str, (void *)name.start, name.length);
 
     Scope *scope = table->current;
     int scope_level = 0;
 
     while (scope != NULL)
     {
         DEBUG_VERBOSE("  Checking scope level %d", scope_level);
 
         Symbol *symbol = scope->symbols;
         while (symbol != NULL)
         {
             // Get symbol name as string
             char sym_name[256];
             int sym_len = symbol->name.length < 255 ? symbol->name.length : 255;
             strncpy(sym_name, symbol->name.start, sym_len);
             sym_name[sym_len] = '\0';
 
             DEBUG_VERBOSE("    Symbol '%s' at address %p, length %d",
                     sym_name, (void *)symbol->name.start, symbol->name.length);
 
             // Step 1: Try exact token comparison
             if (symbol->name.start == name.start && symbol->name.length == name.length) {
                 DEBUG_VERBOSE("Found symbol '%s' in scope level %d (direct pointer match)",
                         sym_name, scope_level);
                 return symbol;
             }
             
             // Step 2: Try content comparison
             if (symbol->name.length == name.length && 
                 memcmp(symbol->name.start, name.start, name.length) == 0) {
                 DEBUG_VERBOSE("Found symbol '%s' in scope level %d (content match)",
                         sym_name, scope_level);
                 return symbol;
             }
 
             // Step 3: Fall back to string comparison
             if (strcmp(sym_name, name_str) == 0) {
                 DEBUG_VERBOSE("Found symbol '%s' by string comparison in scope level %d",
                         sym_name, scope_level);
                 return symbol;
             }
 
             symbol = symbol->next;
         }
 
         scope = scope->enclosing;
         scope_level++;
     }
 
     DEBUG_VERBOSE("Symbol '%s' not found in any scope", name_str);
     return NULL;
 }
 
 /**
  * Get the stack offset for a symbol
  * Returns -1 if the symbol is not found
  */
 int symbol_table_get_symbol_offset(SymbolTable *table, Token name)
 {
     Symbol *symbol = symbol_table_lookup_symbol(table, name);
     if (symbol == NULL)
     {
         // For debugging, print the variable we're looking for
         char temp[256];
         int name_len = name.length < 255 ? name.length : 255;
         strncpy(temp, name.start, name_len);
         temp[name_len] = '\0';
         DEBUG_ERROR("Symbol not found in get_symbol_offset: '%s'", temp);
         return -1;
     }
 
     return symbol->offset;
 }