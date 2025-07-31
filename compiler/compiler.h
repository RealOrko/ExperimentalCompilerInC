#ifndef COMPILER_H
#define COMPILER_H

#include "lexer.h"
#include "parser.h"
#include "code_gen.h"
#include "arena.h"
#include <stdio.h>

typedef struct
{
    Arena arena;
    Lexer lexer;
    Parser parser;
    char *source_file;
    char *output_file;
    char *source;
    int verbose;
    int log_level;
} CompilerOptions;

void compiler_init(CompilerOptions *options);
void compiler_cleanup(CompilerOptions *options);
int compiler_parse_args(int argc, char **argv, CompilerOptions *options);
char *compiler_read_file(Arena *arena, const char *path);

#endif