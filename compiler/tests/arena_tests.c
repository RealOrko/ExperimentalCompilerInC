// tests/arena_tests.c

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../arena.h"
#include "../debug.h"

void test_arena_init()
{
    DEBUG_INFO("Starting test_arena_init");
    printf("Testing arena_init...\n");
    Arena arena;
    size_t initial_size = 16;
    DEBUG_VERBOSE("Initializing arena with size: %zu", initial_size);
    arena_init(&arena, initial_size);
    DEBUG_VERBOSE("Arena first: %p", arena.first);
    DEBUG_VERBOSE("Arena first->data: %p", arena.first ? arena.first->data : NULL);
    DEBUG_VERBOSE("Arena first->size: %zu", arena.first ? arena.first->size : 0);
    DEBUG_VERBOSE("Arena first->next: %p", arena.first ? arena.first->next : NULL);
    DEBUG_VERBOSE("Arena current: %p", arena.current);
    DEBUG_VERBOSE("Arena current_used: %zu", arena.current_used);
    DEBUG_VERBOSE("Arena block_size: %zu", arena.block_size);
    assert(arena.first != NULL);
    assert(arena.first->data != NULL);
    assert(arena.first->size == initial_size);
    assert(arena.first->next == NULL);
    assert(arena.current == arena.first);
    assert(arena.current_used == 0);
    assert(arena.block_size == initial_size);
    DEBUG_INFO("Cleaning up arena in test_arena_init");
    arena_free(&arena); // Clean up
    DEBUG_INFO("Finished test_arena_init");
}

void test_arena_alloc_small()
{
    DEBUG_INFO("Starting test_arena_alloc_small");
    printf("Testing arena_alloc small allocations...\n");
    Arena arena;
    DEBUG_VERBOSE("Initializing arena with size 16");
    arena_init(&arena, 16);

    // Allocate 4 bytes (aligned to 8)
    DEBUG_VERBOSE("Allocating 4 bytes");
    void *p1 = arena_alloc(&arena, 4);
    DEBUG_VERBOSE("Allocated p1: %p", p1);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(p1 == arena.current->data);
    assert(arena.current_used == 8);

    // Allocate another 4 bytes (aligned to 8)
    DEBUG_VERBOSE("Allocating another 4 bytes");
    void *p2 = arena_alloc(&arena, 4);
    DEBUG_VERBOSE("Allocated p2: %p", p2);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(p2 == (char *)arena.current->data + 8);
    assert(arena.current_used == 16);

    // Allocate 1 byte, should fit if any space left, but 16 used == size, so new block
    DEBUG_VERBOSE("Allocating 1 byte, expecting new block");
    void *p3 = arena_alloc(&arena, 1);
    DEBUG_VERBOSE("Allocated p3: %p", p3);
    DEBUG_VERBOSE("Current: %p", arena.current);
    DEBUG_VERBOSE("First->next: %p", arena.first->next);
    DEBUG_VERBOSE("Current->size: %zu", arena.current->size);
    DEBUG_VERBOSE("Block size: %zu", arena.block_size);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(arena.current == arena.first->next);
    assert(arena.current->size == 32); // Doubled from 16
    assert(arena.block_size == 32);
    assert(p3 == arena.current->data);
    assert(arena.current_used == 8);

    DEBUG_INFO("Cleaning up arena in test_arena_alloc_small");
    arena_free(&arena);
    DEBUG_INFO("Finished test_arena_alloc_small");
}

void test_arena_alloc_large()
{
    DEBUG_INFO("Starting test_arena_alloc_large");
    printf("Testing arena_alloc large allocations...\n");
    Arena arena;
    DEBUG_VERBOSE("Initializing arena with size 16");
    arena_init(&arena, 16);

    // Allocate something small first
    DEBUG_VERBOSE("Allocating 4 bytes first");
    arena_alloc(&arena, 4); // Uses 8 bytes, 8 left
    DEBUG_VERBOSE("Current used after small alloc: %zu", arena.current_used);

    // Allocate 20 bytes (aligned to 24), 8 + 24 = 32 > 16, so new block
    // new_block_size = 16*2=32, 32 >=24, ok
    DEBUG_VERBOSE("Allocating 20 bytes, expecting new block");
    void *p1 = arena_alloc(&arena, 20);
    DEBUG_VERBOSE("Allocated p1: %p", p1);
    DEBUG_VERBOSE("Current: %p", arena.current);
    DEBUG_VERBOSE("Current->size: %zu", arena.current->size);
    DEBUG_VERBOSE("Block size: %zu", arena.block_size);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(arena.current == arena.first->next);
    assert(arena.current->size == 32);
    assert(arena.block_size == 32);
    assert(arena.current_used == 24);
    assert(p1 == arena.current->data);

    // Allocate 50 bytes (aligned to 56), current used=24 +56=80 >32, so new block
    // new_block_size=32*2=64, 64>=56, ok
    DEBUG_VERBOSE("Allocating 50 bytes, expecting new block");
    void *p2 = arena_alloc(&arena, 50);
    DEBUG_VERBOSE("Allocated p2: %p", p2);
    DEBUG_VERBOSE("Current: %p", arena.current);
    DEBUG_VERBOSE("Current->size: %zu", arena.current->size);
    DEBUG_VERBOSE("Block size: %zu", arena.block_size);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(arena.current == arena.first->next->next);
    assert(arena.current->size == 64);
    assert(arena.block_size == 64);
    assert(arena.current_used == 56);
    assert(p2 == arena.current->data);

    // Allocate 100 bytes (aligned to 104), current=56+104=160>64, new block
    // new_block_size=64*2=128, 128>104, ok
    DEBUG_VERBOSE("Allocating 100 bytes, expecting new block");
    void *p3 = arena_alloc(&arena, 100);
    DEBUG_VERBOSE("Allocated p3: %p", p3);
    DEBUG_VERBOSE("Current: %p", arena.current);
    DEBUG_VERBOSE("Current->size: %zu", arena.current->size);
    DEBUG_VERBOSE("Block size: %zu", arena.block_size);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(arena.current == arena.first->next->next->next);
    assert(arena.current->size == 128);
    assert(arena.block_size == 128);
    assert(arena.current_used == 104);
    assert(p3 == arena.current->data);

    DEBUG_INFO("Cleaning up arena in test_arena_alloc_large");
    arena_free(&arena);
    DEBUG_INFO("Finished test_arena_alloc_large");
}

