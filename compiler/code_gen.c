#include "code_gen.h"
#include "debug.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>

static StringLiteral *string_literals = NULL;

static void pre_build_symbols(CodeGen *gen, Stmt *stmt);

void code_gen_init_function_stack(CodeGen *gen)
{
    gen->function_stack_capacity = 8;
    gen->function_stack_size = 0;
    gen->function_stack = malloc(gen->function_stack_capacity * sizeof(char *));
    if (gen->function_stack == NULL)
    {
        exit(1);
    }
}

void code_gen_push_function_context(CodeGen *gen)
{
    if (gen->function_stack_size >= gen->function_stack_capacity)
    {
        gen->function_stack_capacity *= 2;
        gen->function_stack = realloc(gen->function_stack,
                                      gen->function_stack_capacity * sizeof(char *));
        if (gen->function_stack == NULL)
        {
            exit(1);
        }
    }
    if (gen->current_function != NULL)
    {
        gen->function_stack[gen->function_stack_size] = strdup(gen->current_function);
        if (gen->function_stack[gen->function_stack_size] == NULL)
        {
            exit(1);
        }
    }
    else
    {
        gen->function_stack[gen->function_stack_size] = NULL;
    }
    gen->function_stack_size++;
}

void code_gen_pop_function_context(CodeGen *gen)
{
    if (gen->function_stack_size <= 0)
    {
        return;
    }
    if (gen->current_function != NULL)
    {
        free(gen->current_function);
    }
    gen->function_stack_size--;
    gen->current_function = gen->function_stack[gen->function_stack_size];
    gen->function_stack[gen->function_stack_size] = NULL;
}

void code_gen_free_function_stack(CodeGen *gen)
{
    if (gen->function_stack == NULL)
    {
        return;
    }
    for (int i = 0; i < gen->function_stack_size; i++)
    {
        if (gen->function_stack[i] != NULL)
        {
            free(gen->function_stack[i]);
        }
    }
    free(gen->function_stack);
    gen->function_stack = NULL;
    gen->function_stack_size = 0;
    gen->function_stack_capacity = 0;
}

void code_gen_init(CodeGen *gen, SymbolTable *symbol_table, const char *output_file)
{
    gen->label_count = 0;
    gen->symbol_table = symbol_table;
    gen->output = fopen(output_file, "w");
    gen->current_function = NULL;
    gen->current_return_type = NULL;
    code_gen_init_function_stack(gen);
    if (gen->output == NULL)
    {
        exit(1);
    }
    string_literals = NULL;
}

void code_gen_cleanup(CodeGen *gen)
{
    if (gen->output != NULL)
    {
        fclose(gen->output);
    }
    if (gen->current_function != NULL)
    {
        free(gen->current_function);
        gen->current_function = NULL;
    }
    code_gen_free_function_stack(gen);
    StringLiteral *current = string_literals;
    while (current != NULL)
    {
        StringLiteral *next = current->next;
        if (current->string != NULL)
        {
            free((void *)current->string);
        }
        free(current);
        current = next;
    }
    string_literals = NULL;
}

int code_gen_new_label(CodeGen *gen)
{
    return gen->label_count++;
}

int code_gen_add_string_literal(CodeGen *gen, const char *string)
{
    if (string == NULL)
    {
        return -1;
    }
    int label = code_gen_new_label(gen);
    StringLiteral *literal = malloc(sizeof(StringLiteral));
    if (literal == NULL)
    {
        exit(1);
    }
    literal->string = strdup(string);
    if (literal->string == NULL)
    {
        free(literal);
        exit(1);
    }
    literal->label = label;
    literal->next = string_literals;
    string_literals = literal;
    return label;
}

void code_gen_string_literal(CodeGen *gen, const char *string, int label)
{
    fprintf(gen->output, "str_%d db ", label);
    if (*string == '\0')
    {
        fprintf(gen->output, "0\n");
        return;
    }
    const unsigned char *p = (const unsigned char *)string;
    int first = 1;
    while (*p)
    {
        if (!first)
            fprintf(gen->output, ", ");
        first = 0;
        switch (*p)
        {
        case '\n':
            fprintf(gen->output, "10");
            break;
        case '\t':
            fprintf(gen->output, "9");
            break;
        case '\\':
            fprintf(gen->output, "92");
            break;
        default:
            fprintf(gen->output, "'%c'", *p);
        }
        p++;
    }
    fprintf(gen->output, ", 0\n");
}

void code_gen_data_section(CodeGen *gen)
{
    fprintf(gen->output, "section .data\n");
    fprintf(gen->output, "empty_str db 0\n");
    StringLiteral *current = string_literals;
    while (current != NULL)
    {
        if (current->string != NULL)
        {
            code_gen_string_literal(gen, current->string, current->label);
        }
        current = current->next;
    }
    fprintf(gen->output, "\n");
}

