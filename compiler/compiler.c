#include "compiler.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void compiler_init(CompilerOptions *options, int argc, char **argv)
{
    if (options == NULL)
    {
        return;
    }
    
    arena_init(&options->arena, 1024);
    options->source_file = NULL;
    options->output_file = NULL;
    options->verbose = 0;
    options->log_level = DEBUG_LEVEL_ERROR;

    if (!compiler_parse_args(argc, argv, options))
    {
        return;
    }
}

void compiler_cleanup(CompilerOptions *options)
{
    if (options == NULL)
    {
        return;
    }

    arena_free(&options->arena);
    options->source_file = NULL;
    options->output_file = NULL;
}

int compiler_parse_args(int argc, char **argv, CompilerOptions *options)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <source_file> [-o <output_file>] [-v] [-l <level>]\n", argv[0]);
        fprintf(stderr, "  -o <output_file>   Specify output file (default is source_file.o)\n");
        fprintf(stderr, "  -v                 Verbose mode\n");
        fprintf(stderr, "  -l <level>         Set log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)\n");
        return 0;
    }

    options->source_file = arena_strdup(&options->arena, argv[1]);

    char *dot = strrchr(options->source_file, '.');
    if (dot != NULL)
    {
        size_t base_len = dot - options->source_file;
        char *out = arena_alloc(&options->arena, base_len + 3);
        strncpy(out, options->source_file, base_len);
        strcpy(out + base_len, ".s");
        options->output_file = out;
    }
    else
    {
        size_t len = strlen(options->source_file);
        char *out = arena_alloc(&options->arena, len + 3);
        strcpy(out, options->source_file);
        strcat(out, ".o");
        options->output_file = out;
    }

    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            options->output_file = arena_strdup(&options->arena, argv[++i]);
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            options->verbose = 1;
        }
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
        {
            i++;
            int log_level = atoi(argv[i]);

            if (log_level < DEBUG_LEVEL_NONE || log_level > DEBUG_LEVEL_VERBOSE)
            {
            }
            else
            {
                options->log_level = log_level;
            }
        }
        else
        {
        }
    }

    return 1;
}

char *compiler_read_file(Arena *arena, const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0)
    {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return NULL;
    }

    char *buffer = arena_alloc(arena, size + 1);

    size_t bytes_read = fread(buffer, 1, size, file);
    if (bytes_read < (size_t)size)
    {
        fclose(file);
        return NULL;
    }

    buffer[size] = '\0';

    fclose(file);
    return buffer;
}