void test_arena_alloc_larger_than_double()
{
    DEBUG_INFO("Starting test_arena_alloc_larger_than_double");
    printf("Testing arena_alloc larger than double...\n");
    Arena arena;
    DEBUG_VERBOSE("Initializing arena with size 16");
    arena_init(&arena, 16);

    // Allocate 100 bytes (aligned 104), current 0+104>16, new block
    // new_block_size=16*2=32 <104, so set to 104
    DEBUG_VERBOSE("Allocating 100 bytes, expecting adjusted block size");
    void *p1 = arena_alloc(&arena, 100);
    DEBUG_VERBOSE("Allocated p1: %p", p1);
    DEBUG_VERBOSE("Current: %p", arena.current);
    DEBUG_VERBOSE("Current->size: %zu", arena.current->size);
    DEBUG_VERBOSE("Block size: %zu", arena.block_size);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(arena.current == arena.first->next);
    assert(arena.current->size == 104);
    assert(arena.block_size == 104);
    assert(arena.current_used == 104);
    assert(p1 == arena.current->data);

    DEBUG_INFO("Cleaning up arena in test_arena_alloc_larger_than_double");
    arena_free(&arena);
    DEBUG_INFO("Finished test_arena_alloc_larger_than_double");
}

void test_arena_alloc_zero()
{
    DEBUG_INFO("Starting test_arena_alloc_zero");
    printf("Testing arena_alloc zero size...\n");
    Arena arena;
    DEBUG_VERBOSE("Initializing arena with size 16");
    arena_init(&arena, 16);

    // Allocate 0 bytes (aligned 0), should return current ptr but not advance
    DEBUG_VERBOSE("Allocating 0 bytes");
    void *p1 = arena_alloc(&arena, 0);
    DEBUG_VERBOSE("Allocated p1: %p", p1);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(p1 == arena.current->data);
    assert(arena.current_used == 0); // No advance

    // Next alloc should be at same place
    DEBUG_VERBOSE("Allocating another 0 bytes");
    void *p2 = arena_alloc(&arena, 0);
    DEBUG_VERBOSE("Allocated p2: %p", p2);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(p2 == p1);
    assert(arena.current_used == 0);

    // Now alloc 1, should advance
    DEBUG_VERBOSE("Allocating 1 byte after zeros");
    void *p3 = arena_alloc(&arena, 1);
    DEBUG_VERBOSE("Allocated p3: %p", p3);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(p3 == arena.current->data);
    assert(arena.current_used == 8);

    DEBUG_INFO("Cleaning up arena in test_arena_alloc_zero");
    arena_free(&arena);
    DEBUG_INFO("Finished test_arena_alloc_zero");
}

