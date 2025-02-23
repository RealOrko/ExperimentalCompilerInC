#include <stdio.h>
#include <string.h>
#include "codegen.h"

static int string_count = 0;
static char* string_values[MAX_STRINGS];

static int find_or_add_string(const char* value, FILE* output) {
    for (int i = 0; i < string_count; i++) {
        if (strcmp(string_values[i], value) == 0) return i;
    }
    string_values[string_count] = strdup(value);
    fprintf(output, "    str%d db \"%s\", 10\n", string_count, value);
    fprintf(output, "    str%dlen equ $ - str%d\n", string_count, string_count);
    return string_count++;
}

static void generate_call(ASTNode* node, FILE* output) {
    if (strcmp(node->value, "std.print") == 0 && node->params) {
        if (node->params->type == AST_IDENT) {
            fprintf(output, "    mov rax, 1\n");
            fprintf(output, "    mov rdi, 1\n");
            fprintf(output, "    mov rsi, [rbp + 16]\n");
            fprintf(output, "    mov rdx, [rbp + 24]\n");
            fprintf(output, "    syscall\n");
        } else if (node->params->type == AST_STRING) {
            int str_id = find_or_add_string(node->params->value, output);
            fprintf(output, "    mov rax, 1\n");
            fprintf(output, "    mov rdi, 1\n");
            fprintf(output, "    mov rsi, str%d\n", str_id);
            fprintf(output, "    mov rdx, str%dlen\n", str_id);
            fprintf(output, "    syscall\n");
        }
    } else {
        ASTNode* param = node->params;
        int param_count = 0;
        while (param) {
            if (param->type == AST_STRING) {
                int str_id = find_or_add_string(param->value, output);
                fprintf(output, "    push str%dlen\n", str_id);
                fprintf(output, "    push str%d\n", str_id);
                param_count += 2;
            }
            param = param->next;
        }
        fprintf(output, "    call %s\n", node->value);
        if (param_count > 0) {
            fprintf(output, "    add rsp, %d\n", param_count * 8);
        }
    }
}

static void generate_func(ASTNode* node, FILE* output) {
    if (debug_enabled) printf("Generating function: %s\n", node->value);
    if (strcmp(node->value, "main") == 0) {
        fprintf(output, "_start:\n");
    } else {
        fprintf(output, "%s:\n", node->value);
    }
    fprintf(output, "    push rbp\n");
    fprintf(output, "    mov rbp, rsp\n");

    ASTNode* stmt = node->body;
    while (stmt) {
        if (stmt->type == AST_CALL) {
            generate_call(stmt, output);
        }
        stmt = stmt->next;
    }

    fprintf(output, "    mov rsp, rbp\n");
    fprintf(output, "    pop rbp\n");
    if (strcmp(node->value, "main") == 0) {
        fprintf(output, "    mov rax, 60\n");
        fprintf(output, "    xor rdi, rdi\n");
        fprintf(output, "    syscall\n");
    } else {
        fprintf(output, "    ret\n");
    }
}

void generate_code(ASTNode* ast, FILE* output) {
    string_count = 0;
    fprintf(output, "section .data\n");
    ASTNode* current = ast;
    while (current) {
        if (current->type == AST_FUNC_DEF) {
            ASTNode* stmt = current->body;
            while (stmt) {
                if (stmt->type == AST_CALL) {
                    ASTNode* param = stmt->params;
                    while (param) {
                        if (param->type == AST_STRING) {
                            find_or_add_string(param->value, output);
                        }
                        param = param->next;
                    }
                }
                stmt = stmt->next;
            }
        }
        current = current->next;
    }
    fprintf(output, "\n");

    fprintf(output, "section .text\n");
    fprintf(output, "    global _start\n\n");

    current = ast;
    while (current) {
        if (current->type == AST_FUNC_DEF) {
            generate_func(current, output);
        }
        current = current->next;
    }
}