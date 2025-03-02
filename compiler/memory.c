/**
 * memory.c
 * Implementation of generalized memory management with debug support
 */

 #include "memory.h"
 #include "debug.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 
 // Default configuration constants
 #define DEFAULT_BLOCK_SIZE 4096
 #define DEFAULT_MAX_MEMORY (1024 * 1024 * 10)  // 10MB per context
 
 // Align memory to 8-byte boundary
 #define MEMORY_ALIGN(size) (((size) + 7) & ~0x7)
 
 // Global memory contexts
 static MemoryContext *g_memory_contexts[MEMORY_CONTEXT_MAX];
 
 // Create a new memory block
 static MemoryBlock* create_memory_block(size_t size) {
     MemoryBlock *block = malloc(sizeof(MemoryBlock));
     if (!block) {
         DEBUG_ERROR("Failed to allocate memory block descriptor");
         return NULL;
     }
 
     // Ensure minimum block size
     size = size < DEFAULT_BLOCK_SIZE ? DEFAULT_BLOCK_SIZE : size;
 
     block->memory = malloc(size);
     if (!block->memory) {
         free(block);
         DEBUG_ERROR("Failed to allocate memory for block of size %zu", size);
         return NULL;
     }
 
     block->used = 0;
     block->capacity = size;
     block->next = NULL;
 
     return block;
 }
 
 // Initialize memory management
 int memory_init(void) {
     // Zero out global contexts
     memset(g_memory_contexts, 0, sizeof(g_memory_contexts));
     DEBUG_VERBOSE("Memory management initialized");
     return 1;
 }
 
 // Shutdown and free all memory contexts
 void memory_shutdown(void) {
     DEBUG_VERBOSE("Shutting down memory management");
     for (int i = 0; i < MEMORY_CONTEXT_MAX; i++) {
         if (g_memory_contexts[i]) {
             destroy_memory_context(g_memory_contexts[i]);
             g_memory_contexts[i] = NULL;
         }
     }
 }
 
 // Create a new memory context
 MemoryContext* create_memory_context(MemoryContextConfig config) {
     MemoryContext *context = malloc(sizeof(MemoryContext));
     if (!context) {
         DEBUG_ERROR("Failed to create memory context");
         return NULL;
     }
 
     // Initialize context
     context->type = config.type;
     context->strategy = config.strategy;
     context->first_block = NULL;
     context->current_block = NULL;
     context->total_allocated = 0;
     context->total_used = 0;
 
     // Set memory limit
     context->max_memory_limit = config.max_total_memory > 0 
         ? config.max_total_memory 
         : DEFAULT_MAX_MEMORY;
 
     // Store in global contexts
     if (config.type < MEMORY_CONTEXT_MAX) {
         g_memory_contexts[config.type] = context;
     }
 
     DEBUG_VERBOSE("Created memory context of type %d with limit %zu bytes", 
                   config.type, context->max_memory_limit);
 
     return context;
 }
 
 // Destroy a memory context
 void destroy_memory_context(MemoryContext *context) {
     if (!context) return;
 
     DEBUG_VERBOSE("Destroying memory context (total allocated: %zu bytes)", 
                   context->total_allocated);
 
     // Free all memory blocks
     MemoryBlock *block = context->first_block;
     while (block) {
         MemoryBlock *next = block->next;
         free(block->memory);
         free(block);
         block = next;
     }
 
     free(context);
 }
 
 // Allocate memory within a context
 void* memory_alloc(MemoryContext *context, size_t size) {
     if (!context) {
         DEBUG_ERROR("Attempting to allocate in NULL context");
         return NULL;
     }
 
     // Align size
     size = MEMORY_ALIGN(size);
 
     // Check memory limit
     if (context->total_allocated + size > context->max_memory_limit) {
         DEBUG_ERROR("Memory limit exceeded (current: %zu, requested: %zu, limit: %zu)", 
                     context->total_allocated, size, context->max_memory_limit);
         return NULL;
     }
 
     // Arena allocation strategy
     if (context->strategy == ALLOC_STRATEGY_ARENA) {
         // If no blocks or current block doesn't have enough space
         if (!context->current_block || 
             context->current_block->used + size > context->current_block->capacity)
         {
             // Create a new block
             MemoryBlock *new_block = create_memory_block(size);
             if (!new_block) return NULL;
 
             // Link the block
             if (!context->first_block) {
                 context->first_block = new_block;
             } else if (context->current_block) {
                 context->current_block->next = new_block;
             }
             context->current_block = new_block;
             context->total_allocated += new_block->capacity;
 
             DEBUG_VERBOSE("Created new memory block of size %zu", new_block->capacity);
         }
 
         // Allocate from current block
         void *ptr = (char*)context->current_block->memory + context->current_block->used;
         context->current_block->used += size;
         context->total_used += size;
 
         return ptr;
     }
 
     // Standard allocation
     void *ptr = malloc(size);
     if (ptr) {
         context->total_allocated += size;
         context->total_used += size;
         DEBUG_VERBOSE("Allocated %zu bytes using standard allocation", size);
     } else {
         DEBUG_ERROR("Failed to allocate %zu bytes", size);
     }
     return ptr;
 }
 
 // Allocate and zero memory
 void* memory_calloc(MemoryContext *context, size_t count, size_t size) {
     size_t total_size = count * size;
     void *ptr = memory_alloc(context, total_size);
     
     if (ptr) {
         memset(ptr, 0, total_size);
         DEBUG_VERBOSE("Allocated and zeroed %zu bytes", total_size);
     }
 
     return ptr;
 }
 
 // Duplicate a string within a context
 char* memory_strdup(MemoryContext *context, const char *str) {
     if (!str) return NULL;
 
     size_t len = strlen(str) + 1;
     char *new_str = memory_alloc(context, len);
     
     if (new_str) {
         memcpy(new_str, str, len);
         DEBUG_VERBOSE("Duplicated string of length %zu", len);
     }
 
     return new_str;
 }
 
 // Reset a memory context
 void memory_reset_context(MemoryContext *context) {
     if (!context) return;
 
     DEBUG_VERBOSE("Resetting memory context (previous usage: %zu bytes)", 
                   context->total_used);
 
     // Reset all blocks to zero usage
     MemoryBlock *block = context->first_block;
     while (block) {
         block->used = 0;
         block = block->next;
     }
 
     // Reset tracking
     context->current_block = context->first_block;
     context->total_used = 0;
 }
 
 // Get total memory used by a context
 size_t memory_context_usage(MemoryContext *context) {
     return context ? context->total_used : 0;
 }
 
 // Print memory statistics
 void memory_print_stats(MemoryContext *context) {
     if (!context) return;
 
     DEBUG_INFO("Memory Context Stats:");
     DEBUG_INFO("  Total Allocated: %zu bytes", context->total_allocated);
     DEBUG_INFO("  Total Used: %zu bytes", context->total_used);
     DEBUG_INFO("  Memory Limit: %zu bytes", context->max_memory_limit);
 
     // Print block details
     size_t block_count = 0;
     size_t total_block_capacity = 0;
     MemoryBlock *block = context->first_block;
     while (block) {
         block_count++;
         total_block_capacity += block->capacity;
         block = block->next;
     }
 
     DEBUG_INFO("  Block Count: %zu", block_count);
     DEBUG_INFO("  Total Block Capacity: %zu bytes", total_block_capacity);
 }