void code_gen_text_section(CodeGen *gen)
{
    fprintf(gen->output, "section .text\n");
    fprintf(gen->output, "    global main\n\n");
    fprintf(gen->output, "    extern rt_str_concat\n");
    fprintf(gen->output, "    extern rt_print_long\n");
    fprintf(gen->output, "    extern rt_print_double\n");
    fprintf(gen->output, "    extern rt_print_char\n");
    fprintf(gen->output, "    extern rt_print_string\n");
    fprintf(gen->output, "    extern rt_print_bool\n");
    fprintf(gen->output, "    extern rt_add_long\n");
    fprintf(gen->output, "    extern rt_sub_long\n");
    fprintf(gen->output, "    extern rt_mul_long\n");
    fprintf(gen->output, "    extern rt_div_long\n");
    fprintf(gen->output, "    extern rt_mod_long\n");
    fprintf(gen->output, "    extern rt_eq_long\n");
    fprintf(gen->output, "    extern rt_ne_long\n");
    fprintf(gen->output, "    extern rt_lt_long\n");
    fprintf(gen->output, "    extern rt_le_long\n");
    fprintf(gen->output, "    extern rt_gt_long\n");
    fprintf(gen->output, "    extern rt_ge_long\n");
    fprintf(gen->output, "    extern rt_add_double\n");
    fprintf(gen->output, "    extern rt_sub_double\n");
    fprintf(gen->output, "    extern rt_mul_double\n");
    fprintf(gen->output, "    extern rt_div_double\n");
    fprintf(gen->output, "    extern rt_eq_double\n");
    fprintf(gen->output, "    extern rt_ne_double\n");
    fprintf(gen->output, "    extern rt_lt_double\n");
    fprintf(gen->output, "    extern rt_le_double\n");
    fprintf(gen->output, "    extern rt_gt_double\n");
    fprintf(gen->output, "    extern rt_ge_double\n");
    fprintf(gen->output, "    extern rt_neg_long\n");
    fprintf(gen->output, "    extern rt_neg_double\n");
    fprintf(gen->output, "    extern rt_not_bool\n");
    fprintf(gen->output, "    extern rt_post_inc_long\n");
    fprintf(gen->output, "    extern rt_post_dec_long\n");
    fprintf(gen->output, "    extern rt_to_string_long\n");
    fprintf(gen->output, "    extern rt_to_string_double\n");
    fprintf(gen->output, "    extern rt_to_string_char\n");
    fprintf(gen->output, "    extern rt_to_string_bool\n");
    fprintf(gen->output, "    extern rt_to_string_string\n");
    fprintf(gen->output, "    extern rt_eq_string\n");
    fprintf(gen->output, "    extern rt_ne_string\n");
    fprintf(gen->output, "    extern rt_lt_string\n");
    fprintf(gen->output, "    extern rt_le_string\n");
    fprintf(gen->output, "    extern rt_gt_string\n");
    fprintf(gen->output, "    extern rt_ge_string\n");
    fprintf(gen->output, "    extern free\n\n");
}

static int code_gen_get_var_offset(CodeGen *gen, Token name)
{
    char var_name[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(var_name, name.start, name_len);
    var_name[name_len] = '\0';
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, name);
    if (symbol == NULL)
    {
        return -1;
    }
    return symbol->offset;
}

