#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <pthread.h>

#define DEFAULT_ARENA_SIZE (16 * 1024)
#define DEFAULT_POOL_CAPACITY 128
#define DEFAULT_MAX_MEMORY (64 * 1024 * 1024)
#define ALIGNMENT 8
#define ALIGN_SIZE(s) (((s) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))
#define MAX_CONTEXT_NAME 64
#define DEFAULT_CONTEXT_CAPACITY 16

typedef struct MemoryBlock {
    void* memory;
    size_t size;
    size_t used;
    struct MemoryBlock* next;
    bool has_free_space;
} MemoryBlock;

typedef struct PoolBlock {
    void* memory;
    size_t object_size;
    size_t capacity;
    uint8_t* free_bitmap;
    size_t free_count;
    struct PoolBlock* next;
} PoolBlock;

#define BITMAP_SIZE(count) (((count) + 7) / 8)
#define SET_BIT(bitmap, idx) ((bitmap)[(idx) / 8] |= (1 << ((idx) % 8)))
#define CLEAR_BIT(bitmap, idx) ((bitmap)[(idx) / 8] &= ~(1 << ((idx) % 8)))
#define TEST_BIT(bitmap, idx) (((bitmap)[(idx) / 8] >> ((idx) % 8)) & 1)

typedef struct AllocationInfo {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    const char* func;
    struct AllocationInfo* next;
} AllocationInfo;

struct MemoryContext {
    char name[MAX_CONTEXT_NAME];
    MemoryStrategy strategy;
    MemoryManager* manager;
    MemoryContext* parent;
    MemoryContext* first_child;
    MemoryContext* next_sibling;
    MemoryBlock* first_block;
    MemoryBlock* current_block;
    PoolBlock* first_pool;
    size_t pool_object_size;
    size_t total_allocated;
    size_t total_used;
    size_t peak_usage;
    size_t max_size;
    size_t alloc_count;
    size_t free_count;
    pthread_mutex_t mutex;
    bool thread_safe;
    AllocationInfo* allocations;
    void* user_data;
};

struct MemoryManager {
    MemoryLogLevel log_level;
    bool leak_detection;
    MemoryContext** contexts;
    size_t context_count;
    size_t context_capacity;
    pthread_mutex_t mutex;
    bool initialized;
    MemoryFailureHandler failure_handler;
    void* failure_handler_data;
};

static MemoryManager* global_memory_manager = NULL;

