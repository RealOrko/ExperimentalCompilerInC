/**
 * compiler.c
 * Implementation of the main compiler module with improved error handling
 */

#include "compiler.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void init_compiler_options(CompilerOptions *options)
{
    DEBUG_VERBOSE("Initializing compiler options");
    if (options == NULL)
    {
        DEBUG_ERROR("Null compiler options pointer");
        return;
    }
    options->source_file = NULL;
    options->output_file = NULL;
    options->verbose = 0;
}

void compiler_options_cleanup(CompilerOptions *options)
{
    DEBUG_VERBOSE("Cleaning up compiler options");
    if (options == NULL)
    {
        DEBUG_ERROR("Null compiler options pointer");
        return;
    }

    if (options->source_file != NULL)
    {
        free(options->source_file);
        options->source_file = NULL;
    }

    if (options->output_file != NULL)
    {
        free(options->output_file);
        options->output_file = NULL;
    }
}

int parse_args(int argc, char **argv, CompilerOptions *options)
{
    DEBUG_VERBOSE("Parsing command line arguments, argc=%d", argc);
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <source_file> [-o <output_file>] [-v]\n", argv[0]);
        return 0;
    }

    DEBUG_VERBOSE("Source file argument: %s", argv[1]);
    options->source_file = strdup(argv[1]);
    if (options->source_file == NULL)
    {
        DEBUG_ERROR("Failed to allocate memory for source file");
        return 0;
    }

    // Default output file name is the source file name with .o extension
    char *dot = strrchr(options->source_file, '.');
    if (dot != NULL)
    {
        // Replace extension
        int base_len = dot - options->source_file;
        options->output_file = malloc(base_len + 3);
        if (options->output_file == NULL)
        {
            DEBUG_ERROR("Failed to allocate memory for output file");
            return 0;
        }
        strncpy(options->output_file, options->source_file, base_len);
        strcpy(options->output_file + base_len, ".o");
    }
    else
    {
        // No extension in source file, append .o
        options->output_file = malloc(strlen(options->source_file) + 3);
        if (options->output_file == NULL)
        {
            DEBUG_ERROR("Failed to allocate memory for output file");
            return 0;
        }
        strcpy(options->output_file, options->source_file);
        strcat(options->output_file, ".o");
    }

    DEBUG_VERBOSE("Default output file: %s", options->output_file);

    // Parse additional arguments
    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            free(options->output_file);
            options->output_file = strdup(argv[++i]);
            if (options->output_file == NULL)
            {
                DEBUG_ERROR("Failed to allocate memory for output file");
                return 0;
            }
            DEBUG_VERBOSE("Output file set to: %s", options->output_file);
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            options->verbose = 1;
            DEBUG_VERBOSE("Verbose mode enabled");
        }
        else
        {
            DEBUG_WARNING("Unknown option: %s", argv[i]);
        }
    }

    return 1;
}

char *read_file(const char *path)
{
    DEBUG_VERBOSE("Reading file: %s", path);
    if (path == NULL)
    {
        DEBUG_ERROR("Null file path");
        return NULL;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        DEBUG_ERROR("Could not open file '%s'", path);
        return NULL;
    }

    // Get file size
    DEBUG_VERBOSE("Getting file size");
    if (fseek(file, 0, SEEK_END) != 0)
    {
        DEBUG_ERROR("fseek failed");
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0)
    {
        DEBUG_ERROR("ftell failed");
        fclose(file);
        return NULL;
    }

    DEBUG_VERBOSE("File size: %ld bytes", size);

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        DEBUG_ERROR("fseek failed");
        fclose(file);
        return NULL;
    }

    // Allocate buffer
    DEBUG_VERBOSE("Allocating buffer");
    char *buffer = malloc(size + 1);
    if (buffer == NULL)
    {
        DEBUG_ERROR("Not enough memory to read file '%s'", path);
        fclose(file);
        return NULL;
    }

    // Read file content
    DEBUG_VERBOSE("Reading file content");
    size_t bytes_read = fread(buffer, 1, size, file);
    if (bytes_read < (size_t)size)
    {
        DEBUG_ERROR("Could not read file '%s', read %zu of %ld bytes",
                    path, bytes_read, size);
        free(buffer);
        fclose(file);
        return NULL;
    }

    // Null-terminate the buffer
    buffer[size] = '\0';

    fclose(file);
    DEBUG_VERBOSE("Successfully read file: %s", path);
    return buffer;
}