void code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr)
{
    code_gen_expression(gen, expr->left);
    fprintf(gen->output, "    mov rbx, rax\n");
    code_gen_expression(gen, expr->right);
    fprintf(gen->output, "    mov rcx, rax\n");
    Type *left_type = expr->left->expr_type;
    if (left_type == NULL)
    {
        exit(1);
    }
    bool free_left = (expr->left->type != EXPR_VARIABLE);
    bool free_right = (expr->right->type != EXPR_VARIABLE);
    switch (expr->operator)
    {
    case TOKEN_PLUS:
        if (left_type->kind == TYPE_STRING)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_str_concat\n");
            fprintf(gen->output, "    add rsp, r15\n");
            // Free inputs if they are temps
            if (free_left)
            {
                fprintf(gen->output, "    mov rdi, rbx\n");
                fprintf(gen->output, "    mov r15, rsp\n");
                fprintf(gen->output, "    and r15, 15\n");
                fprintf(gen->output, "    sub rsp, r15\n");
                fprintf(gen->output, "    call free\n");
                fprintf(gen->output, "    add rsp, r15\n");
            }
            if (free_right)
            {
                fprintf(gen->output, "    mov rdi, rcx\n");
                fprintf(gen->output, "    mov r15, rsp\n");
                fprintf(gen->output, "    and r15, 15\n");
                fprintf(gen->output, "    sub rsp, r15\n");
                fprintf(gen->output, "    call free\n");
                fprintf(gen->output, "    add rsp, r15\n");
            }
        }
        else if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_add_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_add_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
            fprintf(gen->output, "    movq rax, xmm0\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_MINUS:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_sub_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_sub_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
            fprintf(gen->output, "    movq rax, xmm0\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_STAR:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_mul_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_mul_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
            fprintf(gen->output, "    movq rax, xmm0\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_SLASH:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_div_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_div_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
            fprintf(gen->output, "    movq rax, xmm0\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_MODULO:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_mod_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_EQUAL_EQUAL:
        if (left_type->kind == TYPE_STRING)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_eq_string\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_eq_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_eq_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_BANG_EQUAL:
        if (left_type->kind == TYPE_STRING)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_ne_string\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_ne_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_ne_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_LESS:
        if (left_type->kind == TYPE_STRING)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_lt_string\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_lt_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_lt_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_LESS_EQUAL:
        if (left_type->kind == TYPE_STRING)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_le_string\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_le_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_le_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_GREATER:
        if (left_type->kind == TYPE_STRING)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_gt_string\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_gt_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_gt_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_GREATER_EQUAL:
        if (left_type->kind == TYPE_STRING)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_ge_string\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov rsi, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_ge_long\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else if (left_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rbx\n");
            fprintf(gen->output, "    movq xmm1, rcx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_ge_double\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
        else
        {
            exit(1);
        }
        break;
    case TOKEN_AND:
    {
        int end_label = code_gen_new_label(gen);
        fprintf(gen->output, "    test rbx, rbx\n");
        fprintf(gen->output, "    jz .L%d\n", end_label);
        fprintf(gen->output, "    test rcx, rcx\n");
        fprintf(gen->output, "    jz .L%d\n", end_label);
        fprintf(gen->output, "    mov rax, 1\n");
        fprintf(gen->output, "    jmp .L%d_end\n", end_label);
        fprintf(gen->output, ".L%d:\n", end_label);
        fprintf(gen->output, "    xor rax, rax\n");
        fprintf(gen->output, ".L%d_end:\n", end_label);
    }
    break;
    case TOKEN_OR:
    {
        int end_label = code_gen_new_label(gen);
        fprintf(gen->output, "    test rbx, rbx\n");
        fprintf(gen->output, "    jnz .L%d\n", end_label);
        fprintf(gen->output, "    test rcx, rcx\n");
        fprintf(gen->output, "    jz .L%d_false\n", end_label);
        fprintf(gen->output, ".L%d:\n", end_label);
        fprintf(gen->output, "    mov rax, 1\n");
        fprintf(gen->output, "    jmp .L%d_end\n", end_label);
        fprintf(gen->output, ".L%d_false:\n", end_label);
        fprintf(gen->output, "    xor rax, rax\n");
        fprintf(gen->output, ".L%d_end:\n", end_label);
    }
    break;
    default:
        exit(1);
    }
}

void code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr)
{
    code_gen_expression(gen, expr->operand);
    Type *op_type = expr->operand->expr_type;
    if (op_type == NULL)
    {
        exit(1);
    }
    switch (expr->operator)
    {
    case TOKEN_MINUS:
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        if (op_type->kind == TYPE_INT || op_type->kind == TYPE_LONG || op_type->kind == TYPE_CHAR || op_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    mov rdi, rax\n");
            fprintf(gen->output, "    call rt_neg_long\n");
        }
        else if (op_type->kind == TYPE_DOUBLE)
        {
            fprintf(gen->output, "    movq xmm0, rax\n");
            fprintf(gen->output, "    call rt_neg_double\n");
            fprintf(gen->output, "    movq rax, xmm0\n");
        }
        else
        {
            exit(1);
        }
        fprintf(gen->output, "    add rsp, r15\n");
        break;
    case TOKEN_BANG:
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        fprintf(gen->output, "    mov rdi, rax\n");
        fprintf(gen->output, "    call rt_not_bool\n");
        fprintf(gen->output, "    add rsp, r15\n");
        break;
    default:
        exit(1);
    }
}

void code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr)
{
    TypeKind kind = expr->type->kind;
    switch (kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        fprintf(gen->output, "    mov rax, %ld\n", expr->value.int_value);
        break;
    case TYPE_DOUBLE:
        fprintf(gen->output, "    mov rax, 0x%lx\n",
                *(int64_t *)&expr->value.double_value);
        break;
    case TYPE_CHAR:
        fprintf(gen->output, "    mov rax, %d\n", expr->value.char_value);
        break;
    case TYPE_STRING:
    {
        if (expr->value.string_value == NULL)
        {
            fprintf(gen->output, "    xor rax, rax\n");
        }
        else
        {
            int label = code_gen_add_string_literal(gen, expr->value.string_value);
            fprintf(gen->output, "    lea rax, [rel str_%d]\n", label);
            fprintf(gen->output, "    mov rdi, rax\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_to_string_string\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
    }
    break;
    case TYPE_BOOL:
        fprintf(gen->output, "    mov rax, %d\n", expr->value.bool_value);
        break;
    case TYPE_NIL:
        fprintf(gen->output, "    xor rax, rax\n");
        break;
    default:
        exit(1);
    }
}

void code_gen_variable_expression(CodeGen *gen, VariableExpr *expr)
{
    char var_name[256];
    int name_len = expr->name.length < 255 ? expr->name.length : 255;
    strncpy(var_name, expr->name.start, name_len);
    var_name[name_len] = '\0';
    int offset = code_gen_get_var_offset(gen, expr->name);
    if (offset == -1)
    {
        exit(1);
    }
    fprintf(gen->output, "    mov rax, [rbp + %d]\n", offset);
}

void code_gen_assign_expression(CodeGen *gen, AssignExpr *expr)
{
    char var_name[256];
    int name_len = expr->name.length < 255 ? expr->name.length : 255;
    strncpy(var_name, expr->name.start, name_len);
    var_name[name_len] = '\0';
    code_gen_expression(gen, expr->value);
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol == NULL)
    {
        exit(1);
    }
    int offset = symbol->offset;
    if (symbol->type && symbol->type->kind == TYPE_STRING)
    {
        int label = code_gen_new_label(gen);
        fprintf(gen->output, "    mov rdi, [rbp + %d]\n", offset);
        fprintf(gen->output, "    test rdi, rdi\n");
        fprintf(gen->output, "    jz .no_free_%d\n", label);
        fprintf(gen->output, "    push rax\n");
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        fprintf(gen->output, "    call free\n");
        fprintf(gen->output, "    add rsp, r15\n");
        fprintf(gen->output, "    pop rax\n");
        fprintf(gen->output, ".no_free_%d:\n", label);
    }
    fprintf(gen->output, "    mov [rbp + %d], rax\n", offset);
}

static const char* get_to_str_func(TypeKind kind, bool* is_double) {
    *is_double = false;
    switch (kind) {
        case TYPE_STRING: return "rt_to_string_string";
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_CHAR:
        case TYPE_BOOL: return "rt_to_string_long";
        case TYPE_DOUBLE:
            *is_double = true;
            return "rt_to_string_double";
        default:
            // Invalid type in interpolation (should be caught in type checker)
            exit(1);
            return NULL;
    }
}

void code_gen_interpolated_expression(CodeGen *gen, InterpolExpr *expr) {
    int count = expr->part_count;
    if (count == 0) {
        // Empty interpolated string -> duplicate empty string (malloc'd)
        fprintf(gen->output, "    lea rdi, [rel empty_str]\n");
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        fprintf(gen->output, "    call rt_to_string_string\n");
        fprintf(gen->output, "    add rsp, r15\n");
        return;
    }

    // Generate code for first part and convert to string
    code_gen_expression(gen, expr->parts[0]);
    Type *pt = expr->parts[0]->expr_type;
    bool is_double;
    const char* to_str_func = get_to_str_func(pt->kind, &is_double);
    fprintf(gen->output, "    mov r15, rsp\n");
    fprintf(gen->output, "    and r15, 15\n");
    fprintf(gen->output, "    sub rsp, r15\n");
    if (is_double) {
        fprintf(gen->output, "    movq xmm0, rax\n");
    } else {
        fprintf(gen->output, "    mov rdi, rax\n");
    }
    fprintf(gen->output, "    call %s\n", to_str_func);
    fprintf(gen->output, "    add rsp, r15\n");
    // rbx = accumulated string (always malloc'd after conversion)
    fprintf(gen->output, "    mov rbx, rax\n");

    // For each subsequent part: convert to string, concat, free intermediates
    for (int i = 1; i < count; i++) {
        code_gen_expression(gen, expr->parts[i]);
        pt = expr->parts[i]->expr_type;
        to_str_func = get_to_str_func(pt->kind, &is_double);
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        if (is_double) {
            fprintf(gen->output, "    movq xmm0, rax\n");
        } else {
            fprintf(gen->output, "    mov rdi, rax\n");
        }
        fprintf(gen->output, "    call %s\n", to_str_func);
        fprintf(gen->output, "    add rsp, r15\n");
        // rcx = part string (malloc'd)
        fprintf(gen->output, "    mov rcx, rax\n");

        // Concat: rdi = accum (rbx), rsi = part (rcx)
        fprintf(gen->output, "    mov rdi, rbx\n");
        fprintf(gen->output, "    mov rsi, rcx\n");
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        fprintf(gen->output, "    call rt_str_concat\n");
        fprintf(gen->output, "    add rsp, r15\n");

        // Free old accum (rbx, always malloc'd)
        fprintf(gen->output, "    mov rdi, rbx\n");
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        fprintf(gen->output, "    call free\n");
        fprintf(gen->output, "    add rsp, r15\n");

        // Free part (rcx, always malloc'd after conversion)
        fprintf(gen->output, "    mov rdi, rcx\n");
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        fprintf(gen->output, "    call free\n");
        fprintf(gen->output, "    add rsp, r15\n");

        // Update accum
        fprintf(gen->output, "    mov rbx, rax\n");
    }

    // Result in rax
    fprintf(gen->output, "    mov rax, rbx\n");
}

static void code_gen_interpolated_print(CodeGen *gen, InterpolExpr *expr) {
    for (int i = 0; i < expr->part_count; i++) {
        Expr *part = expr->parts[i];
        Type *pt = part->expr_type;
        code_gen_expression(gen, part);
        const char *rt_func = NULL;
        bool is_double = false;
        switch (pt->kind) {
        case TYPE_STRING:
            rt_func = "rt_print_string";
            break;
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_CHAR:
        case TYPE_BOOL:
            rt_func = "rt_print_long";
            break;
        case TYPE_DOUBLE:
            rt_func = "rt_print_double";
            is_double = true;
            break;
        default:
            exit(1);
        }
        // Corrected: Free only if string and not a simple variable load (temps are malloc'd)
        bool needs_free = (pt->kind == TYPE_STRING) && (part->type != EXPR_VARIABLE);
        if (needs_free && !is_double) {
            // Preserve rax in rbx for later free (non-double strings)
            fprintf(gen->output, "    mov rbx, rax\n");
        }
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        if (is_double) {
            fprintf(gen->output, "    movq xmm0, rax\n");
        } else {
            fprintf(gen->output, "    mov rdi, rax\n");
        }
        fprintf(gen->output, "    call %s\n", rt_func);
        fprintf(gen->output, "    add rsp, r15\n");
        if (needs_free && !is_double) {
            // Free the temp string
            fprintf(gen->output, "    mov rdi, rbx\n");
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call free\n");
            fprintf(gen->output, "    add rsp, r15\n");
        }
    }
}

void code_gen_call_expression(CodeGen *gen, Expr *expr)
{
    CallExpr *call = &expr->as.call;
    if (call->arg_count > 6)
    {
        exit(1); // Support more args if needed
    }
    const char *function_name = NULL;
    if (call->callee->type == EXPR_VARIABLE)
    {
        VariableExpr *var = &call->callee->as.variable;
        char *name = malloc(var->name.length + 1);
        if (name == NULL)
        {
            exit(1);
        }
        strncpy(name, var->name.start, var->name.length);
        name[var->name.length] = '\0';
        function_name = name;
    }
    else
    {
        exit(1);
    }
    if (strcmp(function_name, "print") == 0 && call->arg_count == 1)
    {
        Expr *arg = call->arguments[0];
        if (arg->type == EXPR_INTERPOLATED)
        {
            code_gen_interpolated_print(gen, &arg->as.interpol);
            free((void *)function_name);
            return;
        }
        else
        {
            code_gen_expression(gen, arg);
            Type *arg_type = arg->expr_type;
            const char *rt_func = NULL;
            bool is_double = false;
            switch (arg_type->kind)
            {
            case TYPE_INT:
            case TYPE_LONG:
                rt_func = "rt_print_long";
                break;
            case TYPE_DOUBLE:
                rt_func = "rt_print_double";
                is_double = true;
                break;
            case TYPE_CHAR:
                rt_func = "rt_print_char";
                break;
            case TYPE_STRING:
                rt_func = "rt_print_string";
                break;
            case TYPE_BOOL:
                rt_func = "rt_print_bool";
                break;
            default:
                exit(1);
            }
            if (rt_func)
            {
                bool needs_free = (arg_type->kind == TYPE_STRING) && (arg->type != EXPR_VARIABLE);
                if (needs_free)
                {
                    fprintf(gen->output, "    mov rbx, rax\n");
                }
                fprintf(gen->output, "    mov r15, rsp\n");
                fprintf(gen->output, "    and r15, 15\n");
                fprintf(gen->output, "    sub rsp, r15\n");
                if (is_double)
                {
                    fprintf(gen->output, "    movq xmm0, rax\n");
                }
                else
                {
                    fprintf(gen->output, "    mov rdi, rax\n");
                }
                fprintf(gen->output, "    call %s\n", rt_func);
                fprintf(gen->output, "    add rsp, r15\n");
                if (needs_free)
                {
                    fprintf(gen->output, "    mov rdi, rbx\n");
                    fprintf(gen->output, "    mov r15, rsp\n");
                    fprintf(gen->output, "    and r15, 15\n");
                    fprintf(gen->output, "    sub rsp, r15\n");
                    fprintf(gen->output, "    call free\n");
                    fprintf(gen->output, "    add rsp, r15\n");
                }
            }
            free((void *)function_name);
            return;
        }
    }
    // Evaluate arguments left to right and push to stack
    for (int i = 0; i < call->arg_count; i++)
    {
        code_gen_expression(gen, call->arguments[i]);
        fprintf(gen->output, "    push rax\n");
    }
    // Pop into registers right to left
    const char *param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    for (int i = call->arg_count - 1; i >= 0; i--)
    {
        fprintf(gen->output, "    pop %s\n", param_regs[i]);
    }
    fprintf(gen->output, "    mov r15, rsp\n");
    fprintf(gen->output, "    and r15, 15\n");
    fprintf(gen->output, "    sub rsp, r15\n");
    fprintf(gen->output, "    call %s\n", function_name);
    fprintf(gen->output, "    add rsp, r15\n");
    free((void *)function_name);
}

void code_gen_array_expression(CodeGen *gen, ArrayExpr *expr)
{
    (void)gen;
    (void)expr;
    fprintf(gen->output, "    xor rax, rax\n");
}

void code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr)
{
    (void)gen;
    (void)expr;
    fprintf(gen->output, "    xor rax, rax\n");
}

void code_gen_increment_expression(CodeGen *gen, Expr *expr)
{
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    Token name = expr->as.operand->as.variable.name;
    int offset = code_gen_get_var_offset(gen, name);
    if (offset == -1)
    {
        exit(1);
    }
    fprintf(gen->output, "    mov r15, rsp\n");
    fprintf(gen->output, "    and r15, 15\n");
    fprintf(gen->output, "    sub rsp, r15\n");
    fprintf(gen->output, "    lea rdi, [rbp + %d]\n", offset);
    fprintf(gen->output, "    call rt_post_inc_long\n");
    fprintf(gen->output, "    add rsp, r15\n");
}

void code_gen_decrement_expression(CodeGen *gen, Expr *expr)
{
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        exit(1);
    }
    Token name = expr->as.operand->as.variable.name;
    int offset = code_gen_get_var_offset(gen, name);
    if (offset == -1)
    {
        exit(1);
    }
    fprintf(gen->output, "    mov r15, rsp\n");
    fprintf(gen->output, "    and r15, 15\n");
    fprintf(gen->output, "    sub rsp, r15\n");
    fprintf(gen->output, "    lea rdi, [rbp + %d]\n", offset);
    fprintf(gen->output, "    call rt_post_dec_long\n");
    fprintf(gen->output, "    add rsp, r15\n");
}

void code_gen_expression(CodeGen *gen, Expr *expr)
{
    if (expr == NULL)
    {
        fprintf(gen->output, "    xor rax, rax\n");
        return;
    }
    code_gen_push_function_context(gen);
    switch (expr->type)
    {
    case EXPR_BINARY:
        code_gen_binary_expression(gen, &expr->as.binary);
        if (expr->expr_type && expr->expr_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    test rax, rax\n");
            fprintf(gen->output, "    setne al\n");
            fprintf(gen->output, "    movzx rax, al\n");
        }
        break;
    case EXPR_UNARY:
        code_gen_unary_expression(gen, &expr->as.unary);
        break;
    case EXPR_LITERAL:
        code_gen_literal_expression(gen, &expr->as.literal);
        break;
    case EXPR_VARIABLE:
        code_gen_variable_expression(gen, &expr->as.variable);
        break;
    case EXPR_ASSIGN:
        code_gen_assign_expression(gen, &expr->as.assign);
        break;
    case EXPR_CALL:
        code_gen_call_expression(gen, expr);
        break;
    case EXPR_ARRAY:
        code_gen_array_expression(gen, &expr->as.array);
        break;
    case EXPR_ARRAY_ACCESS:
        code_gen_array_access_expression(gen, &expr->as.array_access);
        break;
    case EXPR_INCREMENT:
        code_gen_increment_expression(gen, expr);
        break;
    case EXPR_DECREMENT:
        code_gen_decrement_expression(gen, expr);
        break;
    case EXPR_INTERPOLATED:
        code_gen_interpolated_expression(gen, &expr->as.interpol);
        break;
    }
    code_gen_pop_function_context(gen);
}

void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt)
{
    code_gen_expression(gen, stmt->expression);
}

void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt) {
    symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL);
    if (stmt->initializer != NULL) {
        code_gen_expression(gen, stmt->initializer);
    } else {
        fprintf(gen->output, "    xor rax, rax\n");
    }
    int offset = code_gen_get_var_offset(gen, stmt->name);
    fprintf(gen->output, "    mov [rbp + %d], rax\n", offset);
}

