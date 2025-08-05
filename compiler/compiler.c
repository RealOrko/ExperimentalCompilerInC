#include "compiler.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

void compiler_init(CompilerOptions *options, int argc, char **argv)
{
    if (!options) {
        DEBUG_ERROR("CompilerOptions is NULL");
        exit(1);
    }

    arena_init(&options->arena, 4096);
    options->source_file = NULL;
    options->output_file = NULL;
    options->source = NULL;
    options->verbose = 0;
    options->log_level = DEBUG_LEVEL_ERROR;

    init_debug(options->log_level);

    if (!compiler_parse_args(argc, argv, options)) {
        compiler_cleanup(options);
        exit(1);
    }

    if (options->source_file) {
        options->source = file_read(&options->arena, options->source_file);
        if (!options->source) {
            DEBUG_ERROR("Failed to read source file: %s", options->source_file);
            compiler_cleanup(options);
            exit(1);
        }
    } else {
        DEBUG_ERROR("No source file specified");
        compiler_cleanup(options);
        exit(1);
    }

    lexer_init(&options->arena, &options->lexer, options->source, options->source_file);
    parser_init(&options->arena, &options->parser, &options->lexer);
}

void compiler_cleanup(CompilerOptions *options)
{
    if (!options) {
        return;
    }

    parser_cleanup(&options->parser);
    lexer_cleanup(&options->lexer);
    arena_free(&options->arena);
    
    options->source_file = NULL;
    options->output_file = NULL;
    options->source = NULL;
}

int compiler_parse_args(int argc, char **argv, CompilerOptions *options)
{
    if (argc < 2) {
        DEBUG_ERROR(
            "Usage: %s <source_file> [-o <output_file>] [-v] [-l <level>]\n"
            "  -o <output_file>   Specify output file (default is source_file.s)\n"
            "  -v                 Verbose mode\n"
            "  -l <level>         Set log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)",
            argv[0]);
        return 0;
    }

    // Allocate source_file using arena
    options->source_file = arena_strdup(&options->arena, argv[1]);
    if (!options->source_file) {
        DEBUG_ERROR("Failed to allocate memory for source file path");
        return 0;
    }

    // Generate default output file name
    const char *dot = strrchr(options->source_file, '.');
    size_t base_len = dot ? (size_t)(dot - options->source_file) : strlen(options->source_file);
    size_t out_len = base_len + 3; // ".s" + null terminator
    char *out = arena_alloc(&options->arena, out_len);
    if (!out) {
        DEBUG_ERROR("Failed to allocate memory for output file path");
        return 0;
    }
    
    strncpy(out, options->source_file, base_len);
    strcpy(out + base_len, ".s");
    options->output_file = out;

    // Parse additional arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            options->output_file = arena_strdup(&options->arena, argv[++i]);
            if (!options->output_file) {
                DEBUG_ERROR("Failed to allocate memory for output file path");
                return 0;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            options->verbose = 1;
        } else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc) {
            i++;
            int log_level = atoi(argv[i]);
            if (log_level < DEBUG_LEVEL_NONE || log_level > DEBUG_LEVEL_VERBOSE) {
                DEBUG_ERROR("Invalid log level: %s", argv[i]);
                return 0;
            }
            options->log_level = log_level;
            init_debug(log_level); // Update debug level if changed
        } else {
            DEBUG_ERROR("Unknown option: %s", argv[i]);
            return 0;
        }
    }

    return 1;
}
