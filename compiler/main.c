/**
 * main.c
 * Complete rewrite to fix memory and type checking issues
 */

#include "compiler.h"
#include "debug.h"
#include "type_checker.h"  // New include
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    // Variables that need cleanup
    CompilerOptions options;
    char *source = NULL;
    Module *module = NULL;

    // Initialize compiler options
    compiler_init(&options);

    if (!compiler_parse_args(argc, argv, &options))
    {
        DEBUG_ERROR("Failed to parse command line arguments");
        compiler_cleanup(&options);
        return 1;
    }

    // Initialize debugging with the specified log level
    init_debug(options.log_level);

    DEBUG_INFO("Source file: %s", options.source_file);
    DEBUG_INFO("Output file: %s", options.output_file);
    DEBUG_INFO("Log level: %d", options.log_level);

    // Read the source file
    source = compiler_read_file(options.source_file);
    if (source == NULL)
    {
        DEBUG_ERROR("Failed to read source file: %s", options.source_file);
        compiler_cleanup(&options);
        return 1;
    }

    DEBUG_INFO("Successfully read source file, length: %ld", strlen(source));

    // Create lexer and parser
    Lexer lexer;
    lexer_init(&lexer, source, options.source_file);
    DEBUG_INFO("Lexer initialized");

    Parser parser;
    parser_init(&parser, &lexer);
    DEBUG_INFO("Parser initialized");

    // Parse the source into AST
    DEBUG_INFO("Starting to parse file");
    module = parser_execute(&parser, options.source_file);
    if (module == NULL)
    {
        DEBUG_ERROR("Parsing failed");
        free(source);
        parser_cleanup(&parser);
        compiler_cleanup(&options);
        return 1;
    }

    DEBUG_INFO("Parsing successful, statement count: %d", module->count);

    // Add built-in functions
    DEBUG_INFO("Adding built-in functions");

    // Built-in print (takes any printable type, but type checker handles specially)
    Token print_token;
    print_token.start = "print";
    print_token.length = 5;
    print_token.line = 0;
    print_token.type = TOKEN_IDENTIFIER;

    // Parameter type is placeholder (string), but special-cased in type checker
    Type *placeholder_type = ast_create_primitive_type(TYPE_STRING);
    Type **param_types = malloc(sizeof(Type *));
    if (param_types == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    param_types[0] = placeholder_type;

    Type *void_type = ast_create_primitive_type(TYPE_VOID);
    Type *print_type = ast_create_function_type(void_type, param_types, 1);
    ast_free_type(void_type); // Free temp after cloning

    symbol_table_add_symbol(parser.symbol_table, print_token, print_type);
    ast_free_type(print_type);  // Free after adding (cloned in symbol table)
    free(param_types);          // Free temp array after cloning
    ast_free_type(placeholder_type); // Free original after cloning

    // Type checking
    DEBUG_INFO("Starting type checking");
    int type_check_success = type_check_module(module, parser.symbol_table);

    if (!type_check_success)
    {
        DEBUG_ERROR("Type checking failed");
        if (module != NULL)
        {
            ast_free_module(module);
            free(module);
            module = NULL;
        }

        // Clear symbol table without freeing types again
        SymbolTable *table = parser.symbol_table;
        parser.symbol_table = NULL; // Prevent parser_cleanup from freeing it again
        symbol_table_cleanup(table);

        parser_cleanup(&parser);
        free(source);
        compiler_cleanup(&options);

        return 1;
    }
    DEBUG_INFO("Type checking successful");

    // Generate code
    DEBUG_INFO("Starting code generation");
    CodeGen gen;
    code_gen_init(&gen, parser.symbol_table, options.output_file);

    code_gen_module(&gen, module);
    DEBUG_INFO("Code generation completed successfully");

    // Clean up code generator
    code_gen_cleanup(&gen);

    DEBUG_INFO("Compilation successful: %s -> %s", options.source_file, options.output_file);

    // Clean up in reverse order of initialization
    DEBUG_INFO("Cleaning up resources");

    if (module != NULL)
    {
        ast_free_module(module);
        free(module);
        module = NULL;
    }

    // Clear symbol table without freeing types again
    SymbolTable *table = parser.symbol_table;
    parser.symbol_table = NULL; // Prevent parser_cleanup from freeing it again
    symbol_table_cleanup(table);

    parser_cleanup(&parser);
    free(source);
    compiler_cleanup(&options);

    return 0;
}
