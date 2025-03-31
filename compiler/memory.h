#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MEM_LOG_NONE = 0,
    MEM_LOG_ERROR,
    MEM_LOG_WARNING,
    MEM_LOG_INFO,
    MEM_LOG_VERBOSE
} MemoryLogLevel;

typedef enum {
    MEM_STRATEGY_STANDARD,
    MEM_STRATEGY_ARENA,
    MEM_STRATEGY_POOL
} MemoryStrategy;

typedef struct MemoryContext MemoryContext;
typedef struct MemoryManager MemoryManager;

typedef struct {
    const char* name;
    MemoryStrategy strategy;
    size_t initial_size;
    size_t max_size;
    size_t pool_object_size;
    bool thread_safe;
} MemoryContextConfig;

typedef struct {
    const char* name;
    MemoryStrategy strategy;
    size_t allocated;
    size_t used;
    size_t peak;
    size_t max_size;
    size_t block_count;
    size_t alloc_count;
    size_t free_count;
} MemoryStats;

typedef bool (*MemoryFailureHandler)(MemoryContext* context, size_t requested_size, void* user_data);

MemoryManager* memory_init(MemoryLogLevel log_level, bool detect_leaks);
void memory_shutdown(MemoryManager* manager);
void memory_set_log_level(MemoryManager* manager, MemoryLogLevel level);
void memory_set_failure_handler(MemoryManager* manager, MemoryFailureHandler handler, void* user_data);
MemoryContext* memory_create_context(MemoryManager* manager, MemoryContextConfig config);
MemoryContext* memory_create_child_context(MemoryContext* parent, MemoryContextConfig config);
void memory_destroy_context(MemoryContext* context);
void memory_reset_context(MemoryContext* context);
void memory_get_stats(MemoryContext* context, MemoryStats* stats);
void memory_print_stats(MemoryContext* context, bool verbose);
MemoryContext* memory_get_parent(MemoryContext* context);
MemoryManager* memory_get_manager(MemoryContext* context);
MemoryContext* memory_find_context(MemoryManager* manager, const char* name);
void* memory_alloc(MemoryContext* context, size_t size);
void* memory_calloc(MemoryContext* context, size_t count, size_t size);
void* memory_realloc(MemoryContext* context, void* ptr, size_t size);
char* memory_strdup(MemoryContext* context, const char* str);
void memory_free(MemoryContext* context, void* ptr);
void* memory_pool_alloc(MemoryContext* context);
void memory_pool_free(MemoryContext* context, void* ptr);
void memory_enable_leak_detection(MemoryManager* manager);
void memory_disable_leak_detection(MemoryManager* manager);
void memory_print_leaks(MemoryManager* manager);

#define MEM_NEW(ctx, type) ((type*)memory_calloc((ctx), 1, sizeof(type)))
#define MEM_NEW_ARRAY(ctx, type, count) ((type*)memory_calloc((ctx), (count), sizeof(type)))

#endif