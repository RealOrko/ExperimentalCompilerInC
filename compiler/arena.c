// arena.c
#include "arena.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

#define ARENA_ALIGNMENT 8 // Align allocations to 8 bytes (common for x64)

static size_t align_up(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

void arena_init(Arena *arena, size_t initial_block_size)
{
    arena->block_size = initial_block_size;
    arena->first = malloc(sizeof(Block));
    if (arena->first == NULL)
    {
        DEBUG_ERROR("Failed to initialize arena block");
        exit(1);
    }
    arena->first->data = malloc(initial_block_size);
    if (arena->first->data == NULL)
    {
        DEBUG_ERROR("Failed to allocate initial arena block data");
        free(arena->first);
        exit(1);
    }
    arena->first->size = initial_block_size;
    arena->first->next = NULL;
    arena->current = arena->first;
    arena->current_used = 0;
}

void *arena_alloc(Arena *arena, size_t size)
{
    size = align_up(size, ARENA_ALIGNMENT);

    if (arena->current_used + size > arena->current->size)
    {
        size_t new_block_size = arena->block_size * 2;
        if (new_block_size < size)
        {
            new_block_size = size;
        }
        Block *new_block = malloc(sizeof(Block));
        if (new_block == NULL)
        {
            DEBUG_ERROR("Failed to allocate new arena block");
            exit(1);
        }
        new_block->data = malloc(new_block_size);
        if (new_block->data == NULL)
        {
            DEBUG_ERROR("Failed to allocate new arena block data");
            free(new_block);
            exit(1);
        }
        new_block->size = new_block_size;
        new_block->next = NULL;
        arena->current->next = new_block;
        arena->current = new_block;
        arena->current_used = 0;
        arena->block_size = new_block_size;
    }

    void *ptr = arena->current->data + arena->current_used;
    arena->current_used += size;
    return ptr;
}

char *arena_strdup(Arena *arena, const char *str)
{
    if (str == NULL)
        return NULL;
    size_t len = strlen(str);
    char *dup = arena_alloc(arena, len + 1);
    memcpy(dup, str, len + 1);
    return dup;
}

char *arena_strndup(Arena *arena, const char *str, size_t n)
{
    if (str == NULL)
        return NULL;
    size_t len = strnlen(str, n);
    char *dup = arena_alloc(arena, len + 1);
    memcpy(dup, str, len);
    dup[len] = '\0';
    return dup;
}

Token *ast_dup_token(Arena *arena, const Token *token)
{
    if (token == NULL)
    {
        return NULL;
    }
    Token *dup = arena_alloc(arena, sizeof(Token));
    if (dup == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating token");
        exit(1);
    }
    *dup = *token;
    dup->start = arena_strndup(arena, token->start, token->length);
    if (dup->start == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating token start");
        exit(1);
    }
    dup->filename = arena_strdup(arena, token->filename);
    if (dup->filename == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating token filename");
        exit(1);
    }
    return dup;
}

void arena_free(Arena *arena)
{
    Block *block = arena->first;
    while (block != NULL)
    {
        Block *next = block->next;
        free(block->data);
        free(block);
        block = next;
    }
    arena->first = NULL;
    arena->current = NULL;
    arena->current_used = 0;
    arena->block_size = 0;
}