/**
 * memory.h
 * Generic hierarchical memory management system
 */

 #ifndef MEMORY_H
 #define MEMORY_H
 
 #include <stddef.h>
 #include <stdint.h>
 #include <stdbool.h>
 
 /**
  * @brief Logging levels for memory subsystem messages
  */
 typedef enum {
     MEM_LOG_NONE = 0,
     MEM_LOG_ERROR,
     MEM_LOG_WARNING,
     MEM_LOG_INFO,
     MEM_LOG_VERBOSE
 } MemoryLogLevel;
 
 /**
  * @brief Memory allocation strategies
  */
 typedef enum {
     MEM_STRATEGY_STANDARD, // Traditional malloc/free behavior
     MEM_STRATEGY_ARENA,    // Fast arena allocation for objects with similar lifetimes
     MEM_STRATEGY_POOL      // Fixed-size object pool for uniform allocations
 } MemoryStrategy;
 
 /**
  * Forward declarations of opaque structures
  */
 typedef struct MemoryContext MemoryContext;
 typedef struct MemoryManager MemoryManager;
 
 /**
  * @brief Configuration for a memory context
  */
 typedef struct {
     const char* name;           // Human-readable name for debugging
     MemoryStrategy strategy;    // Allocation strategy to use
     size_t initial_size;        // Initial arena/pool size (0 = use default)
     size_t max_size;            // Maximum memory limit (0 = use default)
     size_t pool_object_size;    // Size of each object in pool (for pool strategy only)
     bool thread_safe;           // Whether to use thread safety mechanisms
 } MemoryContextConfig;
 
 /**
  * @brief Statistics for a memory context
  */
 typedef struct {
     const char* name;          // Context name
     MemoryStrategy strategy;   // Allocation strategy used
     size_t allocated;          // Total bytes allocated
     size_t used;               // Current bytes in use
     size_t peak;               // Peak memory usage
     size_t max_size;           // Maximum allowed size
     size_t block_count;        // Number of memory blocks
     size_t alloc_count;        // Number of allocations
     size_t free_count;         // Number of frees
 } MemoryStats;
 
 /**
  * @brief Optional callback used to handle memory allocation failures
  *
  * Return true if the allocation should be retried, false to propagate the failure
  */
 typedef bool (*MemoryFailureHandler)(MemoryContext* context, size_t requested_size, void* user_data);
 
 /**
  * Global memory management initialization and cleanup
  */
 
 /**
  * @brief Initialize the memory management system
  * 
  * @param log_level Initial logging level
  * @param detect_leaks Whether to enable leak detection
  * @return A new memory manager or NULL on failure
  */
 MemoryManager* memory_init(MemoryLogLevel log_level, bool detect_leaks);
 
 /**
  * @brief Shut down the memory management system and free all memory
  * 
  * @param manager The memory manager to shut down
  */
 void memory_shutdown(MemoryManager* manager);
 
 /**
  * @brief Set the logging level for memory operations
  * 
  * @param manager The memory manager
  * @param level The new logging level
  */
 void memory_set_log_level(MemoryManager* manager, MemoryLogLevel level);
 
 /**
  * @brief Set a callback to handle memory allocation failures
  * 
  * @param manager The memory manager
  * @param handler The failure handler callback
  * @param user_data User data to pass to the handler
  */
 void memory_set_failure_handler(MemoryManager* manager, MemoryFailureHandler handler, void* user_data);
 
 /**
  * Memory context operations
  */
 
 /**
  * @brief Create a root memory context
  * 
  * @param manager The memory manager
  * @param config Configuration for the context
  * @return A new memory context or NULL on failure
  */
 MemoryContext* memory_create_context(MemoryManager* manager, MemoryContextConfig config);
 
 /**
  * @brief Create a child memory context
  * 
  * @param parent The parent memory context
  * @param config Configuration for the child context
  * @return A new memory context or NULL on failure
  */
 MemoryContext* memory_create_child_context(MemoryContext* parent, MemoryContextConfig config);
 
 /**
  * @brief Destroy a memory context and all its children
  * 
  * @param context The context to destroy
  */
 void memory_destroy_context(MemoryContext* context);
 
 /**
  * @brief Reset a memory context (free all allocations but keep the context)
  * 
  * @param context The context to reset
  */
 void memory_reset_context(MemoryContext* context);
 
 /**
  * @brief Get statistics for a memory context
  * 
  * @param context The context to query
  * @param stats The statistics structure to fill
  */
 void memory_get_stats(MemoryContext* context, MemoryStats* stats);
 
 /**
  * @brief Print detailed statistics for a memory context and its children
  * 
  * @param context The context to print statistics for
  * @param verbose Whether to include detailed information
  */
 void memory_print_stats(MemoryContext* context, bool verbose);
 
 /**
  * @brief Get the parent of a memory context
  * 
  * @param context The context to query
  * @return The parent context or NULL if this is a root context
  */
 MemoryContext* memory_get_parent(MemoryContext* context);
 
 /**
  * @brief Get the memory manager that owns a context
  * 
  * @param context The context to query
  * @return The memory manager
  */
 MemoryManager* memory_get_manager(MemoryContext* context);
 
 /**
  * @brief Find a memory context by name
  * 
  * @param manager The memory manager
  * @param name The name to search for
  * @return The found context or NULL if not found
  */
 MemoryContext* memory_find_context(MemoryManager* manager, const char* name);
 
 /**
  * Memory allocation functions
  */
 
 /**
  * @brief Allocate memory from a context
  * 
  * @param context The memory context
  * @param size The number of bytes to allocate
  * @return A pointer to the allocated memory or NULL on failure
  */
 void* memory_alloc(MemoryContext* context, size_t size);
 
 /**
  * @brief Allocate memory and initialize it to zero
  * 
  * @param context The memory context
  * @param count The number of elements to allocate
  * @param size The size of each element
  * @return A pointer to the allocated memory or NULL on failure
  */
 void* memory_calloc(MemoryContext* context, size_t count, size_t size);
 
 /**
  * @brief Reallocate a memory block to a new size
  * 
  * @param context The memory context
  * @param ptr The existing memory block (or NULL to allocate a new block)
  * @param size The new size in bytes
  * @return A pointer to the reallocated memory or NULL on failure
  */
 void* memory_realloc(MemoryContext* context, void* ptr, size_t size);
 
 /**
  * @brief Duplicate a string using memory from the context
  * 
  * @param context The memory context
  * @param str The string to duplicate
  * @return A new copy of the string or NULL on failure
  */
 char* memory_strdup(MemoryContext* context, const char* str);
 
 /**
  * @brief Free memory allocated from a context
  * 
  * @param context The memory context
  * @param ptr The memory to free
  * @note Only needed for MEM_STRATEGY_STANDARD
  */
 void memory_free(MemoryContext* context, void* ptr);
 
 /**
  * Pool allocation functions (for MEM_STRATEGY_POOL)
  */
 
 /**
  * @brief Allocate an object from a memory pool
  * 
  * @param context The memory context (must use MEM_STRATEGY_POOL)
  * @return A pointer to a pool object or NULL on failure
  */
 void* memory_pool_alloc(MemoryContext* context);
 
 /**
  * @brief Return an object to a memory pool
  * 
  * @param context The memory context
  * @param ptr The object to return to the pool
  */
 void memory_pool_free(MemoryContext* context, void* ptr);
 
 /**
  * Memory leak detection
  */
 
 /**
  * @brief Enable memory leak detection
  * 
  * @param manager The memory manager
  */
 void memory_enable_leak_detection(MemoryManager* manager);
 
 /**
  * @brief Disable memory leak detection
  * 
  * @param manager The memory manager
  */
 void memory_disable_leak_detection(MemoryManager* manager);
 
 /**
  * @brief Print memory leak report
  * 
  * @param manager The memory manager
  */
 void memory_print_leaks(MemoryManager* manager);
 
 /**
  * Convenience macros for allocating structures
  */
 
 /**
  * @brief Allocate a structure of the specified type
  */
 #define MEM_NEW(ctx, type) ((type*)memory_calloc((ctx), 1, sizeof(type)))
 
 /**
  * @brief Allocate an array of structures
  */
 #define MEM_NEW_ARRAY(ctx, type, count) ((type*)memory_calloc((ctx), (count), sizeof(type)))
 
 #endif /* MEMORY_H */