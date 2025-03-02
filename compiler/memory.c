/**
 * memory.c
 * Enhanced implementation of generalized memory management with:
 * - Thread safety
 * - Hierarchical contexts
 * - Pool allocation
 * - Memory tracking
 * - Leak detection
 * - Improved debugging
 */

 #include "memory.h"
 #include "debug.h"
 #include <stdlib.h>
 #include <string.h>
 #include <stdio.h>
 #include <assert.h>
 
 // Default configuration constants
 #define DEFAULT_BLOCK_SIZE 4096
 #define DEFAULT_MAX_MEMORY (1024 * 1024 * 64)  // 64MB per context
 #define DEFAULT_POOL_SIZE 128                  // Elements per pool block
 #define DEFAULT_BITMAP_SIZE(elements) ((elements + 7) / 8)
 
 // Align memory to 8-byte boundary
 #define MEMORY_ALIGN(size) (((size) + 7) & ~0x7)
 
 // Bitmap operations for pool allocation
 #define SET_BIT(bitmap, index) ((bitmap)[(index) / 8] |= (1 << ((index) % 8)))
 #define CLEAR_BIT(bitmap, index) ((bitmap)[(index) / 8] &= ~(1 << ((index) % 8)))
 #define TEST_BIT(bitmap, index) (((bitmap)[(index) / 8] >> ((index) % 8)) & 1)
 
 // Global memory contexts
 static MemoryContext *g_memory_contexts[MEMORY_CONTEXT_MAX];
 static int g_leak_detection_enabled = 0;
 
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
     block->has_free_space = 1;
 
     return block;
 }
 
 // Create a new pool block for fixed-size allocations
 static PoolBlock* create_pool_block(size_t element_size, size_t elements) {
     PoolBlock *block = malloc(sizeof(PoolBlock));
     if (!block) {
         DEBUG_ERROR("Failed to allocate pool block descriptor");
         return NULL;
     }
 
     // Ensure element size is aligned
     element_size = MEMORY_ALIGN(element_size);
     
     // Allocate memory for the pool
     block->memory = malloc(element_size * elements);
     if (!block->memory) {
         free(block);
         DEBUG_ERROR("Failed to allocate memory for pool of size %zu", element_size * elements);
         return NULL;
     }
     
     // Allocate bitmap for tracking free elements
     size_t bitmap_size = DEFAULT_BITMAP_SIZE(elements);
     block->free_bitmap = calloc(bitmap_size, 1); // All bits set to 0 (free)
     if (!block->free_bitmap) {
         free(block->memory);
         free(block);
         DEBUG_ERROR("Failed to allocate bitmap for pool");
         return NULL;
     }
     
     block->element_size = element_size;
     block->capacity = elements;
     block->free_count = elements;
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
 
 // Enable leak detection
 int memory_add_leak_detection(void) {
     g_leak_detection_enabled = 1;
     DEBUG_INFO("Memory leak detection enabled");
     return 1;
 }
 
 // Print memory leaks
 void memory_print_leaks(void) {
 #ifdef DEBUG_MEMORY
     int leak_count = 0;
     size_t total_leaked = 0;
     
     DEBUG_INFO("=== MEMORY LEAK REPORT ===");
     
     for (int i = 0; i < MEMORY_CONTEXT_MAX; i++) {
         if (g_memory_contexts[i]) {
             MemoryContext *ctx = g_memory_contexts[i];
             if (ctx->total_used > 0) {
                 DEBUG_INFO("Context %d has %zu bytes still allocated", i, ctx->total_used);
                 
                 // Print detailed allocation information if available
                 AllocationInfo *info = ctx->allocations;
                 while (info) {
                     leak_count++;
                     total_leaked += info->size;
                     DEBUG_INFO("  Leak: %zu bytes at %p, allocated at %s:%d", 
                              info->size, info->ptr, info->file, info->line);
                     info = info->next;
                 }
             }
         }
     }
     
     if (leak_count > 0) {
         DEBUG_WARNING("MEMORY LEAK: %d allocations totaling %zu bytes", 
                     leak_count, total_leaked);
     } else {
         DEBUG_INFO("No memory leaks detected");
     }
     
     DEBUG_INFO("=== END LEAK REPORT ===");
 #else
     DEBUG_WARNING("Memory leak detection requires DEBUG_MEMORY to be defined");
 #endif
 }
 
 // Shutdown and free all memory contexts
 void memory_shutdown(void) {
     DEBUG_VERBOSE("Shutting down memory management");
     
     // Check for leaks if enabled
     if (g_leak_detection_enabled) {
         memory_print_leaks();
     }
     
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
     memset(context, 0, sizeof(MemoryContext));
     context->type = config.type;
     context->strategy = config.strategy;
     context->total_allocated = 0;
     context->total_used = 0;
     context->thread_safe = config.thread_safe;
     
     // Initialize parent/child pointers
     context->parent = NULL;
     context->first_child = NULL;
     context->next_sibling = NULL;
 
     // Set memory limit
     context->max_memory_limit = config.max_total_memory > 0 
         ? config.max_total_memory 
         : DEFAULT_MAX_MEMORY;
     
     // Initialize mutex if thread safety is requested
     if (context->thread_safe) {
         if (pthread_mutex_init(&context->mutex, NULL) != 0) {
             DEBUG_ERROR("Failed to initialize mutex for context");
             free(context);
             return NULL;
         }
     }
     
     // For pool allocation strategy, create initial pool
     if (context->strategy == ALLOC_STRATEGY_POOL) {
         size_t element_size = config.pool_element_size > 0 
             ? config.pool_element_size 
             : sizeof(void*);
             
         context->first_pool = create_pool_block(element_size, DEFAULT_POOL_SIZE);
         if (!context->first_pool) {
             if (context->thread_safe) {
                 pthread_mutex_destroy(&context->mutex);
             }
             free(context);
             return NULL;
         }
         
         context->total_allocated = element_size * DEFAULT_POOL_SIZE;
     }
 
     // Store in global contexts
     if (config.type < MEMORY_CONTEXT_MAX) {
         g_memory_contexts[config.type] = context;
     }
 
     DEBUG_VERBOSE("Created memory context of type %d with limit %zu bytes", 
                  config.type, context->max_memory_limit);
 
     return context;
 }
 
 // Create a child memory context
 MemoryContext* create_child_context(MemoryContext *parent, MemoryContextConfig config) {
     if (!parent) {
         DEBUG_ERROR("Parent context is NULL when creating child");
         return NULL;
     }
     
     MemoryContext *child = create_memory_context(config);
     if (!child) {
         return NULL;
     }
     
     // Set parent-child relationship
     child->parent = parent;
     
     // Add child to parent's child list
     if (parent->thread_safe) {
         pthread_mutex_lock(&parent->mutex);
     }
     
     if (!parent->first_child) {
         parent->first_child = child;
     } else {
         MemoryContext *sibling = parent->first_child;
         while (sibling->next_sibling) {
             sibling = sibling->next_sibling;
         }
         sibling->next_sibling = child;
     }
     
     if (parent->thread_safe) {
         pthread_mutex_unlock(&parent->mutex);
     }
     
     DEBUG_VERBOSE("Created child context of type %d with parent of type %d",
                  child->type, parent->type);
                  
     return child;
 }
 
 // Free all memory blocks in a memory context
 static void free_memory_blocks(MemoryBlock *block) {
     while (block) {
         MemoryBlock *next = block->next;
         free(block->memory);
         free(block);
         block = next;
     }
 }
 
 // Free all pool blocks in a memory context
 static void free_pool_blocks(PoolBlock *block) {
     while (block) {
         PoolBlock *next = block->next;
         free(block->memory);
         free(block->free_bitmap);
         free(block);
         block = next;
     }
 }
 
 // Destroy a memory context and all its resources
 void destroy_memory_context(MemoryContext *context) {
     if (!context) return;
 
     DEBUG_VERBOSE("Destroying memory context of type %d (total allocated: %zu bytes)", 
                  context->type, context->total_allocated);
                  
     // First destroy all child contexts
     destroy_child_contexts(context);
 
     // Free all memory blocks
     free_memory_blocks(context->first_block);
     
     // Free all pool blocks
     free_pool_blocks(context->first_pool);
     
     // Clean up thread safety resources
     if (context->thread_safe) {
         pthread_mutex_destroy(&context->mutex);
     }
     
     // Call cleanup function if defined
     if (context->cleanup) {
         context->cleanup(context);
     }
     
     // Remove from parent's child list if applicable
     if (context->parent) {
         MemoryContext *parent = context->parent;
         
         if (parent->thread_safe) {
             pthread_mutex_lock(&parent->mutex);
         }
         
         if (parent->first_child == context) {
             parent->first_child = context->next_sibling;
         } else {
             MemoryContext *sibling = parent->first_child;
             while (sibling && sibling->next_sibling != context) {
                 sibling = sibling->next_sibling;
             }
             if (sibling) {
                 sibling->next_sibling = context->next_sibling;
             }
         }
         
         if (parent->thread_safe) {
             pthread_mutex_unlock(&parent->mutex);
         }
     }
 
     // Free the context itself
     free(context);
 }
 
 // Destroy all child contexts
 void destroy_child_contexts(MemoryContext *parent) {
     if (!parent) return;
     
     if (parent->thread_safe) {
         pthread_mutex_lock(&parent->mutex);
     }
     
     MemoryContext *child = parent->first_child;
     // Set first_child to NULL before we start destroying to avoid recursion issues
     parent->first_child = NULL;
     
     if (parent->thread_safe) {
         pthread_mutex_unlock(&parent->mutex);
     }
     
     // Now destroy all children
     while (child) {
         MemoryContext *next = child->next_sibling;
         destroy_memory_context(child);
         child = next;
     }
 }
 
 // Lock a memory context
 void memory_context_lock(MemoryContext *context) {
     if (!context || !context->thread_safe) return;
     pthread_mutex_lock(&context->mutex);
 }
 
 // Unlock a memory context
 void memory_context_unlock(MemoryContext *context) {
     if (!context || !context->thread_safe) return;
     pthread_mutex_unlock(&context->mutex);
 }
 
 #ifdef DEBUG_MEMORY
 // Track an allocation for debugging
 static void track_allocation(MemoryContext *context, void *ptr, size_t size, 
                             const char *file, int line) {
     if (!context || !ptr) return;
     
     AllocationInfo *info = malloc(sizeof(AllocationInfo));
     if (!info) {
         DEBUG_ERROR("Failed to allocate memory for tracking");
         return;
     }
     
     info->ptr = ptr;
     info->size = size;
     info->file = file;
     info->line = line;
     
     // Add to the allocation list
     info->next = context->allocations;
     context->allocations = info;
 }
 
 // Untrack an allocation for debugging
 static void untrack_allocation(MemoryContext *context, void *ptr) {
     if (!context || !ptr) return;
     
     AllocationInfo **pp = &context->allocations;
     while (*pp) {
         if ((*pp)->ptr == ptr) {
             AllocationInfo *to_free = *pp;
             *pp = to_free->next;
             free(to_free);
             return;
         }
         pp = &(*pp)->next;
     }
     
     // If we get here, the pointer wasn't found
     DEBUG_WARNING("Attempted to free untracked pointer %p", ptr);
 }
 #endif
 
 // Find the best fitting block for a size
 static MemoryBlock* find_best_fit_block(MemoryContext *context, size_t size) {
     MemoryBlock *best = NULL;
     size_t best_fit = SIZE_MAX;
     MemoryBlock *block = context->first_block;
     
     while (block) {
         size_t available = block->capacity - block->used;
         if (available >= size && available < best_fit) {
             best = block;
             best_fit = available;
         }
         block = block->next;
     }
     
     return best;
 }
 
 // Allocate memory using best-fit strategy
 void* memory_alloc_best_fit(MemoryContext *context, size_t size) {
     if (!context) {
         DEBUG_ERROR("Attempting to allocate in NULL context");
         return NULL;
     }
     
     if (context->thread_safe) {
         pthread_mutex_lock(&context->mutex);
     }
     
     // Align size
     size = MEMORY_ALIGN(size);
     
     // Check memory limit
     if (context->total_allocated + size > context->max_memory_limit) {
         DEBUG_ERROR("Memory limit exceeded (current: %zu, requested: %zu, limit: %zu)", 
                   context->total_allocated, size, context->max_memory_limit);
         if (context->thread_safe) {
             pthread_mutex_unlock(&context->mutex);
         }
         return NULL;
     }
     
     // Find the best fitting block
     MemoryBlock *block = find_best_fit_block(context, size);
     if (!block) {
         // No block with enough space - create new one
         size_t new_block_size = size > DEFAULT_BLOCK_SIZE ? size : DEFAULT_BLOCK_SIZE;
         block = create_memory_block(new_block_size);
         if (!block) {
             if (context->thread_safe) {
                 pthread_mutex_unlock(&context->mutex);
             }
             return NULL;
         }
         
         // Add to block list
         block->next = context->first_block;
         context->first_block = block;
         context->current_block = block;
         context->total_allocated += block->capacity;
     }
     
     // Allocate from the block
     void *ptr = (char*)block->memory + block->used;
     block->used += size;
     context->total_used += size;
     
     // Update has_free_space flag
     if (block->used + MEMORY_ALIGN(sizeof(void*)) >= block->capacity) {
         block->has_free_space = 0;
     }
     
     if (context->thread_safe) {
         pthread_mutex_unlock(&context->mutex);
     }
     
     return ptr;
 }
 
 // Allocate memory within a context
 void* memory_alloc(MemoryContext *context, size_t size) {
     if (!context) {
         DEBUG_ERROR("Attempting to allocate in NULL context");
         return NULL;
     }
     
     if (context->thread_safe) {
         pthread_mutex_lock(&context->mutex);
     }
     
     void *ptr = NULL;
     
     // Align size
     size = MEMORY_ALIGN(size);
     
     // Check memory limit
     if (context->total_allocated + size > context->max_memory_limit) {
         DEBUG_ERROR("Memory limit exceeded (current: %zu, requested: %zu, limit: %zu)", 
                   context->total_allocated, size, context->max_memory_limit);
         if (context->thread_safe) {
             pthread_mutex_unlock(&context->mutex);
         }
         return NULL;
     }
 
     // Different allocation strategies
     switch (context->strategy) {
         case ALLOC_STRATEGY_ARENA:
             // If no blocks or current block doesn't have enough space
             if (!context->current_block || 
                 context->current_block->used + size > context->current_block->capacity)
             {
                 // Create a new block
                 size_t new_block_size = size > DEFAULT_BLOCK_SIZE ? size : DEFAULT_BLOCK_SIZE;
                 MemoryBlock *new_block = create_memory_block(new_block_size);
                 if (!new_block) {
                     if (context->thread_safe) {
                         pthread_mutex_unlock(&context->mutex);
                     }
                     return NULL;
                 }
 
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
             ptr = (char*)context->current_block->memory + context->current_block->used;
             context->current_block->used += size;
             context->total_used += size;
             break;
             
         case ALLOC_STRATEGY_POOL:
             DEBUG_ERROR("Direct memory_alloc not supported for pool allocation strategy");
             DEBUG_ERROR("Use memory_pool_alloc instead");
             if (context->thread_safe) {
                 pthread_mutex_unlock(&context->mutex);
             }
             return NULL;
             
         case ALLOC_STRATEGY_STANDARD:
             // Standard allocation
             ptr = malloc(size);
             if (ptr) {
                 context->total_allocated += size;
                 context->total_used += size;
                 DEBUG_VERBOSE("Allocated %zu bytes using standard allocation", size);
             } else {
                 DEBUG_ERROR("Failed to allocate %zu bytes", size);
             }
             break;
     }
     
     if (context->thread_safe) {
         pthread_mutex_unlock(&context->mutex);
     }
     
     return ptr;
 }
 
 #ifdef DEBUG_MEMORY
 // Allocate with debug tracking
 void* memory_alloc_debug(MemoryContext *context, size_t size, const char *file, int line) {
     void *ptr = memory_alloc(context, size);
     if (ptr) {
         if (context->thread_safe) {
             pthread_mutex_lock(&context->mutex);
         }
         track_allocation(context, ptr, size, file, line);
         if (context->thread_safe) {
             pthread_mutex_unlock(&context->mutex);
         }
     }
     return ptr;
 }
 #endif
 
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
 
 #ifdef DEBUG_MEMORY
 // Allocate and zero with debug tracking
 void* memory_calloc_debug(MemoryContext *context, size_t count, size_t size, const char *file, int line) {
     void *ptr = memory_calloc(context, count, size);
     if (ptr) {
         if (context->thread_safe) {
             pthread_mutex_lock(&context->mutex);
         }
         track_allocation(context, ptr, count * size, file, line);
         if (context->thread_safe) {
             pthread_mutex_unlock(&context->mutex);
         }
     }
     return ptr;
 }
 #endif
 
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
 
 #ifdef DEBUG_MEMORY
 // Duplicate string with debug tracking
 char* memory_strdup_debug(MemoryContext *context, const char *str, const char *file, int line) {
     char *new_str = memory_strdup(context, str);
     if (new_str) {
         if (context->thread_safe) {
             pthread_mutex_lock(&context->mutex);
         }
         track_allocation(context, new_str, strlen(str) + 1, file, line);
         if (context->thread_safe) {
             pthread_mutex_unlock(&context->mutex);
         }
     }
     return new_str;
 }
 #endif
 
 // Reallocate memory (only works with ALLOC_STRATEGY_STANDARD)
 void* memory_realloc(MemoryContext *context, void *ptr, size_t new_size) {
     if (!context) {
         DEBUG_ERROR("NULL context in memory_realloc");
         return NULL;
     }
     
     if (context->strategy != ALLOC_STRATEGY_STANDARD) {
         DEBUG_ERROR("memory_realloc only supported for standard allocation strategy");
         return NULL;
     }
     
     // Check if this would exceed memory limits
     if (context->total_allocated + new_size > context->max_memory_limit) {
         DEBUG_ERROR("Memory limit exceeded in realloc");
         return NULL;
     }
     
     if (context->thread_safe) {
         pthread_mutex_lock(&context->mutex);
     }
     
     void *new_ptr = realloc(ptr, new_size);
     if (new_ptr) {
 #ifdef DEBUG_MEMORY
         // Find the original allocation size
         AllocationInfo *info = context->allocations;
         size_t old_size = 0;
         while (info) {
             if (info->ptr == ptr) {
                 old_size = info->size;
                 // Update tracking
                 info->ptr = new_ptr;
                 info->size = new_size;
                 break;
             }
             info = info->next;
         }
 #else
         // Without DEBUG_MEMORY, we can't know the original size
         // This will lead to slight inaccuracy in tracking
         size_t old_size = 0;
 #endif
 
         // Update context tracking
         context->total_allocated = context->total_allocated - old_size + new_size;
         context->total_used = context->total_used - old_size + new_size;
     }
     
     if (context->thread_safe) {
         pthread_mutex_unlock(&context->mutex);
     }
     
     return new_ptr;
 }
 
 #ifdef DEBUG_MEMORY
 // Reallocate with debug tracking
 void* memory_realloc_debug(MemoryContext *context, void *ptr, size_t new_size, const char *file, int line) {
     void *new_ptr = memory_realloc(context, ptr, new_size);
     
     if (new_ptr && new_ptr != ptr) {
         if (context->thread_safe) {
             pthread_mutex_lock(&context->mutex);
         }
         
         // Remove old tracking and add new
         untrack_allocation(context, ptr);
         track_allocation(context, new_ptr, new_size, file, line);
         
         if (context->thread_safe) {
             pthread_mutex_unlock(&context->mutex);
         }
     }
     
     return new_ptr;
 }
 #endif
 
 // Allocate from a pool
 void* memory_pool_alloc(MemoryContext *context) {
     if (!context) {
         DEBUG_ERROR("NULL context in memory_pool_alloc");
         return NULL;
     }
     
     if (context->strategy != ALLOC_STRATEGY_POOL) {
         DEBUG_ERROR("memory_pool_alloc only for pool allocation strategy");
         return NULL;
     }
     
     if (context->thread_safe) {
         pthread_mutex_lock(&context->mutex);
     }
     
     void *ptr = NULL;
     PoolBlock *block = context->first_pool;
     
     // Find a block with free elements
     while (block) {
         if (block->free_count > 0) {
             // Find a free element
             for (size_t i = 0; i < block->capacity; i++) {
                 if (!TEST_BIT(block->free_bitmap, i)) {
                     // Found a free element
                     SET_BIT(block->free_bitmap, i);
                     block->free_count--;
                     
                     // Calculate the pointer
                     ptr = (char*)block->memory + (i * block->element_size);
                     context->total_used += block->element_size;
                     
                     if (context->thread_safe) {
                         pthread_mutex_unlock(&context->mutex);
                     }
                     return ptr;
                 }
             }
         }
         block = block->next;
     }
     
     // If we get here, we need a new pool block
     if (!context->first_pool) {
         DEBUG_ERROR("No pool blocks in memory_pool_alloc");
         if (context->thread_safe) {
             pthread_mutex_unlock(&context->mutex);
         }
         return NULL;
     }
     
     // Create a new pool with the same element size
     size_t element_size = context->first_pool->element_size;
     PoolBlock *new_block = create_pool_block(element_size, DEFAULT_POOL_SIZE);
     if (!new_block) {
         if (context->thread_safe) {
             pthread_mutex_unlock(&context->mutex);
         }
         return NULL;
     }
     
     // Add to the pool list
     new_block->next = context->first_pool;
     context->first_pool = new_block;
     context->total_allocated += element_size * DEFAULT_POOL_SIZE;
     
     // Allocate from the new block
     SET_BIT(new_block->free_bitmap, 0);
     new_block->free_count--;
     ptr = new_block->memory;
     context->total_used += element_size;
     
     if (context->thread_safe) {
         pthread_mutex_unlock(&context->mutex);
     }
     
     return ptr;
 }
 
 #ifdef DEBUG_MEMORY
 // Pool allocate with debug tracking
 void* memory_pool_alloc_debug(MemoryContext *context, const char *file, int line) {
     void *ptr = memory_pool_alloc(context);
     
     if (ptr) {
         if (context->thread_safe) {
             pthread_mutex_lock(&context->mutex);
         }
         
         // Track the allocation
         track_allocation(context, ptr, context->first_pool->element_size, file, line);
         
         if (context->thread_safe) {
             pthread_mutex_unlock(&context->mutex);
         }
     }
     
     return ptr;
 }
 #endif
 
 // Free memory from a pool
 void memory_pool_free(MemoryContext *context, void *ptr) {
     if (!context || !ptr) return;
     
     if (context->strategy != ALLOC_STRATEGY_POOL) {
         DEBUG_ERROR("memory_pool_free only for pool allocation strategy");
         return;
     }
     
     if (context->thread_safe) {
         pthread_mutex_lock(&context->mutex);
     }
     
     int found = 0;
     PoolBlock *block = context->first_pool;
     
     // Find which block the pointer belongs to
     while (block && !found) {
         if (ptr >= block->memory && 
             ptr < (char*)block->memory + (block->capacity * block->element_size)) {
             
             // Calculate the element index
             size_t offset = (char*)ptr - (char*)block->memory;
             size_t index = offset / block->element_size;
             
             // Check if this element is allocated
             if (TEST_BIT(block->free_bitmap, index)) {
                 // Clear the bit and update free count
                 CLEAR_BIT(block->free_bitmap, index);
                 block->free_count++;
                 context->total_used -= block->element_size;
                 found = 1;
             } else {
                 DEBUG_WARNING("Double free detected in memory_pool_free");
             }
             break;
         }
         block = block->next;
     }
     
     if (!found) {
         DEBUG_WARNING("Tried to free pointer not from this pool");
     }
     
 #ifdef DEBUG_MEMORY
     // Remove from tracking
     untrack_allocation(context, ptr);
 #endif
     
     if (context->thread_safe) {
         pthread_mutex_unlock(&context->mutex);
     }
 }
 
 // Free memory (only for standard allocation strategy)
 void memory_free(MemoryContext *context, void *ptr) {
     if (!context || !ptr) return;
     
     if (context->strategy != ALLOC_STRATEGY_STANDARD) {
         DEBUG_ERROR("memory_free only for standard allocation strategy");
         return;
     }
     
     if (context->thread_safe) {
         pthread_mutex_lock(&context->mutex);
     }
     
 #ifdef DEBUG_MEMORY
     // Find the allocation size
     AllocationInfo *info = context->allocations;
     size_t size = 0;
     while (info) {
         if (info->ptr == ptr) {
             size = info->size;
             break;
         }
         info = info->next;
     }
     
     if (size > 0) {
         context->total_allocated -= size;
         context->total_used -= size;
         untrack_allocation(context, ptr);
     } else {
         DEBUG_WARNING("Freeing untracked pointer in memory_free");
     }
 #else
     // Without DEBUG_MEMORY, we don't know size, so total_allocated becomes inaccurate
     // We estimate the decrease in total_used based on average allocation size
     if (context->total_allocated > 0 && context->total_used > 0) {
         size_t avg_size = context->total_allocated / (context->total_allocated / sizeof(void*));
         context->total_used -= avg_size;
     }
 #endif
     
     free(ptr);
     
     if (context->thread_safe) {
         pthread_mutex_unlock(&context->mutex);
     }
 }
 
 // Reset a memory context
 void memory_reset_context(MemoryContext *context) {
     if (!context) return;
 
     DEBUG_VERBOSE("Resetting memory context (previous usage: %zu bytes)", 
                  context->total_used);
                  
     if (context->thread_safe) {
         pthread_mutex_lock(&context->mutex);
     }
 
     // Reset based on strategy
     switch (context->strategy) {
         case ALLOC_STRATEGY_ARENA:
             // Reset all blocks to zero usage
             MemoryBlock *block = context->first_block;
             while (block) {
                 block->used = 0;
                 block->has_free_space = 1;
                 block = block->next;
             }
             
             // Reset tracking
             context->current_block = context->first_block;
             context->total_used = 0;
             break;
             
         case ALLOC_STRATEGY_POOL:
             // Reset all pool blocks
             PoolBlock *pool = context->first_pool;
             while (pool) {
                 // Zero out the bitmap (all elements free)
                 memset(pool->free_bitmap, 0, DEFAULT_BITMAP_SIZE(pool->capacity));
                 pool->free_count = pool->capacity;
                 pool = pool->next;
             }
             
             context->total_used = 0;
             break;
             
         case ALLOC_STRATEGY_STANDARD:
             // Can't really reset standard allocations
             // Just note that this isn't fully supported
             DEBUG_WARNING("Reset not fully supported for standard allocation strategy");
             break;
     }
     
 #ifdef DEBUG_MEMORY
     // Free all allocation tracking
     AllocationInfo *info = context->allocations;
     while (info) {
         AllocationInfo *next = info->next;
         free(info);
         info = next;
     }
     context->allocations = NULL;
 #endif
     
     if (context->thread_safe) {
         pthread_mutex_unlock(&context->mutex);
     }
 }
 
 // Compact memory by merging blocks
 void memory_compact(MemoryContext *context) {
     if (!context) return;
     
     // Only meaningful for arena allocation
     if (context->strategy != ALLOC_STRATEGY_ARENA) {
         DEBUG_WARNING("Compaction only supported for arena allocation strategy");
         return;
     }
     
     if (context->thread_safe) {
         pthread_mutex_lock(&context->mutex);
     }
     
     // Simple compaction: remove empty blocks except the first one
     if (context->first_block) {
         MemoryBlock *prev = context->first_block;
         MemoryBlock *curr = prev->next;
         
         while (curr) {
             MemoryBlock *next = curr->next;
             
             if (curr->used == 0) {
                 // Remove this empty block
                 prev->next = next;
                 context->total_allocated -= curr->capacity;
                 free(curr->memory);
                 free(curr);
             } else {
                 prev = curr;
             }
             
             curr = next;
         }
         
         // Set current block to the first one
         context->current_block = context->first_block;
     }
     
     DEBUG_VERBOSE("Compacted memory context to %zu bytes allocated", 
                  context->total_allocated);
                  
     if (context->thread_safe) {
         pthread_mutex_unlock(&context->mutex);
     }
 }
 
 // Get total memory used by a context
 size_t memory_context_usage(MemoryContext *context) {
     return context ? context->total_used : 0;
 }
 
 // Get total memory capacity of a context
 size_t memory_context_capacity(MemoryContext *context) {
     return context ? context->total_allocated : 0;
 }
 
 // Print memory statistics
 void memory_print_stats(MemoryContext *context) {
     if (!context) return;
 
     DEBUG_INFO("Memory Context Stats (Type %d):", context->type);
     DEBUG_INFO("  Total Allocated: %zu bytes", context->total_allocated);
     DEBUG_INFO("  Total Used: %zu bytes", context->total_used);
     DEBUG_INFO("  Memory Limit: %zu bytes", context->max_memory_limit);
     
     // Print allocation strategy
     const char *strategy = "Unknown";
     switch (context->strategy) {
         case ALLOC_STRATEGY_ARENA:
             strategy = "Arena";
             break;
         case ALLOC_STRATEGY_STANDARD:
             strategy = "Standard";
             break;
         case ALLOC_STRATEGY_POOL:
             strategy = "Pool";
             break;
     }
     DEBUG_INFO("  Allocation Strategy: %s", strategy);
 
     // Print block details based on strategy
     switch (context->strategy) {
         case ALLOC_STRATEGY_ARENA:
         {
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
             break;
         }
         
         case ALLOC_STRATEGY_POOL:
         {
             size_t pool_count = 0;
             size_t element_count = 0;
             size_t free_count = 0;
             PoolBlock *pool = context->first_pool;
             
             while (pool) {
                 pool_count++;
                 element_count += pool->capacity;
                 free_count += pool->free_count;
                 pool = pool->next;
             }
             
             DEBUG_INFO("  Pool Count: %zu", pool_count);
             DEBUG_INFO("  Element Size: %zu bytes", 
                      context->first_pool ? context->first_pool->element_size : 0);
             DEBUG_INFO("  Total Elements: %zu", element_count);
             DEBUG_INFO("  Free Elements: %zu", free_count);
             break;
         }
         
         default:
             // No extra stats for standard allocation
             break;
     }
     
     // Print hierarchy info
     if (context->parent) {
         DEBUG_INFO("  Parent Context: Type %d", context->parent->type);
     }
     
     size_t child_count = 0;
     MemoryContext *child = context->first_child;
     while (child) {
         child_count++;
         child = child->next_sibling;
     }
     DEBUG_INFO("  Child Contexts: %zu", child_count);
 }
 
 // Print detailed memory statistics
 void memory_print_detailed_stats(MemoryContext *context, int verbose) {
     if (!context) return;
     
     memory_print_stats(context);
     
     if (verbose) {
         // Print all blocks
         if (context->strategy == ALLOC_STRATEGY_ARENA) {
             int block_index = 0;
             MemoryBlock *block = context->first_block;
             while (block) {
                 DEBUG_INFO("  Block %d: %zu/%zu bytes used (%.1f%%)", 
                          block_index++, block->used, block->capacity,
                          (double)block->used / block->capacity * 100);
                 block = block->next;
             }
         }
         
         // Print pool details
         if (context->strategy == ALLOC_STRATEGY_POOL) {
             int pool_index = 0;
             PoolBlock *pool = context->first_pool;
             while (pool) {
                 DEBUG_INFO("  Pool %d: %zu/%zu elements free (%.1f%%)", 
                          pool_index++, pool->free_count, pool->capacity,
                          (double)pool->free_count / pool->capacity * 100);
                 pool = pool->next;
             }
         }
         
 #ifdef DEBUG_MEMORY
         // Print top allocations
         if (context->allocations) {
             DEBUG_INFO("  Top allocations:");
             
             // Count allocations
             size_t count = 0;
             AllocationInfo *info = context->allocations;
             while (info) {
                 count++;
                 info = info->next;
             }
             
             // Only show up to 10 allocations
             count = count > 10 ? 10 : count;
             
             info = context->allocations;
             for (size_t i = 0; i < count; i++) {
                 DEBUG_INFO("    %zu bytes at %p, allocated at %s:%d", 
                          info->size, info->ptr, info->file, info->line);
                 info = info->next;
             }
         }
 #endif
         
         // Print child contexts
         MemoryContext *child = context->first_child;
         if (child) {
             DEBUG_INFO("  Child contexts:");
             while (child) {
                 DEBUG_INFO("    Type %d: %zu/%zu bytes used", 
                          child->type, child->total_used, child->total_allocated);
                 child = child->next_sibling;
             }
         }
     }
 }