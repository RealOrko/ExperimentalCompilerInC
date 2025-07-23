#include "compiler.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

void compiler_init(CompilerOptions *options)
{
    if (options == NULL)
    {
        return;
    }
    options->source_file = NULL;
    options->output_file = NULL;
    options->verbose = 0;
    options->log_level = DEBUG_LEVEL_ERROR;
}

void compiler_cleanup(CompilerOptions *options)
{
    if (options == NULL)
    {
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
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <source_file> [-o <output_file>] [-v] [-l <level>]\n", argv[0]);
        fprintf(stderr, "  -o <output_file>   Specify output file (default is source_file.o)\n");
        fprintf(stderr, "  -v                 Verbose mode\n");
        fprintf(stderr, "  -l <level>         Set log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)\n");
        return 0;
    }

    options->source_file = malloc(strlen(argv[1]) + 1);
    if (options->source_file == NULL)
    {
        return 0;
    }
    strcpy(options->source_file, argv[1]);

    char *dot = strrchr(options->source_file, '.');
    if (dot != NULL)
    {
        int base_len = dot - options->source_file;
        options->output_file = malloc(base_len + 3);
        if (options->output_file == NULL)
        {
            return 0;
        }
        strncpy(options->output_file, options->source_file, base_len);
        strcpy(options->output_file + base_len, ".s");
    }
    else
    {
        options->output_file = malloc(strlen(options->source_file) + 3);
        if (options->output_file == NULL)
        {
            return 0;
        }
        strcpy(options->output_file, options->source_file);
        strcat(options->output_file, ".o");
    }

    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            free(options->output_file);
            options->output_file = malloc(strlen(argv[++i]) + 1);
            if (options->output_file == NULL)
            {
                return 0;
            }
            strcpy(options->output_file, argv[i]);
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

char *compiler_read_file(const char *path)
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

    char *buffer = malloc(size + 1);
    if (buffer == NULL)
    {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, size, file);
    if (bytes_read < (size_t)size)
    {
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[size] = '\0';

    fclose(file);
    return buffer;
}