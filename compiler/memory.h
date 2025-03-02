/**
 * memory.h
 * Enhanced memory management system for compiler with thread safety, hierarchy,
 * pool allocation, and improved debugging
 */

 #ifndef COMPILER_MEMORY_H
 #define COMPILER_MEMORY_H
 
 #include "debug.h"
 #include <stddef.h>
 #include <stdint.h>
 #include <pthread.h>
 
 // Memory context types
 typedef enum {
     MEMORY_CONTEXT_GLOBAL,    // Long-lived global allocations
     MEMORY_CONTEXT_PARSING,   // Temporary memory during parsing
     MEMORY_CONTEXT_TYPE_CHECK,// Memory used during type checking
     MEMORY_CONTEXT_CODE_GEN,  // Memory for code generation
     MEMORY_CONTEXT_SYMBOL_TABLE, // Memory for symbol table
     MEMORY_CONTEXT_AST,       // Memory for AST nodes
     MEMORY_CONTEXT_MAX        // Sentinel value
 } MemoryContextType;
 
 // Memory allocation strategies
 typedef enum {
     ALLOC_STRATEGY_ARENA,     // Pre-allocated block strategy
     ALLOC_STRATEGY_STANDARD,  // Standard malloc/free
     ALLOC_STRATEGY_POOL       // Fixed-size object pool
 } AllocationStrategy;
 
 // Forward declarations for opaque structures
 typedef struct MemoryBlock MemoryBlock;
 typedef struct PoolBlock PoolBlock;
 typedef struct MemoryContext MemoryContext;
 
 // Allocation tracking for debugging
 #ifdef DEBUG_MEMORY
 typedef struct AllocationInfo {
     void *ptr;                 // Allocated memory pointer
     size_t size;               // Size of allocation
     const char *file;          // Source file
     int line;                  // Source line
     struct AllocationInfo *next;
 } AllocationInfo;
 #endif
 
 // Memory block structure
 struct MemoryBlock {
     void *memory;              // The actual memory buffer
     size_t used;               // How much is currently used
     size_t capacity;           // Total capacity of this block
     MemoryBlock *next;         // Next block in the chain
     int has_free_space;        // Quick flag for blocks with free space
 };
 
 // Pool block for fixed-size allocations
 struct PoolBlock {
     void *memory;              // The memory pool
     size_t element_size;       // Size of each element
     size_t capacity;           // Number of elements in the pool
     uint8_t *free_bitmap;      // Bitmap tracking free elements
     size_t free_count;         // Count of free elements
     PoolBlock *next;           // Next pool block
 };
 
 // Memory context configuration
 typedef struct {
     MemoryContextType type;    // Type identifier for this context
     AllocationStrategy strategy; // Allocation strategy to use
     size_t initial_block_size; // Initial block size (0 = default)
     size_t max_total_memory;   // Maximum memory allowed (0 = default)
     size_t pool_element_size;  // For pool allocation, size of each element
     int thread_safe;           // Whether to use thread safety mechanisms
 } MemoryContextConfig;
 
 // Memory context for tracking allocations
 struct MemoryContext {
     MemoryContextType type;
     AllocationStrategy strategy;
     
     // Hierarchy
     MemoryContext *parent;     // Parent context, if any
     MemoryContext *first_child; // First child context
     MemoryContext *next_sibling; // Next sibling context
 
     // Arena allocation fields
     MemoryBlock *first_block;
     MemoryBlock *current_block;
     
     // Pool allocation fields
     PoolBlock *first_pool;
     
     // Memory tracking
     size_t total_allocated;
     size_t total_used;
     size_t max_memory_limit;
     
     // Thread safety
     pthread_mutex_t mutex;
     int thread_safe;
     
     // Debugging
 #ifdef DEBUG_MEMORY
     AllocationInfo *allocations; // Linked list of tracked allocations
 #endif
 
     // Optional cleanup function
     void (*cleanup)(struct MemoryContext *context);
 };
 
 // Global memory management functions
 int memory_init(void);
 void memory_shutdown(void);
 int memory_add_leak_detection(void);
 void memory_print_leaks(void);
 
 // Context management
 MemoryContext* create_memory_context(MemoryContextConfig config);
 MemoryContext* create_child_context(MemoryContext *parent, MemoryContextConfig config);
 void destroy_memory_context(MemoryContext *context);
 void destroy_child_contexts(MemoryContext *parent);
 
 // Thread safety
 void memory_context_lock(MemoryContext *context);
 void memory_context_unlock(MemoryContext *context);
 
 // Allocation functions
 void* memory_alloc(MemoryContext *context, size_t size);
 void* memory_calloc(MemoryContext *context, size_t count, size_t size);
 char* memory_strdup(MemoryContext *context, const char *str);
 void* memory_realloc(MemoryContext *context, void *ptr, size_t new_size);
 
 // Pool allocation
 void* memory_pool_alloc(MemoryContext *context);
 void memory_pool_free(MemoryContext *context, void *ptr);
 
 // Free functions (for standard allocation strategy only)
 void memory_free(MemoryContext *context, void *ptr);
 
 // Block management
 void* memory_alloc_best_fit(MemoryContext *context, size_t size);
 void memory_compact(MemoryContext *context);
 
 // Utility functions
 void memory_reset_context(MemoryContext *context);
 size_t memory_context_usage(MemoryContext *context);
 size_t memory_context_capacity(MemoryContext *context);
 void memory_print_stats(MemoryContext *context);
 void memory_print_detailed_stats(MemoryContext *context, int verbose);
 
 // Debug helpers
 #ifdef DEBUG_MEMORY
 #define memory_alloc(ctx, size) memory_alloc_debug(ctx, size, __FILE__, __LINE__)
 #define memory_calloc(ctx, count, size) memory_calloc_debug(ctx, count, size, __FILE__, __LINE__)
 #define memory_strdup(ctx, str) memory_strdup_debug(ctx, str, __FILE__, __LINE__)
 #define memory_realloc(ctx, ptr, size) memory_realloc_debug(ctx, ptr, size, __FILE__, __LINE__)
 #define memory_pool_alloc(ctx) memory_pool_alloc_debug(ctx, __FILE__, __LINE__)
 
 void* memory_alloc_debug(MemoryContext *context, size_t size, const char *file, int line);
 void* memory_calloc_debug(MemoryContext *context, size_t count, size_t size, const char *file, int line);
 char* memory_strdup_debug(MemoryContext *context, const char *str, const char *file, int line);
 void* memory_realloc_debug(MemoryContext *context, void *ptr, size_t new_size, const char *file, int line);
 void* memory_pool_alloc_debug(MemoryContext *context, const char *file, int line);
 #endif
 
 // AST node helpers
 #define AST_ALLOC(ctx, type) ((type*)memory_calloc(ctx, 1, sizeof(type)))
 
 #endif // COMPILER_MEMORY_H