void code_gen_block(CodeGen *gen, BlockStmt *stmt) {
    symbol_table_push_scope(gen->symbol_table);
    for (int i = 0; i < stmt->count; i++) {
        code_gen_statement(gen, stmt->statements[i]);
    }
    Scope *scope = gen->symbol_table->current;
    Symbol *sym = scope->symbols;
    int label_base = code_gen_new_label(gen);
    int free_idx = 0;
    while (sym) {
        if (sym->type && sym->type->kind == TYPE_STRING && sym->kind == SYMBOL_LOCAL) {
            int label = label_base + free_idx++;
            fprintf(gen->output, "    mov rdi, [rbp + %d]\n", sym->offset);
            fprintf(gen->output, "    test rdi, rdi\n");
            fprintf(gen->output, "    jz .no_free_%d\n", label);
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call free\n");
            fprintf(gen->output, "    add rsp, r15\n");
            fprintf(gen->output, ".no_free_%d:\n", label);
        }
        sym = sym->next;
    }
    symbol_table_pop_scope(gen->symbol_table);
}

static void pre_compute_stack_usage(CodeGen *gen, Stmt *stmt) {
    if (stmt == NULL) return;

    switch (stmt->type) {
    case STMT_VAR_DECL: {
        int type_size = get_type_size(stmt->as.var_decl.type);
        int aligned_size = ((type_size + OFFSET_ALIGNMENT - 1) / OFFSET_ALIGNMENT) * OFFSET_ALIGNMENT;
        gen->symbol_table->current->next_local_offset += aligned_size;
        break;
    }
    case STMT_BLOCK: {
        int save = gen->symbol_table->current->next_local_offset;
        for (int i = 0; i < stmt->as.block.count; i++) {
            pre_compute_stack_usage(gen, stmt->as.block.statements[i]);
        }
        gen->symbol_table->current->next_local_offset = MAX(save, gen->symbol_table->current->next_local_offset);
        break;
    }
    case STMT_IF: {
        int save = gen->symbol_table->current->next_local_offset;
        pre_compute_stack_usage(gen, stmt->as.if_stmt.then_branch);
        int then_max = gen->symbol_table->current->next_local_offset;
        gen->symbol_table->current->next_local_offset = save;
        if (stmt->as.if_stmt.else_branch) {
            pre_compute_stack_usage(gen, stmt->as.if_stmt.else_branch);
        }
        int else_max = gen->symbol_table->current->next_local_offset;
        gen->symbol_table->current->next_local_offset = MAX(save, MAX(then_max, else_max));
        break;
    }
    case STMT_WHILE: {
        pre_compute_stack_usage(gen, stmt->as.while_stmt.body);
        break;
    }
    case STMT_FOR: {
        int save = gen->symbol_table->current->next_local_offset;
        if (stmt->as.for_stmt.initializer) {
            pre_compute_stack_usage(gen, stmt->as.for_stmt.initializer);
        }
        pre_compute_stack_usage(gen, stmt->as.for_stmt.body);
        gen->symbol_table->current->next_local_offset = MAX(save, gen->symbol_table->current->next_local_offset);
        break;
    }
    default:
        break;
    }
}

