// tests/arena_tests.c

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "../arena.h"
#include "../debug.h"

void test_arena_init() {
    printf("Testing arena_init...\n");
    Arena arena;
    size_t initial_size = 16;
    arena_init(&arena, initial_size);
    assert(arena.first != NULL);
    assert(arena.first->data != NULL);
    assert(arena.first->size == initial_size);
    assert(arena.first->next == NULL);
    assert(arena.current == arena.first);
    assert(arena.current_used == 0);
    assert(arena.block_size == initial_size);
    arena_free(&arena);  // Clean up
}

void test_arena_alloc_small() {
    printf("Testing arena_alloc small allocations...\n");
    Arena arena;
    arena_init(&arena, 16);

    // Allocate 4 bytes (aligned to 8)
    void *p1 = arena_alloc(&arena, 4);
    assert(p1 == arena.current->data);
    assert(arena.current_used == 8);

    // Allocate another 4 bytes (aligned to 8)
    void *p2 = arena_alloc(&arena, 4);
    assert(p2 == (char *)arena.current->data + 8);
    assert(arena.current_used == 16);

    // Allocate 1 byte, should fit if any space left, but 16 used == size, so new block
    void *p3 = arena_alloc(&arena, 1);
    assert(arena.current == arena.first->next);
    assert(arena.current->size == 32);  // Doubled from 16
    assert(arena.block_size == 32);
    assert(p3 == arena.current->data);
    assert(arena.current_used == 8);

    arena_free(&arena);
}

void test_arena_alloc_large() {
    printf("Testing arena_alloc large allocations...\n");
    Arena arena;
    arena_init(&arena, 16);

    // Allocate something small first
    arena_alloc(&arena, 4);  // Uses 8 bytes, 8 left

    // Allocate 20 bytes (aligned to 24), 8 + 24 = 32 > 16, so new block
    // new_block_size = 16*2=32, 32 >=24, ok
    void *p1 = arena_alloc(&arena, 20);
    assert(arena.current == arena.first->next);
    assert(arena.current->size == 32);
    assert(arena.block_size == 32);
    assert(arena.current_used == 24);
    assert(p1 == arena.current->data);

    // Allocate 50 bytes (aligned to 56), current used=24 +56=80 >32, so new block
    // new_block_size=32*2=64, 64>=56, ok
    void *p2 = arena_alloc(&arena, 50);
    assert(arena.current == arena.first->next->next);
    assert(arena.current->size == 64);
    assert(arena.block_size == 64);
    assert(arena.current_used == 56);
    assert(p2 == arena.current->data);

    // Allocate 100 bytes (aligned to 104), current=56+104=160>64, new block
    // new_block_size=64*2=128, 128>104, ok
    void *p3 = arena_alloc(&arena, 100);
    assert(arena.current == arena.first->next->next->next);
    assert(arena.current->size == 128);
    assert(arena.block_size == 128);
    assert(arena.current_used == 104);
    assert(p3 == arena.current->data);

    arena_free(&arena);
}

void test_arena_alloc_larger_than_double() {
    printf("Testing arena_alloc larger than double...\n");
    Arena arena;
    arena_init(&arena, 16);

    // Allocate 100 bytes (aligned 104), current 0+104>16, new block
    // new_block_size=16*2=32 <104, so set to 104
    void *p1 = arena_alloc(&arena, 100);
    assert(arena.current == arena.first->next);
    assert(arena.current->size == 104);
    assert(arena.block_size == 104);
    assert(arena.current_used == 104);
    assert(p1 == arena.current->data);

    arena_free(&arena);
}

void test_arena_alloc_zero() {
    printf("Testing arena_alloc zero size...\n");
    Arena arena;
    arena_init(&arena, 16);

    // Allocate 0 bytes (aligned 0), should return current ptr but not advance
    void *p1 = arena_alloc(&arena, 0);
    assert(p1 == arena.current->data);
    assert(arena.current_used == 0);  // No advance

    // Next alloc should be at same place
    void *p2 = arena_alloc(&arena, 0);
    assert(p2 == p1);
    assert(arena.current_used == 0);

    // Now alloc 1, should advance
    void *p3 = arena_alloc(&arena, 1);
    assert(p3 == arena.current->data);
    assert(arena.current_used == 8);

    arena_free(&arena);
}

void test_arena_strdup() {
    printf("Testing arena_strdup...\n");
    Arena arena;
    arena_init(&arena, 16);

    // NULL input
    char *s1 = arena_strdup(&arena, NULL);
    assert(s1 == NULL);

    // Empty string
    char *s2 = arena_strdup(&arena, "");
    assert(strcmp(s2, "") == 0);
    assert(arena.current_used == 8);  // 1 byte aligned to 8

    // Normal string "hello" (6 bytes incl null, aligned 8)
    char *s3 = arena_strdup(&arena, "hello");
    assert(strcmp(s3, "hello") == 0);
    assert(arena.current_used == 16);  // 8 + 8 =16

    // Longer string to force new block "this is a longer string" (23+1=24, aligned 24)
    char *s4 = arena_strdup(&arena, "this is a longer string");
    assert(strcmp(s4, "this is a longer string") == 0);
    assert(arena.current == arena.first->next);
    assert(arena.current_used == 24);

    arena_free(&arena);
}

void test_arena_strndup() {
    printf("Testing arena_strndup...\n");
    Arena arena;
    arena_init(&arena, 16);

    // NULL input
    char *s1 = arena_strndup(&arena, NULL, 5);
    assert(s1 == NULL);

    // Empty string, n=5
    char *s2 = arena_strndup(&arena, "", 5);
    assert(strcmp(s2, "") == 0);
    assert(arena.current_used == 8);  // 1 byte aligned

    // String "hello", n=3 -> "hel"
    char *s3 = arena_strndup(&arena, "hello", 3);
    assert(strcmp(s3, "hel") == 0);
    assert(arena.current_used == 16);  // 8 + 8 (4 bytes aligned)

    // String "hello", n=10 > len -> "hello"
    char *s4 = arena_strndup(&arena, "hello", 10);
    assert(strcmp(s4, "hello") == 0);
    assert(arena.current == arena.first->next);
    assert(arena.current_used == 8);  // 6 bytes aligned to 8

    // String "abc", n=0 -> ""
    char *s5 = arena_strndup(&arena, "abc", 0);
    assert(strcmp(s5, "") == 0);
    assert(arena.current_used == 16);  // 8 + 8 (1 byte aligned)

    arena_free(&arena);
}

void test_arena_free() {
    printf("Testing arena_free...\n");
    Arena arena;
    arena_init(&arena, 16);

    // Allocate some stuff to create multiple blocks
    arena_alloc(&arena, 10);  // 16 bytes used (aligned 16)
    arena_alloc(&arena, 10);  // New block
    arena_strdup(&arena, "test");

    arena_free(&arena);
    assert(arena.first == NULL);
    assert(arena.current == NULL);
    assert(arena.current_used == 0);
    assert(arena.block_size == 0);

    // Should be able to re-init after free
    arena_init(&arena, 32);
    assert(arena.first != NULL);
    assert(arena.first->size == 32);
    arena_free(&arena);
}

int main() {
    test_arena_init();
    test_arena_alloc_small();
    test_arena_alloc_large();
    test_arena_alloc_larger_than_double();
    test_arena_alloc_zero();
    test_arena_strdup();
    test_arena_strndup();
    test_arena_free();

    printf("All tests passed!\n");
    return 0;
}