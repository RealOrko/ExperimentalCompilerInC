// arena.c
#include "arena.h"
#include "debug.h"  // Assuming you have DEBUG_ERROR
#include <stdlib.h>
#include <string.h>

#define ARENA_ALIGNMENT 8  // Align allocations to 8 bytes (common for x64)

static size_t align_up(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

void arena_init(Arena *arena, size_t initial_capacity) {
    arena->buffer = malloc(initial_capacity);
    if (arena->buffer == NULL) {
        DEBUG_ERROR("Failed to initialize arena");
        exit(1);
    }
    arena->capacity = initial_capacity;
    arena->used = 0;
}

void *arena_alloc(Arena *arena, size_t size) {
    size = align_up(size, ARENA_ALIGNMENT);
    if (arena->used + size > arena->capacity) {
        // Grow the arena (double capacity)
        size_t new_capacity = arena->capacity * 2;
        if (new_capacity < arena->used + size) {
            new_capacity = arena->used + size;
        }
        char *new_buffer = realloc(arena->buffer, new_capacity);
        if (new_buffer == NULL) {
            DEBUG_ERROR("Arena growth failed");
            exit(1);
        }
        arena->buffer = new_buffer;
        arena->capacity = new_capacity;
    }
    void *ptr = arena->buffer + arena->used;
    arena->used += size;
    return ptr;
}

char *arena_strdup(Arena *arena, const char *str) {
    if (str == NULL) return NULL;
    size_t len = strlen(str);
    char *dup = arena_alloc(arena, len + 1);
    memcpy(dup, str, len + 1);
    return dup;
}

char *arena_strndup(Arena *arena, const char *str, size_t n) {
    if (str == NULL) return NULL;
    size_t len = strnlen(str, n);
    char *dup = arena_alloc(arena, len + 1);
    memcpy(dup, str, len);
    dup[len] = '\0';
    return dup;
}

void arena_free(Arena *arena) {
    free(arena->buffer);
    arena->buffer = NULL;
    arena->capacity = 0;
    arena->used = 0;
}