void code_gen_function(CodeGen *gen, FunctionStmt *stmt) {
    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;
    if (stmt->name.length < 256) {
        gen->current_function = malloc(stmt->name.length + 1);
        if (gen->current_function == NULL) {
            exit(1);
        }
        strncpy(gen->current_function, stmt->name.start, stmt->name.length);
        gen->current_function[stmt->name.length] = '\0';
    } else {
        exit(1);
    }
    gen->current_return_type = stmt->return_type;
    symbol_table_push_scope(gen->symbol_table);
    for (int i = 0; i < stmt->param_count; i++) {
        symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->params[i].name, stmt->params[i].type, SYMBOL_PARAM);
    }
    for (int i = 0; i < stmt->body_count; i++) {
        pre_compute_stack_usage(gen, stmt->body[i]);
    }
    int locals_size = gen->symbol_table->current->next_local_offset - LOCAL_BASE_OFFSET;
    // Reset next_local_offset to the base value after pre-computation to avoid double-counting sizes during actual code generation.
    gen->symbol_table->current->next_local_offset = LOCAL_BASE_OFFSET;
    int saved_size = CALLEE_SAVED_SPACE;
    int stack_space = locals_size + saved_size;
    stack_space = ((stack_space + 15) / 16) * 16;
    if (stack_space < 128) stack_space = 128;
    fprintf(gen->output, "%s:\n", gen->current_function);
    fprintf(gen->output, "    push rbp\n");
    fprintf(gen->output, "    mov rbp, rsp\n");
    fprintf(gen->output, "    sub rsp, %d\n", stack_space);
    fprintf(gen->output, "    mov [rbp - 8], rbx\n");
    fprintf(gen->output, "    mov [rbp - 16], r15\n");
    const char *param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    for (int i = 0; i < stmt->param_count && i < 6; i++) {
        int offset = symbol_table_get_symbol_offset(gen->symbol_table, stmt->params[i].name);
        fprintf(gen->output, "    mov [rbp + %d], %s\n", offset, param_regs[i]);
    }
    for (int i = 0; i < stmt->body_count; i++) {
        code_gen_statement(gen, stmt->body[i]);
    }
    if (strcmp(gen->current_function, "main") == 0) {
        fprintf(gen->output, "main_return:\n");
    } else {
        fprintf(gen->output, "%s_return:\n", gen->current_function);
    }
    Scope *scope = gen->symbol_table->current;
    Symbol *sym = scope->symbols;
    int label_base = code_gen_new_label(gen);
    int free_idx = 0;
    while (sym) {
        if (sym->type && sym->type->kind == TYPE_STRING && sym->kind == SYMBOL_LOCAL) {
            int label = label_base + free_idx++;
            fprintf(gen->output, "    mov rdi, [rbp + %d]\n", sym->offset);
            fprintf(gen->output, "    test rdi, rdi\n");
            fprintf(gen->output, "    jz .no_free_%d\n", label);
            if (gen->current_return_type && gen->current_return_type->kind == TYPE_STRING) {
                fprintf(gen->output, "    cmp rdi, rax\n");
                fprintf(gen->output, "    je .no_free_%d\n", label);
            }
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call free\n");
            fprintf(gen->output, "    add rsp, r15\n");
            fprintf(gen->output, ".no_free_%d:\n", label);
        }
        sym = sym->next;
    }
    fprintf(gen->output, "    mov rbx, [rbp - 8]\n");
    fprintf(gen->output, "    mov r15, [rbp - 16]\n");
    fprintf(gen->output, "    mov rsp, rbp\n");
    fprintf(gen->output, "    pop rbp\n");
    fprintf(gen->output, "    ret\n");
    symbol_table_pop_scope(gen->symbol_table);
    free(gen->current_function);
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;
}

