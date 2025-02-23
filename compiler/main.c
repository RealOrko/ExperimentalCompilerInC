#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "ast.h"
#include "codegen.h"

int debug_enabled = 0; // Define here

int main(int argc, char* argv[]) {
    const char* output_name = "program";
    const char* input_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            debug_enabled = 1;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_name = argv[i + 1];
            i++;
        } else if (input_file == NULL) {
            input_file = argv[i];
        }
    }

    if (input_file == NULL) {
        printf("Usage: %s [-d] <input_file> [-o <output_name>]\n", argv[0]);
        return 1;
    }

    FILE* input = fopen(input_file, "r");
    if (!input) {
        perror("Error opening input file");
        return 1;
    }

    fseek(input, 0, SEEK_END);
    long size = ftell(input);
    fseek(input, 0, SEEK_SET);
    char* source = malloc(size + 1);
    fread(source, 1, size, input);
    source[size] = '\0';
    fclose(input);

    int token_count;
    Token* tokens = tokenize(source, &token_count);
    if (!tokens) {
        free(source);
        return 1;
    }

    ASTNode* ast = parse(tokens, token_count);
    if (!ast) {
        free_tokens(tokens, token_count);
        free(source);
        return 1;
    }

    FILE* output = fopen("output.asm", "w");
    if (!output) {
        perror("Error opening output.asm");
        free_ast(ast);
        free_tokens(tokens, token_count);
        free(source);
        return 1;
    }
    generate_code(ast, output);
    fclose(output);

    char command[512];
    snprintf(command, sizeof(command), "nasm -f elf64 output.asm -o output.o");
    if (system(command) != 0) {
        fprintf(stderr, "Assembly failed\n");
    } else {
        snprintf(command, sizeof(command), "ld output.o -o %s", output_name);
        if (system(command) != 0) {
            fprintf(stderr, "Linking failed\n");
        } else {
            printf("Compilation completed. Run './%s' to execute.\n", output_name);
        }
    }

    free_ast(ast);
    free_tokens(tokens, token_count);
    free(source);
    return 0;
}