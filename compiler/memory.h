/**
 * memory.h
 * Generalized memory management for the compiler with debug support
 */

 #ifndef COMPILER_MEMORY_H
 #define COMPILER_MEMORY_H
 
 #include "debug.h"
 #include <stddef.h>
 #include <stdint.h>
 
 // Memory context types
 typedef enum {
     MEMORY_CONTEXT_GLOBAL,    // Long-lived global allocations
     MEMORY_CONTEXT_PARSING,   // Temporary memory during parsing
     MEMORY_CONTEXT_TYPE_CHECK,// Memory used during type checking
     MEMORY_CONTEXT_CODE_GEN,  // Memory for code generation
     MEMORY_CONTEXT_SYMBOL_TABLE, // Memory for symbol table
     MEMORY_CONTEXT_MAX        // Sentinel value
 } MemoryContextType;
 
 // Memory allocation strategies
 typedef enum {
     ALLOC_STRATEGY_ARENA,     // Pre-allocated block strategy
     ALLOC_STRATEGY_STANDARD,  // Standard malloc/free
     ALLOC_STRATEGY_POOL       // Fixed-size object pool
 } AllocationStrategy;
 
 // Memory block structure
 typedef struct MemoryBlock {
     void *memory;
     size_t used;
     size_t capacity;
     struct MemoryBlock *next;
 } MemoryBlock;
 
 // Memory context configuration
 typedef struct {
     MemoryContextType type;
     AllocationStrategy strategy;
     size_t initial_block_size;
     size_t max_total_memory;
 } MemoryContextConfig;
 
 // Memory context for tracking allocations
 typedef struct MemoryContext {
     MemoryContextType type;
     AllocationStrategy strategy;
     
     // Arena allocation fields
     MemoryBlock *first_block;
     MemoryBlock *current_block;
     
     // Memory tracking
     size_t total_allocated;
     size_t total_used;
     size_t max_memory_limit;
 
     // Optional cleanup function
     void (*cleanup)(struct MemoryContext *context);
 } MemoryContext;
 
 // Global memory management functions
 int memory_init(void);
 void memory_shutdown(void);
 
 // Context management
 MemoryContext* create_memory_context(MemoryContextConfig config);
 void destroy_memory_context(MemoryContext *context);
 
 // Allocation functions
 void* memory_alloc(MemoryContext *context, size_t size);
 void* memory_calloc(MemoryContext *context, size_t count, size_t size);
 char* memory_strdup(MemoryContext *context, const char *str);
 
 // Utility functions
 void memory_reset_context(MemoryContext *context);
 size_t memory_context_usage(MemoryContext *context);
 void memory_print_stats(MemoryContext *context);
 
 #endif // COMPILER_MEMORY_H