void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt)
{
    if (stmt->value != NULL)
    {
        code_gen_expression(gen, stmt->value);
    }
    else
    {
        fprintf(gen->output, "    xor rax, rax\n");
    }
    if (gen->current_function && strcmp(gen->current_function, "main") == 0)
    {
        fprintf(gen->output, "    jmp main_return\n");
    }
    else
    {
        fprintf(gen->output, "    jmp %s_return\n", gen->current_function);
    }
}

void code_gen_if_statement(CodeGen *gen, IfStmt *stmt)
{
    int else_label = code_gen_new_label(gen);
    int end_label = code_gen_new_label(gen);
    code_gen_expression(gen, stmt->condition);
    fprintf(gen->output, "    test rax, rax\n");
    fprintf(gen->output, "    jz .L%d\n", else_label);
    code_gen_statement(gen, stmt->then_branch);
    if (stmt->else_branch != NULL)
    {
        fprintf(gen->output, "    jmp .L%d\n", end_label);
    }
    fprintf(gen->output, ".L%d:\n", else_label);
    if (stmt->else_branch != NULL)
    {
        code_gen_statement(gen, stmt->else_branch);
    }
    if (stmt->else_branch != NULL)
    {
        fprintf(gen->output, ".L%d:\n", end_label);
    }
}