static void memory_log(MemoryManager* manager, MemoryLogLevel level, const char* file, int line, 
                       const char* func, const char* format, ...) {
    if (!manager || level > manager->log_level) {
        return;
    }
    const char* level_str = "UNKNOWN";
    switch (level) {
        case MEM_LOG_ERROR:   level_str = "ERROR"; break;
        case MEM_LOG_WARNING: level_str = "WARNING"; break;
        case MEM_LOG_INFO:    level_str = "INFO"; break;
        case MEM_LOG_VERBOSE: level_str = "VERBOSE"; break;
        default: break;
    }
    va_list args;
    va_start(args, format);
    fprintf(stderr, "[MEMORY-%s] %s:%d:%s: ", level_str, file, line, func);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

#define MEM_ERROR(manager, ...) memory_log(manager, MEM_LOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEM_WARNING(manager, ...) memory_log(manager, MEM_LOG_WARNING, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEM_INFO(manager, ...) memory_log(manager, MEM_LOG_INFO, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define MEM_VERBOSE(manager, ...) memory_log(manager, MEM_LOG_VERBOSE, __FILE__, __LINE__, __func__, __VA_ARGS__)

static MemoryBlock* create_memory_block(size_t size) {
    size_t actual_size = size > 0 ? size : DEFAULT_ARENA_SIZE;
    MemoryBlock* block = malloc(sizeof(MemoryBlock));
    if (!block) {
        return NULL;
    }
    block->memory = malloc(actual_size);
    if (!block->memory) {
        free(block);
        return NULL;
    }
    block->size = actual_size;
    block->used = 0;
    block->next = NULL;
    block->has_free_space = true;
    return block;
}

static PoolBlock* create_pool_block(size_t object_size, size_t capacity) {
    size_t actual_capacity = capacity > 0 ? capacity : DEFAULT_POOL_CAPACITY;
    size_t aligned_size = ALIGN_SIZE(object_size);
    PoolBlock* block = malloc(sizeof(PoolBlock));
    if (!block) {
        return NULL;
    }
    size_t memory_size = aligned_size * actual_capacity;
    block->memory = malloc(memory_size);
    if (!block->memory) {
        free(block);
        return NULL;
    }
    size_t bitmap_size = BITMAP_SIZE(actual_capacity);
    block->free_bitmap = calloc(bitmap_size, 1);
    if (!block->free_bitmap) {
        free(block->memory);
        free(block);
        return NULL;
    }
    block->object_size = aligned_size;
    block->capacity = actual_capacity;
    block->free_count = actual_capacity;
    block->next = NULL;
    return block;
}

static void free_memory_block(MemoryBlock* block) {
    if (!block) return;
    free(block->memory);
    free(block);
}

static void free_pool_block(PoolBlock* block) {
    if (!block) return;
    free(block->memory);
    free(block->free_bitmap);
    free(block);
}

static void track_allocation(MemoryContext* context, void* ptr, size_t size, 
                             const char* file, int line, const char* func) {
    if (!context || !ptr || !context->manager->leak_detection) {
        return;
    }
    AllocationInfo* info = malloc(sizeof(AllocationInfo));
    if (!info) {
        return;
    }
    info->ptr = ptr;
    info->size = size;
    info->file = file;
    info->line = line;
    info->func = func;
    if (context->thread_safe) {
        pthread_mutex_lock(&context->mutex);
    }
    info->next = context->allocations;
    context->allocations = info;
    if (context->thread_safe) {
        pthread_mutex_unlock(&context->mutex);
    }
}

static void untrack_allocation(MemoryContext* context, void* ptr) {
    if (!context || !ptr || !context->manager->leak_detection) {
        return;
    }
    if (context->thread_safe) {
        pthread_mutex_lock(&context->mutex);
    }
    AllocationInfo** pp = &context->allocations;
    while (*pp) {
        if ((*pp)->ptr == ptr) {
            AllocationInfo* to_free = *pp;
            *pp = to_free->next;
            free(to_free);
            break;
        }
        pp = &(*pp)->next;
    }
    if (context->thread_safe) {
        pthread_mutex_unlock(&context->mutex);
    }
}

static void register_context(MemoryManager* manager, MemoryContext* context) {
    if (!manager || !context) {
        return;
    }
    pthread_mutex_lock(&manager->mutex);
    if (manager->context_count >= manager->context_capacity) {
        size_t new_capacity = manager->context_capacity * 2;
        MemoryContext** new_contexts = realloc(manager->contexts, 
                                             new_capacity * sizeof(MemoryContext*));
        if (!new_contexts) {
            pthread_mutex_unlock(&manager->mutex);
            return;
        }
        manager->contexts = new_contexts;
        manager->context_capacity = new_capacity;
    }
    manager->contexts[manager->context_count++] = context;
    pthread_mutex_unlock(&manager->mutex);
}

static void unregister_context(MemoryManager* manager, MemoryContext* context) {
    if (!manager || !context) {
        return;
    }
    pthread_mutex_lock(&manager->mutex);
    for (size_t i = 0; i < manager->context_count; i++) {
        if (manager->contexts[i] == context) {
            if (i < manager->context_count - 1) {
                manager->contexts[i] = manager->contexts[manager->context_count - 1];
            }
            manager->context_count--;
            break;
        }
    }
    pthread_mutex_unlock(&manager->mutex);
}

MemoryManager* memory_init(MemoryLogLevel log_level, bool detect_leaks) {
    if (global_memory_manager) {
        return global_memory_manager;
    }
    MemoryManager* manager = malloc(sizeof(MemoryManager));
    if (!manager) {
        return NULL;
    }
    memset(manager, 0, sizeof(MemoryManager));
    manager->log_level = log_level;
    manager->leak_detection = detect_leaks;
    manager->context_capacity = DEFAULT_CONTEXT_CAPACITY;
    manager->contexts = malloc(manager->context_capacity * sizeof(MemoryContext*));
    if (!manager->contexts) {
        free(manager);
        return NULL;
    }
    if (pthread_mutex_init(&manager->mutex, NULL) != 0) {
        free(manager->contexts);
        free(manager);
        return NULL;
    }
    manager->initialized = true;
    global_memory_manager = manager;
    return manager;
}

void memory_shutdown(MemoryManager* manager) {
    if (!manager) {
        return;
    }
    if (manager->leak_detection) {
        memory_print_leaks(manager);
    }
    while (manager->context_count > 0) {
        memory_destroy_context(manager->contexts[manager->context_count - 1]);
    }
    free(manager->contexts);
    pthread_mutex_destroy(&manager->mutex);
    if (global_memory_manager == manager) {
        global_memory_manager = NULL;
    }
    free(manager);
}

void memory_set_log_level(MemoryManager* manager, MemoryLogLevel level) {
    if (manager) {
        manager->log_level = level;
    }
}

void memory_set_failure_handler(MemoryManager* manager, MemoryFailureHandler handler, void* user_data) {
    if (manager) {
        manager->failure_handler = handler;
        manager->failure_handler_data = user_data;
    }
}

MemoryContext* memory_create_context(MemoryManager* manager, MemoryContextConfig config) {
    if (!manager) {
        return NULL;
    }
    MemoryContext* context = malloc(sizeof(MemoryContext));
    if (!context) {
        MEM_ERROR(manager, "Failed to allocate memory context");
        return NULL;
    }
    memset(context, 0, sizeof(MemoryContext));
    strncpy(context->name, config.name ? config.name : "unnamed", MAX_CONTEXT_NAME - 1);
    context->name[MAX_CONTEXT_NAME - 1] = '\0';
    context->strategy = config.strategy;
    context->max_size = config.max_size > 0 ? config.max_size : DEFAULT_MAX_MEMORY;
    context->manager = manager;
    context->thread_safe = config.thread_safe;
    if (context->thread_safe) {
        if (pthread_mutex_init(&context->mutex, NULL) != 0) {
            MEM_ERROR(manager, "Failed to initialize mutex for context '%s'", context->name);
            free(context);
            return NULL;
        }
    }
    switch (context->strategy) {
        case MEM_STRATEGY_ARENA:
            context->first_block = create_memory_block(config.initial_size);
            if (!context->first_block) {
                MEM_ERROR(manager, "Failed to create initial memory block for context '%s'", context->name);
                if (context->thread_safe) {
                    pthread_mutex_destroy(&context->mutex);
                }
                free(context);
                return NULL;
            }
            context->current_block = context->first_block;
            context->total_allocated = context->first_block->size;
            break;
        case MEM_STRATEGY_POOL:
            if (config.pool_object_size == 0) {
                MEM_ERROR(manager, "Pool object size must be greater than 0 for context '%s'", context->name);
                if (context->thread_safe) {
                    pthread_mutex_destroy(&context->mutex);
                }
                free(context);
                return NULL;
            }
            context->pool_object_size = config.pool_object_size;
            context->first_pool = create_pool_block(config.pool_object_size, 
                                                   config.initial_size / config.pool_object_size);
            if (!context->first_pool) {
                MEM_ERROR(manager, "Failed to create initial pool block for context '%s'", context->name);
                if (context->thread_safe) {
                    pthread_mutex_destroy(&context->mutex);
                }
                free(context);
                return NULL;
            }
            context->total_allocated = context->first_pool->object_size * context->first_pool->capacity;
            break;
        case MEM_STRATEGY_STANDARD:
            break;
    }
    register_context(manager, context);
    MEM_VERBOSE(manager, "Created memory context '%s' with strategy %d", context->name, context->strategy);
    return context;
}

MemoryContext* memory_create_child_context(MemoryContext* parent, MemoryContextConfig config) {
    if (!parent) {
        return NULL;
    }
    MemoryContext* child = memory_create_context(parent->manager, config);
    if (!child) {
        return NULL;
    }
    child->parent = parent;
    if (parent->thread_safe) {
        pthread_mutex_lock(&parent->mutex);
    }
    if (!parent->first_child) {
        parent->first_child = child;
    } else {
        MemoryContext* sibling = parent->first_child;
        while (sibling->next_sibling) {
            sibling = sibling->next_sibling;
        }
        sibling->next_sibling = child;
    }
    if (parent->thread_safe) {
        pthread_mutex_unlock(&parent->mutex);
    }
    MEM_VERBOSE(parent->manager, "Created child context '%s' with parent '%s'", 
              child->name, parent->name);
    return child;
}

void memory_destroy_context(MemoryContext* context) {
    if (!context) {
        return;
    }
    MemoryManager* manager = context->manager;
    MEM_VERBOSE(manager, "Destroying memory context '%s' (allocated: %zu, used: %zu)", 
              context->name, context->total_allocated, context->total_used);
    while (context->first_child) {
        MemoryContext* child = context->first_child;
        context->first_child = child->next_sibling;
        child->next_sibling = NULL;
        child->parent = NULL;
        memory_destroy_context(child);
    }
    MemoryBlock* block = context->first_block;
    while (block) {
        MemoryBlock* next = block->next;
        free_memory_block(block);
        block = next;
    }
    PoolBlock* pool = context->first_pool;
    while (pool) {
        PoolBlock* next = pool->next;
        free_pool_block(pool);
        pool = next;
    }
    AllocationInfo* info = context->allocations;
    while (info) {
        AllocationInfo* next = info->next;
        free(info);
        info = next;
    }
    if (context->parent) {
        MemoryContext* parent = context->parent;
        if (parent->thread_safe) {
            pthread_mutex_lock(&parent->mutex);
        }
        if (parent->first_child == context) {
            parent->first_child = context->next_sibling;
        } else {
            MemoryContext* sibling = parent->first_child;
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
    if (context->thread_safe) {
        pthread_mutex_destroy(&context->mutex);
    }
    unregister_context(manager, context);
    free(context);
}

void memory_reset_context(MemoryContext* context) {
    if (!context) {
        return;
    }
    MEM_VERBOSE(context->manager, "Resetting memory context '%s' (previous usage: %zu bytes)", 
              context->name, context->total_used);
    if (context->thread_safe) {
        pthread_mutex_lock(&context->mutex);
    }
    switch (context->strategy) {
        case MEM_STRATEGY_ARENA:
            MemoryBlock* block = context->first_block;
            while (block) {
                block->used = 0;
                block->has_free_space = true;
                block = block->next;
            }
            context->current_block = context->first_block;
            context->total_used = 0;
            break;
        case MEM_STRATEGY_POOL:
            PoolBlock* pool = context->first_pool;
            while (pool) {
                memset(pool->free_bitmap, 0, BITMAP_SIZE(pool->capacity));
                pool->free_count = pool->capacity;
                pool = pool->next;
            }
            context->total_used = 0;
            break;
        case MEM_STRATEGY_STANDARD:
            MEM_WARNING(context->manager, "Reset not fully supported for standard allocation strategy");
            break;
    }
    AllocationInfo* info = context->allocations;
    while (info) {
        AllocationInfo* next = info->next;
        free(info);
        info = next;
    }
    context->allocations = NULL;
    context->alloc_count = 0;
    context->free_count = 0;
    context->peak_usage = 0;
    if (context->thread_safe) {
        pthread_mutex_unlock(&context->mutex);
    }
}

void memory_get_stats(MemoryContext* context, MemoryStats* stats) {
    if (!context || !stats) {
        return;
    }
    if (context->thread_safe) {
        pthread_mutex_lock(&context->mutex);
    }
    stats->name = context->name;
    stats->strategy = context->strategy;
    stats->allocated = context->total_allocated;
    stats->used = context->total_used;
    stats->peak = context->peak_usage;
    stats->max_size = context->max_size;
    stats->alloc_count = context->alloc_count;
    stats->free_count = context->free_count;
    stats->block_count = 0;
    switch (context->strategy) {
        case MEM_STRATEGY_ARENA: {
            MemoryBlock* block = context->first_block;
            while (block) {
                stats->block_count++;
                block = block->next;
            }
            break;
        }
        case MEM_STRATEGY_POOL: {
            PoolBlock* pool = context->first_pool;
            while (pool) {
                stats->block_count++;
                pool = pool->next;
            }
            break;
        }
        default:
            stats->block_count = 0;
    }
    if (context->thread_safe) {
        pthread_mutex_unlock(&context->mutex);
    }
}

void memory_print_stats(MemoryContext* context, bool verbose) {
    if (!context) {
        return;
    }
    MemoryStats stats;
    memory_get_stats(context, &stats);
    printf("Memory Context '%s':\n", stats.name);
    printf("  Strategy: %s\n", 
           stats.strategy == MEM_STRATEGY_STANDARD ? "Standard" :
           stats.strategy == MEM_STRATEGY_ARENA ? "Arena" :
           stats.strategy == MEM_STRATEGY_POOL ? "Pool" : "Unknown");
    printf("  Allocated: %zu bytes\n", stats.allocated);
    printf("  Used: %zu bytes (%.1f%%)\n", stats.used, 
           stats.allocated > 0 ? (stats.used * 100.0 / stats.allocated) : 0.0);
    printf("  Peak usage: %zu bytes\n", stats.peak);
    printf("  Memory limit: %zu bytes\n", stats.max_size);
    printf("  Allocations: %zu\n", stats.alloc_count);
    printf("  Frees: %zu\n", stats.free_count);
    printf("  Blocks: %zu\n", stats.block_count);
    if (verbose) {
        if (context->strategy == MEM_STRATEGY_ARENA) {
            int block_idx = 0;
            MemoryBlock* block = context->first_block;
            printf("  Arena blocks:\n");
            while (block) {
                printf("    Block %d: %zu/%zu bytes used (%.1f%%)\n", 
                      block_idx++, block->used, block->size, 
                      (block->used * 100.0 / block->size));
                block = block->next;
            }
        } else if (context->strategy == MEM_STRATEGY_POOL) {
            int pool_idx = 0;
            PoolBlock* pool = context->first_pool;
            printf("  Pool blocks:\n");
            while (pool) {
                printf("    Pool %d: %zu/%zu objects free (%.1f%%), object size: %zu\n", 
                      pool_idx++, pool->free_count, pool->capacity,
                      (pool->free_count * 100.0 / pool->capacity),
                      pool->object_size);
                pool = pool->next;
            }
        }
        if (context->manager->leak_detection && context->allocations) {
            printf("  Active allocations:\n");
            if (context->thread_safe) {
                pthread_mutex_lock(&context->mutex);
            }
            int count = 0;
            AllocationInfo* info = context->allocations;
            while (info && count < 10) {
                printf("    %p: %zu bytes at %s:%d (%s)\n", 
                      info->ptr, info->size, info->file, info->line, info->func);
                info = info->next;
                count++;
            }
            if (info) {
                printf("    (... more allocations ...)\n");
            }
            if (context->thread_safe) {
                pthread_mutex_unlock(&context->mutex);
            }
        }
        MemoryContext* child = context->first_child;
        if (child) {
            printf("  Child contexts:\n");
            while (child) {
                printf("    '%s': %zu bytes used\n", child->name, child->total_used);
                child = child->next_sibling;
            }
        }
    }
}

MemoryContext* memory_get_parent(MemoryContext* context) {
    return context ? context->parent : NULL;
}

MemoryManager* memory_get_manager(MemoryContext* context) {
    return context ? context->manager : NULL;
}

MemoryContext* memory_find_context(MemoryManager* manager, const char* name) {
    if (!manager || !name) {
        return NULL;
    }
    pthread_mutex_lock(&manager->mutex);
    for (size_t i = 0; i < manager->context_count; i++) {
        if (strcmp(manager->contexts[i]->name, name) == 0) {
            MemoryContext* found = manager->contexts[i];
            pthread_mutex_unlock(&manager->mutex);
            return found;
        }
    }
    pthread_mutex_unlock(&manager->mutex);
    return NULL;
}

void* memory_alloc(MemoryContext* context, size_t size) {
    if (!context) {
        return NULL;
    }
    if (size == 0) {
        return NULL;
    }
    size_t aligned_size = ALIGN_SIZE(size);
    if (context->total_used + aligned_size > context->max_size) {
        MEM_ERROR(context->manager, "Memory limit exceeded for context '%s' (used: %zu, requested: %zu, limit: %zu)",
                context->name, context->total_used, aligned_size, context->max_size);
        if (context->manager->failure_handler) {
            if (context->manager->failure_handler(context, aligned_size, context->manager->failure_handler_data)) {
                return memory_alloc(context, size);
            }
        }
        return NULL;
    }
    void* ptr = NULL;
    if (context->thread_safe) {
        pthread_mutex_lock(&context->mutex);
    }
    switch (context->strategy) {
        case MEM_STRATEGY_ARENA: {
            if (!context->current_block || context->current_block->used + aligned_size > context->current_block->size) {
                size_t block_size = aligned_size > DEFAULT_ARENA_SIZE ? aligned_size : DEFAULT_ARENA_SIZE;
                MemoryBlock* new_block = create_memory_block(block_size);
                if (!new_block) {
                    if (context->thread_safe) {
                        pthread_mutex_unlock(&context->mutex);
                    }
                    MEM_ERROR(context->manager, "Failed to create memory block for context '%s'", context->name);
                    return NULL;
                }
                new_block->next = context->first_block;
                context->first_block = new_block;
                context->current_block = new_block;
                context->total_allocated += new_block->size;
            }
            ptr = (char*)context->current_block->memory + context->current_block->used;
            context->current_block->used += aligned_size;
            if (context->current_block->used + ALIGNMENT >= context->current_block->size) {
                context->current_block->has_free_space = false;
            }
            break;
        }
        case MEM_STRATEGY_POOL:
            if (aligned_size > context->pool_object_size) {
                MEM_ERROR(context->manager, 
                        "Requested allocation (%zu bytes) larger than pool object size (%zu bytes) in context '%s'",
                        aligned_size, context->pool_object_size, context->name);
                if (context->thread_safe) {
                    pthread_mutex_unlock(&context->mutex);
                }
                return NULL;
            }
            PoolBlock* pool = context->first_pool;
            while (pool) {
                if (pool->free_count > 0) {
                    for (size_t i = 0; i < pool->capacity; i++) {
                        if (!TEST_BIT(pool->free_bitmap, i)) {
                            SET_BIT(pool->free_bitmap, i);
                            pool->free_count--;
                            ptr = (char*)pool->memory + (i * pool->object_size);
                            break;
                        }
                    }
                    if (ptr) break;
                }
                pool = pool->next;
            }
            if (!ptr) {
                PoolBlock* new_pool = create_pool_block(context->pool_object_size, 0);
                if (!new_pool) {
                    if (context->thread_safe) {
                        pthread_mutex_unlock(&context->mutex);
                    }
                    MEM_ERROR(context->manager, "Failed to create pool block for context '%s'", context->name);
                    return NULL;
                }
                new_pool->next = context->first_pool;
                context->first_pool = new_pool;
                context->total_allocated += new_pool->object_size * new_pool->capacity;
                SET_BIT(new_pool->free_bitmap, 0);
                new_pool->free_count--;
                ptr = new_pool->memory;
            }
            break;
        case MEM_STRATEGY_STANDARD:
            ptr = malloc(aligned_size);
            if (ptr) {
                context->total_allocated += aligned_size;
            } else {
                MEM_ERROR(context->manager, "Failed to allocate %zu bytes with standard malloc in context '%s'", 
                       aligned_size, context->name);
            }
            break;
    }
    if (ptr) {
        context->total_used += aligned_size;
        if (context->total_used > context->peak_usage) {
            context->peak_usage = context->total_used;
        }
        context->alloc_count++;
        MEM_VERBOSE(context->manager, "Allocated %zu bytes at %p in context '%s' (total: %zu, peak: %zu)", 
                  aligned_size, ptr, context->name, context->total_used, context->peak_usage);
    }
    if (context->thread_safe) {
        pthread_mutex_unlock(&context->mutex);
    }
    return ptr;
}

void* memory_calloc(MemoryContext* context, size_t count, size_t size) {
    size_t total_size = count * size;
    void* ptr = memory_alloc(context, total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void* memory_realloc(MemoryContext* context, void* ptr, size_t size) {
    if (!context) {
        return NULL;
    }
    if (ptr == NULL) {
        return memory_alloc(context, size);
    }
    if (size == 0) {
        memory_free(context, ptr);
        return NULL;
    }
    if (context->strategy != MEM_STRATEGY_STANDARD) {
        MEM_ERROR(context->manager, "memory_realloc only supported for standard allocation strategy");
        return NULL;
    }
    size_t aligned_size = ALIGN_SIZE(size);
    if (context->thread_safe) {
        pthread_mutex_lock(&context->mutex);
    }
    size_t old_size = 0;
    if (context->manager->leak_detection) {
        AllocationInfo* info = context->allocations;
        while (info) {
            if (info->ptr == ptr) {
                old_size = info->size;
                break;
            }
            info = info->next;
        }
    }
    void* new_ptr = realloc(ptr, aligned_size);
    if (!new_ptr) {
        MEM_ERROR(context->manager, "Failed to reallocate %p to %zu bytes in context '%s'", 
                ptr, aligned_size, context->name);
    }
    if (new_ptr) {
        if (old_size > 0) {
            context->total_used = context->total_used - old_size + aligned_size;
            context->total_allocated = context->total_allocated - old_size + aligned_size;
            if (context->manager->leak_detection) {
                AllocationInfo* info = context->allocations;
                while (info) {
                    if (info->ptr == ptr) {
                        info->ptr = new_ptr;
                        info->size = aligned_size;
                        break;
                    }
                    info = info->next;
                }
            }
        } else {
            context->total_used += aligned_size;
            context->total_allocated += aligned_size;
        }
        if (context->total_used > context->peak_usage) {
            context->peak_usage = context->total_used;
        }
        MEM_VERBOSE(context->manager, "Reallocated %p to %p (%zu bytes) in context '%s'", 
                  ptr, new_ptr, aligned_size, context->name);
    } else {
        MEM_ERROR(context->manager, "Failed to reallocate %p to %zu bytes in context '%s'",
                ptr, aligned_size, context->name);
    }
    if (context->thread_safe) {
        pthread_mutex_unlock(&context->mutex);
    }
    return new_ptr;
}

char* memory_strdup(MemoryContext* context, const char* str) {
    if (!context || !str) {
        return NULL;
    }
    size_t len = strlen(str) + 1;
    char* new_str = memory_alloc(context, len);
    if (new_str) {
        memcpy(new_str, str, len);
    }
    return new_str;
}

void memory_free(MemoryContext* context, void* ptr) {
    if (!context || !ptr) {
        return;
    }
    if (context->strategy != MEM_STRATEGY_STANDARD) {
        MEM_WARNING(context->manager, "memory_free called on non-standard allocation strategy context '%s'",
                  context->name);
        return;
    }
    if (context->thread_safe) {
        pthread_mutex_lock(&context->mutex);
    }
    size_t freed_size = 0;
    if (context->manager->leak_detection) {
        AllocationInfo* info = context->allocations;
        while (info) {
            if (info->ptr == ptr) {
                freed_size = info->size;
                break;
            }
            info = info->next;
        }
        if (freed_size > 0) {
            untrack_allocation(context, ptr);
            context->total_used -= freed_size;
            context->total_allocated -= freed_size;
        } else {
            MEM_WARNING(context->manager, "Freeing untracked pointer %p in context '%s'", ptr, context->name);
        }
    }
    free(ptr);
    context->free_count++;
    MEM_VERBOSE(context->manager, "Freed %zu bytes at %p in context '%s'", freed_size, ptr, context->name);
    if (context->thread_safe) {
        pthread_mutex_unlock(&context->mutex);
    }
}

void* memory_pool_alloc(MemoryContext* context) {
    if (!context) {
        return NULL;
    }
    if (context->strategy != MEM_STRATEGY_POOL) {
        MEM_ERROR(context->manager, "memory_pool_alloc called on non-pool context '%s'", context->name);
        return NULL;
    }
    return memory_alloc(context, context->pool_object_size);
}

void memory_pool_free(MemoryContext* context, void* ptr) {
    if (!context || !ptr) {
        return;
    }
    if (context->strategy != MEM_STRATEGY_POOL) {
        MEM_ERROR(context->manager, "memory_pool_free called on non-pool context '%s'", context->name);
        return;
    }
    if (context->thread_safe) {
        pthread_mutex_lock(&context->mutex);
    }
    bool found = false;
    PoolBlock* pool = context->first_pool;
    while (pool && !found) {
        if (ptr >= pool->memory && 
            ptr < (char*)pool->memory + (pool->capacity * pool->object_size)) {
            size_t offset = (char*)ptr - (char*)pool->memory;
            size_t index = offset / pool->object_size;
            if (TEST_BIT(pool->free_bitmap, index)) {
                CLEAR_BIT(pool->free_bitmap, index);
                pool->free_count++;
                context->total_used -= pool->object_size;
                context->free_count++;
                found = true;
                MEM_VERBOSE(context->manager, "Returned object at %p to pool in context '%s'", 
                         ptr, context->name);
            } else {
                MEM_WARNING(context->manager, "Double free detected at %p in pool context '%s'",
                         ptr, context->name);
            }
            break;
        }
        pool = pool->next;
    }
    if (!found) {
        MEM_WARNING(context->manager, "Tried to free pointer %p not from pool context '%s'",
                 ptr, context->name);
    }
    if (context->manager->leak_detection) {
        untrack_allocation(context, ptr);
    }
    if (context->thread_safe) {
        pthread_mutex_unlock(&context->mutex);
    }
}

void memory_enable_leak_detection(MemoryManager* manager) {
    if (manager) {
        manager->leak_detection = true;
    }
}

void memory_disable_leak_detection(MemoryManager* manager) {
    if (manager) {
        manager->leak_detection = false;
    }
}

void memory_print_leaks(MemoryManager* manager) {
    if (!manager || !manager->leak_detection) {
        return;
    }
    printf("=== MEMORY LEAK REPORT ===\n");
    size_t total_leaks = 0;
    size_t total_bytes_leaked = 0;
    pthread_mutex_lock(&manager->mutex);
    for (size_t i = 0; i < manager->context_count; i++) {
        MemoryContext* ctx = manager->contexts[i];
        if (ctx->thread_safe) {
            pthread_mutex_lock(&ctx->mutex);
        }
        AllocationInfo* info = ctx->allocations;
        size_t context_leaks = 0;
        size_t context_bytes = 0;
        while (info) {
            context_leaks++;
            context_bytes += info->size;
            info = info->next;
        }
        if (context_leaks > 0) {
            printf("Context '%s': %zu leaks, %zu bytes\n", ctx->name, context_leaks, context_bytes);
            info = ctx->allocations;
            while (info) {
                printf("  %p: %zu bytes, allocated at %s:%d in %s\n", 
                     info->ptr, info->size, info->file, info->line, info->func);
                info = info->next;
            }
            total_leaks += context_leaks;
            total_bytes_leaked += context_bytes;
        }
        if (ctx->thread_safe) {
            pthread_mutex_unlock(&ctx->mutex);
        }
    }
    pthread_mutex_unlock(&manager->mutex);
    if (total_leaks > 0) {
        printf("\nTotal: %zu leaks, %zu bytes\n", total_leaks, total_bytes_leaked);
    } else {
        printf("No memory leaks detected.\n");
    }
    printf("==========================\n");
}