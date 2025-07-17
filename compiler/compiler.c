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

void compiler_init(CompilerOptions *options)
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
    options->log_level = DEBUG_LEVEL_ERROR; // Default log level is ERROR
}

void compiler_cleanup(CompilerOptions *options)
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

int compiler_parse_args(int argc, char **argv, CompilerOptions *options)
{
    DEBUG_VERBOSE("Parsing command line arguments, argc=%d", argc);
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <source_file> [-o <output_file>] [-v] [-l <level>]\n", argv[0]);
        fprintf(stderr, "  -o <output_file>   Specify output file (default is source_file.o)\n");
        fprintf(stderr, "  -v                 Verbose mode\n");
        fprintf(stderr, "  -l <level>         Set log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)\n");
        return 0;
    }

    DEBUG_VERBOSE("Source file argument: %s", argv[1]);
    options->source_file = malloc(strlen(argv[1]) + 1);
    if (options->source_file == NULL)
    {
        DEBUG_ERROR("Failed to allocate memory for source file");
        return 0;
    }
    strcpy(options->source_file, argv[1]);

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
        strcpy(options->output_file + base_len, ".s");
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
            options->output_file = malloc(strlen(argv[++i]) + 1);
            if (options->output_file == NULL)
            {
                DEBUG_ERROR("Failed to allocate memory for output file");
                return 0;
            }
            strcpy(options->output_file, argv[i]);
            DEBUG_VERBOSE("Output file set to: %s", options->output_file);
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            options->verbose = 1;
            DEBUG_VERBOSE("Verbose mode enabled");
        }
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
        {
            // Parse log level
            i++;
            int log_level = atoi(argv[i]);

            // Ensure log level is valid
            if (log_level < DEBUG_LEVEL_NONE || log_level > DEBUG_LEVEL_VERBOSE)
            {
                DEBUG_WARNING("Invalid log level: %d, using default", log_level);
            }
            else
            {
                options->log_level = log_level;
                DEBUG_VERBOSE("Log level set to: %d", options->log_level);
            }
        }
        else
        {
            DEBUG_WARNING("Unknown option: %s", argv[i]);
        }
    }

    return 1;
}

char *compiler_read_file(const char *path)
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