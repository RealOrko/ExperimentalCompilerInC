#include "arena_tests.c"
#include "ast_tests.c"
#include "file_tests.c"
#include "parser_tests.c"
#include "token_tests.c"
#include "lexer_tests.c"

int main()
{

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

    // *** AST ***

    test_ast_create_primitive_type();
    test_ast_create_array_type();
    test_ast_create_function_type();
    test_ast_clone_type();
    test_ast_type_equals();
    test_ast_type_to_string();
    test_ast_create_binary_expr();
    test_ast_create_unary_expr();
    test_ast_create_literal_expr();
    test_ast_create_variable_expr();
    test_ast_create_assign_expr();
    test_ast_create_call_expr();
    test_ast_create_array_expr();
    test_ast_create_array_access_expr();
    test_ast_create_increment_expr();
    test_ast_create_decrement_expr();
    test_ast_create_interpolated_expr();
    test_ast_create_comparison_expr();
    test_ast_create_expr_stmt();
    test_ast_create_var_decl_stmt();
    test_ast_create_function_stmt();
    test_ast_create_return_stmt();
    test_ast_create_block_stmt();
    test_ast_create_if_stmt();
    test_ast_create_while_stmt();
    test_ast_create_for_stmt();
    test_ast_create_import_stmt();
    test_ast_init_module();
    test_ast_module_add_statement();
    test_ast_clone_token();
    test_ast_print();

    // *** File ***

    test_file_read_null_arena();
    test_file_read_null_path();
    test_file_read_nonexistent_file();
    test_file_read_empty_file();
    test_file_read_small_file();
    test_file_read_large_file();
    test_file_read_seek_failure();
    test_file_read_read_failure();
    test_file_read_special_characters();

    // *** Parser ***

    test_empty_program_parsing();
    test_var_decl_parsing();
    test_function_no_params_parsing();
    test_if_statement_parsing();
    test_simple_program_parsing();
    test_while_loop_parsing();
    test_for_loop_parsing();
    test_interpolated_string_parsing();
    test_literal_types_parsing();
    test_recursive_function_parsing();
    test_full_program_parsing();

    // *** Token ***

    test_token_init();
    test_token_is_type_keyword();
    test_token_get_array_token_type();
    test_token_set_int_literal();
    test_token_set_double_literal();
    test_token_set_char_literal();
    test_token_set_string_literal();
    test_token_set_bool_literal();
    test_token_type_to_string();
    test_token_print();

    // *** Lexer ***

    test_lexer_init();
    test_lexer_simple_identifier();
    test_lexer_keywords();
    test_lexer_bool_literals();
    test_lexer_number_literals();
    test_lexer_string_literal();
    test_lexer_interpol_string();
    test_lexer_char_literal();
    test_lexer_operators();
    test_lexer_indentation();
    test_lexer_inconsistent_indent();
    test_lexer_unterminated_string();
    test_lexer_invalid_escape();
    test_lexer_comments();
    test_lexer_empty_line_ignore();
    test_lexer_dedent_at_eof();
    test_lexer_invalid_character();
    test_lexer_multiline_string();

    printf("All tests passed!\n");

    return 0;
}