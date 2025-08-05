#include "arena_tests.c"
#include "parser_tests.c"

int main() {

    // *** Arena ***

    test_arena_init();
    test_arena_alloc_small();
    test_arena_alloc_large();
    test_arena_alloc_larger_than_double();
    test_arena_alloc_zero();
    test_arena_strdup();
    test_arena_strndup();
    test_arena_free();

    // *** Parser ***
    
    test_simple_program_parsing();

    printf("All tests passed!\n");
    
    return 0;
}