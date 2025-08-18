#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include "token.h"

typedef struct Block
{
    char *data;
    size_t size;
    struct Block *next;
} Block;

typedef struct
{
    Block *first;
    Block *current;
    size_t current_used;
    size_t block_size;
} Arena;

void arena_init(Arena *arena, size_t initial_block_size);
void *arena_alloc(Arena *arena, size_t size);
char *arena_strdup(Arena *arena, const char *str);
char *arena_strndup(Arena *arena, const char *str, size_t n);
Token *ast_dup_token(Arena *arena, const Token *token);
void arena_free(Arena *arena);

#endif