void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt)
{
    int loop_start = code_gen_new_label(gen);
    int loop_end = code_gen_new_label(gen);
    fprintf(gen->output, ".L%d:\n", loop_start);
    code_gen_expression(gen, stmt->condition);
    fprintf(gen->output, "    test rax, rax\n");
    fprintf(gen->output, "    jz .L%d\n", loop_end);
    if (stmt->body->type == STMT_BLOCK)
    {
        code_gen_block(gen, &stmt->body->as.block);
    }
    else
    {
        code_gen_statement(gen, stmt->body);
    }
    fprintf(gen->output, "    jmp .L%d\n", loop_start);
    fprintf(gen->output, ".L%d:\n", loop_end);
}

void code_gen_for_statement(CodeGen *gen, ForStmt *stmt) {
    int loop_start = code_gen_new_label(gen);
    int loop_end = code_gen_new_label(gen);
    symbol_table_push_scope(gen->symbol_table);
    if (stmt->initializer != NULL) {
        code_gen_statement(gen, stmt->initializer);
    }
    fprintf(gen->output, ".L%d:\n", loop_start);
    if (stmt->condition != NULL) {
        code_gen_expression(gen, stmt->condition);
        fprintf(gen->output, "    test rax, rax\n");
        fprintf(gen->output, "    jz .L%d\n", loop_end);
    }
    code_gen_statement(gen, stmt->body);
    if (stmt->increment != NULL) {
        code_gen_expression(gen, stmt->increment);
    }
    fprintf(gen->output, "    jmp .L%d\n", loop_start);
    fprintf(gen->output, ".L%d:\n", loop_end);
    Scope *scope = gen->symbol_table->current;
    Symbol *sym = scope->symbols;
    int label_base = code_gen_new_label(gen);
    int free_idx = 0;
    while (sym) {
        if (sym->type && sym->type->kind == TYPE_STRING && sym->kind == SYMBOL_LOCAL) {
            int label = label_base + free_idx++;
            fprintf(gen->output, "    mov rdi, [rbp + %d]\n", sym->offset);
            fprintf(gen->output, "    test rdi, rdi\n");
            fprintf(gen->output, "    jz .no_free_%d\n", label);
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call free\n");
            fprintf(gen->output, "    add rsp, r15\n");
            fprintf(gen->output, ".no_free_%d:\n", label);
        }
        sym = sym->next;
    }
    symbol_table_pop_scope(gen->symbol_table);
}