int compile_source(const char *source, const char *filename, const char *output_path, int verbose)
{
    DEBUG_INFO("Compiling %s...", filename);

    if (source == NULL)
    {
        DEBUG_ERROR("Null source pointer");
        return 0;
    }

    if (filename == NULL)
    {
        DEBUG_ERROR("Null filename pointer");
        return 0;
    }

    if (output_path == NULL)
    {
        DEBUG_ERROR("Null output path pointer");
        return 0;
    }

    // Create the lexer
    DEBUG_VERBOSE("Initializing lexer");
    Lexer lexer;
    init_lexer(&lexer, source, filename);

    // Create the parser
    DEBUG_VERBOSE("Initializing parser");
    Parser parser;
    init_parser(&parser, &lexer);

    // Parse the source into an AST
    DEBUG_VERBOSE("Parsing source code");
    Module *module = parse(&parser, filename);
    if (module == NULL)
    {
        DEBUG_ERROR("Parsing failed");
        parser_cleanup(&parser);
        return 0;
    }

    DEBUG_INFO("Parsing successful");

    // Type check the AST
    DEBUG_VERBOSE("Type checking AST");
    TypeChecker checker;
    init_type_checker(&checker, parser.symbol_table);

    if (!type_check_module(&checker, module))
    {
        DEBUG_ERROR("Type checking failed");
        free_module(module);
        free(module);
        type_checker_cleanup(&checker);
        parser_cleanup(&parser);
        return 0;
    }

    DEBUG_INFO("Type checking successful");

    // Generate assembly code
    DEBUG_VERBOSE("Generating assembly code");
    CodeGen gen;
    init_code_gen(&gen, parser.symbol_table, output_path);

    generate_module(&gen, module);

    code_gen_cleanup(&gen);

    DEBUG_INFO("Code generation successful");

    // Clean up
    DEBUG_VERBOSE("Cleaning up compiler resources");
    free_module(module);
    free(module);
    type_checker_cleanup(&checker);
    parser_cleanup(&parser);

    return 1;
}

int compile_file(const char *source_path, const char *output_path, int verbose)
{
    DEBUG_INFO("Compiling file %s to %s", source_path, output_path);

    if (source_path == NULL)
    {
        DEBUG_ERROR("Null source path");
        return 0;
    }

    if (output_path == NULL)
    {
        DEBUG_ERROR("Null output path");
        return 0;
    }

    // Read the source file
    DEBUG_VERBOSE("Reading source file");
    char *source = read_file(source_path);
    if (source == NULL)
    {
        DEBUG_ERROR("Failed to read source file");
        return 0;
    }

    int result = compile_source(source, source_path, output_path, verbose);

    DEBUG_VERBOSE("Freeing source buffer");
    free(source);

    return result;
}

int execute(const char *output_path)
{
    DEBUG_INFO("Executing %s", output_path);

    if (output_path == NULL)
    {
        DEBUG_ERROR("Null output path");
        return 0;
    }

    // Create a temporary executable file name
    char exec_path[256];
    snprintf(exec_path, sizeof(exec_path), "a.out");

    // Create a command to assemble and link the output file
    char command[512];
    snprintf(command, sizeof(command), "nasm -f elf64 %s && gcc -no-pie -o %s %s",
             output_path, exec_path, output_path);

    // Execute the command
    DEBUG_VERBOSE("Running command: %s", command);
    int status = system(command);
    if (status != 0)
    {
        DEBUG_ERROR("Assembly or linking failed, status: %d", status);
        return 0;
    }

    // Execute the compiled program
    DEBUG_INFO("Executing %s:", exec_path);
    printf("-----------------------------------\n");

    status = system(exec_path);

    printf("-----------------------------------\n");
    DEBUG_INFO("Program exited with status %d", WEXITSTATUS(status));

    return WEXITSTATUS(status);
}