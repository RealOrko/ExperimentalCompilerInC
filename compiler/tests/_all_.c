#include "arena_tests.c"
#include "parser_tests.c"

int main() {

    // *** Debugging ***
    printf("Running tests with debug level: %d\n", DEBUG_LEVEL_VERBOSE);
    init_debug(DEBUG_LEVEL_ERROR);

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
    
    test_empty_program_parsing();
    test_var_decl_parsing();
    test_function_no_params_parsing();
    test_if_statement_parsing(); // NOT sure about this one, needs more debugging
    test_simple_program_parsing();
    test_while_loop_parsing();
    test_for_loop_parsing();
    test_interpolated_string_parsing();
    test_literal_types_parsing();
    test_recursive_function_parsing();
    //test_full_program_parsing();

    printf("All tests passed!\n");
    
    return 0;
}