void code_gen_statement(CodeGen *gen, Stmt *stmt)
{
    switch (stmt->type)
    {
    case STMT_EXPR:
        code_gen_expression_statement(gen, &stmt->as.expression);
        break;
    case STMT_VAR_DECL:
        code_gen_var_declaration(gen, &stmt->as.var_decl);
        break;
    case STMT_FUNCTION:
        code_gen_function(gen, &stmt->as.function);
        break;
    case STMT_RETURN:
        code_gen_return_statement(gen, &stmt->as.return_stmt);
        break;
    case STMT_BLOCK:
        code_gen_block(gen, &stmt->as.block);
        break;
    case STMT_IF:
        code_gen_if_statement(gen, &stmt->as.if_stmt);
        break;
    case STMT_WHILE:
        code_gen_while_statement(gen, &stmt->as.while_stmt);
        break;
    case STMT_FOR:
        code_gen_for_statement(gen, &stmt->as.for_stmt);
        break;
    case STMT_IMPORT:
        break;
    }
}

void code_gen_footer(CodeGen *gen)
{
    fprintf(gen->output, "\nsection .note.GNU-stack noalloc noexec nowrite progbits\n");
}

void code_gen_module(CodeGen *gen, Module *module)
{
    code_gen_text_section(gen);
    for (int i = 0; i < module->count; i++)
    {
        code_gen_statement(gen, module->statements[i]);
    }
    code_gen_data_section(gen);
    code_gen_footer(gen);
}

static void pre_build_symbols(CodeGen *gen, Stmt *stmt)
{
    if (stmt == NULL)
        return;
    switch (stmt->type)
    {
    case STMT_VAR_DECL:
        symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->as.var_decl.name, stmt->as.var_decl.type, SYMBOL_LOCAL);
        break;
    case STMT_BLOCK:
        symbol_table_push_scope(gen->symbol_table);
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            pre_build_symbols(gen, stmt->as.block.statements[i]);
        }
        symbol_table_pop_scope(gen->symbol_table);
        break;
    case STMT_IF:
        pre_build_symbols(gen, stmt->as.if_stmt.then_branch);
        if (stmt->as.if_stmt.else_branch)
            pre_build_symbols(gen, stmt->as.if_stmt.else_branch);
        break;
    case STMT_WHILE:
        pre_build_symbols(gen, stmt->as.while_stmt.body);
        break;
    case STMT_FOR:
        symbol_table_push_scope(gen->symbol_table);
        if (stmt->as.for_stmt.initializer)
            pre_build_symbols(gen, stmt->as.for_stmt.initializer);
        pre_build_symbols(gen, stmt->as.for_stmt.body);
        symbol_table_pop_scope(gen->symbol_table);
        break;
    case STMT_EXPR:
    case STMT_RETURN:
    case STMT_FUNCTION:
    case STMT_IMPORT:
        break;
    }
}