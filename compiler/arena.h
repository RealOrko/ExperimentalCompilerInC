// arena.h
#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct Arena {
    char *buffer;      // The backing buffer
    size_t capacity;   // Total allocated size
    size_t used;       // Bytes used so far
} Arena;

void arena_init(Arena *arena, size_t initial_capacity);
void *arena_alloc(Arena *arena, size_t size);  // Alloc raw memory (aligned)
char *arena_strdup(Arena *arena, const char *str);  // Dup a string
char *arena_strndup(Arena *arena, const char *str, size_t n);  // Dup n bytes
void arena_free(Arena *arena);  // Free everything

#endif