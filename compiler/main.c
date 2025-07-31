#include "compiler.h"
#include "debug.h"
#include "type_checker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    CompilerOptions options;
    char *source = NULL;
    Module *module = NULL;

    compiler_init(&options);

    if (!compiler_parse_args(argc, argv, &options))
    {
        compiler_cleanup(&options);
        return 1;
    }

    init_debug(options.log_level);

    source = compiler_read_file(&options.arena, options.source_file);
    if (source == NULL)
    {
        compiler_cleanup(&options);
        return 1;
    }

    Lexer lexer;
    lexer_init(&lexer, source, options.source_file);

    Parser parser;
    parser_init(&parser, &lexer);

    module = parser_execute(&parser, options.source_file);
    if (module == NULL)
    {
        parser_cleanup(&parser);
        compiler_cleanup(&options);
        return 1;
    }

    Token print_token;
    print_token.start = "print";
    print_token.length = 5;
    print_token.line = 0;
    print_token.type = TOKEN_IDENTIFIER;

    Type *placeholder_type = ast_create_primitive_type(TYPE_STRING);
    Type **param_types = malloc(sizeof(Type *));
    if (param_types == NULL)
    {
        exit(1);
    }
    param_types[0] = placeholder_type;

    Type *void_type = ast_create_primitive_type(TYPE_VOID);
    Type *print_type = ast_create_function_type(void_type, param_types, 1);
    ast_free_type(void_type);

    symbol_table_add_symbol(parser.symbol_table, print_token, print_type);
    ast_free_type(print_type);
    free(param_types);
    ast_free_type(placeholder_type);

    Token to_string_token;
    to_string_token.start = "to_string";
    to_string_token.length = 9;
    to_string_token.line = 0;
    to_string_token.type = TOKEN_IDENTIFIER;

    placeholder_type = ast_create_primitive_type(TYPE_STRING);
    param_types = malloc(sizeof(Type *));
    if (param_types == NULL)
    {
        exit(1);
    }
    param_types[0] = placeholder_type;

    Type *string_type = ast_create_primitive_type(TYPE_STRING);
    Type *to_string_type = ast_create_function_type(string_type, param_types, 1);
    ast_free_type(string_type);

    symbol_table_add_symbol(parser.symbol_table, to_string_token, to_string_type);
    ast_free_type(to_string_type);
    free(param_types);
    ast_free_type(placeholder_type);

    int type_check_success = type_check_module(module, parser.symbol_table);

    if (!type_check_success)
    {
        if (module != NULL)
        {
            ast_free_module(module);
            free(module);
            module = NULL;
        }

        SymbolTable *table = parser.symbol_table;
        parser.symbol_table = NULL;
        symbol_table_cleanup(table);

        parser_cleanup(&parser);
        compiler_cleanup(&options);

        return 1;
    }

    CodeGen gen;
    code_gen_init(&gen, parser.symbol_table, options.output_file);

    code_gen_module(&gen, module);

    code_gen_cleanup(&gen);

    if (module != NULL)
    {
        ast_free_module(module);
        free(module);
        module = NULL;
    }

    SymbolTable *table = parser.symbol_table;
    parser.symbol_table = NULL;
    symbol_table_cleanup(table);

    parser_cleanup(&parser);
    compiler_cleanup(&options);

    return 0;
}