void test_arena_strdup()
{
    DEBUG_INFO("Starting test_arena_strdup");
    printf("Testing arena_strdup...\n");
    Arena arena;
    DEBUG_VERBOSE("Initializing arena with size 16");
    arena_init(&arena, 16);

    // NULL input
    DEBUG_VERBOSE("strdup NULL");
    char *s1 = arena_strdup(&arena, NULL);
    DEBUG_VERBOSE("Result s1: %p", s1);
    assert(s1 == NULL);

    // Empty string
    DEBUG_VERBOSE("strdup empty string");
    char *s2 = arena_strdup(&arena, "");
    DEBUG_VERBOSE("Result s2: %s", s2);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(strcmp(s2, "") == 0);
    assert(arena.current_used == 8); // 1 byte aligned to 8

    // Normal string "hello" (6 bytes incl null, aligned 8)
    DEBUG_VERBOSE("strdup 'hello'");
    char *s3 = arena_strdup(&arena, "hello");
    DEBUG_VERBOSE("Result s3: %s", s3);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(strcmp(s3, "hello") == 0);
    assert(arena.current_used == 16); // 8 + 8 =16

    // Longer string to force new block "this is a longer string" (23+1=24, aligned 24)
    DEBUG_VERBOSE("strdup 'this is a longer string'");
    char *s4 = arena_strdup(&arena, "this is a longer string");
    DEBUG_VERBOSE("Result s4: %s", s4);
    DEBUG_VERBOSE("Current: %p", arena.current);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(strcmp(s4, "this is a longer string") == 0);
    assert(arena.current == arena.first->next);
    assert(arena.current_used == 24);

    DEBUG_INFO("Cleaning up arena in test_arena_strdup");
    arena_free(&arena);
    DEBUG_INFO("Finished test_arena_strdup");
}

void test_arena_strndup()
{
    DEBUG_INFO("Starting test_arena_strndup");
    printf("Testing arena_strndup...\n");
    Arena arena;
    DEBUG_VERBOSE("Initializing arena with size 16");
    arena_init(&arena, 16);

    // NULL input
    DEBUG_VERBOSE("strndup NULL, n=5");
    char *s1 = arena_strndup(&arena, NULL, 5);
    DEBUG_VERBOSE("Result s1: %p", s1);
    assert(s1 == NULL);

    // Empty string, n=5
    DEBUG_VERBOSE("strndup empty, n=5");
    char *s2 = arena_strndup(&arena, "", 5);
    DEBUG_VERBOSE("Result s2: %s", s2);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(strcmp(s2, "") == 0);
    assert(arena.current_used == 8); // 1 byte aligned

    // String "hello", n=3 -> "hel"
    DEBUG_VERBOSE("strndup 'hello', n=3");
    char *s3 = arena_strndup(&arena, "hello", 3);
    DEBUG_VERBOSE("Result s3: %s", s3);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(strcmp(s3, "hel") == 0);
    assert(arena.current_used == 16); // 8 + 8 (4 bytes aligned)

    // String "hello", n=10 > len -> "hello"
    DEBUG_VERBOSE("strndup 'hello', n=10");
    char *s4 = arena_strndup(&arena, "hello", 10);
    DEBUG_VERBOSE("Result s4: %s", s4);
    DEBUG_VERBOSE("Current: %p", arena.current);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(strcmp(s4, "hello") == 0);
    assert(arena.current == arena.first->next);
    assert(arena.current_used == 8); // 6 bytes aligned to 8

    // String "abc", n=0 -> ""
    DEBUG_VERBOSE("strndup 'abc', n=0");
    char *s5 = arena_strndup(&arena, "abc", 0);
    DEBUG_VERBOSE("Result s5: %s", s5);
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    assert(strcmp(s5, "") == 0);
    assert(arena.current_used == 16); // 8 + 8 (1 byte aligned)

    DEBUG_INFO("Cleaning up arena in test_arena_strndup");
    arena_free(&arena);
    DEBUG_INFO("Finished test_arena_strndup");
}

void test_arena_free()
{
    DEBUG_INFO("Starting test_arena_free");
    printf("Testing arena_free...\n");
    Arena arena;
    DEBUG_VERBOSE("Initializing arena with size 16");
    arena_init(&arena, 16);

    // Allocate some stuff to create multiple blocks
    DEBUG_VERBOSE("Allocating 10 bytes");
    arena_alloc(&arena, 10); // 16 bytes used (aligned 16)
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    DEBUG_VERBOSE("Allocating another 10 bytes, new block");
    arena_alloc(&arena, 10); // New block
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);
    DEBUG_VERBOSE("strdup 'test'");
    arena_strdup(&arena, "test");
    DEBUG_VERBOSE("Current used: %zu", arena.current_used);

    DEBUG_VERBOSE("Freeing arena");
    arena_free(&arena);
    DEBUG_VERBOSE("Arena first after free: %p", arena.first);
    DEBUG_VERBOSE("Arena current after free: %p", arena.current);
    DEBUG_VERBOSE("Arena current_used after free: %zu", arena.current_used);
    DEBUG_VERBOSE("Arena block_size after free: %zu", arena.block_size);
    assert(arena.first == NULL);
    assert(arena.current == NULL);
    assert(arena.current_used == 0);
    assert(arena.block_size == 0);

    // Should be able to re-init after free
    DEBUG_VERBOSE("Re-initializing arena with size 32");
    arena_init(&arena, 32);
    DEBUG_VERBOSE("Arena first after re-init: %p", arena.first);
    DEBUG_VERBOSE("Arena first->size: %zu", arena.first->size);
    assert(arena.first != NULL);
    assert(arena.first->size == 32);
    DEBUG_INFO("Cleaning up arena in test_arena_free");
    arena_free(&arena);
    DEBUG_INFO("Finished test_arena_free");
}