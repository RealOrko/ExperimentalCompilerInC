/**
 * code_gen.c
 * Implementation of the code generator with fixes for string handling
 */

#include "code_gen.h"
#include "debug.h"
#include "parser.h"       // For sub-parsing in interpolation
#include "type_checker.h" // For type_check_expr
#include <stdlib.h>
#include <string.h>

static void pre_build_symbols(CodeGen *gen, Stmt *stmt);

static char *my_strndup(const char *s, size_t n)
{
    if (s == NULL)
        return NULL;
    size_t len = strlen(s);
    if (len < n)
        n = len;
    char *new_str = malloc(n + 1);
    if (new_str == NULL)
        return NULL;
    memcpy(new_str, s, n);
    new_str[n] = '\0';
    return new_str;
}

// Custom strdup implementation since it's not part of C99 standard
static char *my_strdup(const char *s)
{
    if (s == NULL)
        return NULL;
    size_t len = strlen(s) + 1;
    char *new_str = malloc(len);
    if (new_str == NULL)
        return NULL;
    return memcpy(new_str, s, len);
}

// Initialize the function context stack
void code_gen_init_function_stack(CodeGen *gen)
{
    gen->function_stack_capacity = 8;
    gen->function_stack_size = 0;
    gen->function_stack = malloc(gen->function_stack_capacity * sizeof(char *));
    if (gen->function_stack == NULL)
    {
        DEBUG_ERROR("Out of memory initializing function stack");
        exit(1);
    }
}

void code_gen_push_function_context(CodeGen *gen)
{
    // Resize if needed
    if (gen->function_stack_size >= gen->function_stack_capacity)
    {
        gen->function_stack_capacity *= 2;
        gen->function_stack = realloc(gen->function_stack,
                                      gen->function_stack_capacity * sizeof(char *));
        if (gen->function_stack == NULL)
        {
            DEBUG_ERROR("Out of memory expanding function stack");
            exit(1);
        }
    }

    // Store current function (even if NULL)
    if (gen->current_function != NULL)
    {
        gen->function_stack[gen->function_stack_size] = my_strdup(gen->current_function);
        if (gen->function_stack[gen->function_stack_size] == NULL)
        {
            DEBUG_ERROR("Failed to duplicate function name");
            exit(1);
        }
    }
    else
    {
        gen->function_stack[gen->function_stack_size] = NULL;
    }
    gen->function_stack_size++;

    DEBUG_VERBOSE("Pushed function context '%s', stack size now %d",
                  gen->current_function ? gen->current_function : "NULL", gen->function_stack_size);
}

void code_gen_pop_function_context(CodeGen *gen)
{
    if (gen->function_stack_size <= 0)
    {
        DEBUG_WARNING("Attempt to pop empty function stack");
        return;
    }

    // Free current function name if set
    if (gen->current_function != NULL)
    {
        free(gen->current_function);
    }

    // Pop and restore
    gen->function_stack_size--;
    gen->current_function = gen->function_stack[gen->function_stack_size];
    // Remove the popped entry to avoid double-free
    gen->function_stack[gen->function_stack_size] = NULL;

    DEBUG_VERBOSE("Popped function context, restored to '%s', stack size now %d",
                  gen->current_function ? gen->current_function : "NULL", gen->function_stack_size);
}

void code_gen_free_function_stack(CodeGen *gen)
{
    if (gen->function_stack == NULL)
    {
        return;
    }

    // Free all stacked function names
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

    // Initialize function context stack
    code_gen_init_function_stack(gen);

    if (gen->output == NULL)
    {
        DEBUG_ERROR("Could not open output file '%s'", output_file);
        exit(1);
    }

    // Initialize string literals table
    string_literals = NULL;
}

void code_gen_cleanup(CodeGen *gen)
{
    if (gen->output != NULL)
    {
        fclose(gen->output);
    }

    // Free current function name if set
    if (gen->current_function != NULL)
    {
        free(gen->current_function);
        gen->current_function = NULL;
    }

    // Free function context stack
    code_gen_free_function_stack(gen);

    // Free string literals
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
        DEBUG_ERROR("Null string passed to add_string_literal");
        return -1;
    }

    int label = code_gen_new_label(gen);

    StringLiteral *literal = malloc(sizeof(StringLiteral));
    if (literal == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }

    literal->string = my_strdup(string);
    if (literal->string == NULL)
    {
        DEBUG_ERROR("Out of memory");
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
            break; // Newline
        case '\t':
            fprintf(gen->output, "9");
            break; // Tab
        case '\\':
            fprintf(gen->output, "92");
            break; // Backslash
        default:
            fprintf(gen->output, "'%c'", *p);
        }
        p++;
    }

    fprintf(gen->output, ", 0\n"); // Null terminator
}

void code_gen_data_section(CodeGen *gen)
{
    fprintf(gen->output, "section .data\n");

    // Generate string literals
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

    // Existing externs
    fprintf(gen->output, "    extern rt_str_concat\n");
    fprintf(gen->output, "    extern rt_print_long\n");
    fprintf(gen->output, "    extern rt_print_double\n");
    fprintf(gen->output, "    extern rt_print_char\n");
    fprintf(gen->output, "    extern rt_print_string\n");
    fprintf(gen->output, "    extern rt_print_bool\n");

    // New externs for ops
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
    fprintf(gen->output, "    extern rt_post_dec_long\n\n");
}

void code_gen_prologue(CodeGen *gen, const char *function_name)
{
    fprintf(gen->output, "%s:\n", function_name);
    fprintf(gen->output, "    push rbp\n");
    fprintf(gen->output, "    mov rbp, rsp\n");

    // Dynamically calculate stack space needed
    int stack_space = 72; // Base allocation (adjusted for alignment)
    if (strcmp(function_name, "main") == 0)
        stack_space = 136; // More space for main function (adjusted)
    fprintf(gen->output, "    sub rsp, %d\n", stack_space);
}

void code_gen_epilogue(CodeGen *gen)
{
    // This function is now a no-op
    (void)gen; // Avoid unused parameter warning
}

static int code_gen_get_var_offset(CodeGen *gen, Token name)
{
    // Extract variable name for comparison
    char var_name[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(var_name, name.start, name_len);
    var_name[name_len] = '\0';

    // Debug output to see what variable we're trying to access
    DEBUG_VERBOSE("Looking up variable '%s' in function '%s'",
                  var_name, gen->current_function ? gen->current_function : "global");

    // Use the symbol table to get the variable's offset
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, name);

    if (symbol == NULL)
    {
        // For debugging, print the variable we're looking for
        char temp[256];
        int name_len = name.length < 255 ? name.length : 255;
        strncpy(temp, name.start, name_len);
        temp[name_len] = '\0';
        DEBUG_ERROR("Symbol not found in get_symbol_offset: '%s'", temp);
        return -1;
    }

    DEBUG_VERBOSE("Found symbol '%s' with offset %d", var_name, symbol->offset);
    return symbol->offset;
}

void code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr)
{
    DEBUG_VERBOSE("Generating binary expression in function '%s' with operator %d",
                  gen->current_function ? gen->current_function : "global", expr->operator);

    // Generate left operand (result in rax)
    code_gen_expression(gen, expr->left);
    // Save left ptr in rbx (callee-saved, but we manage)
    fprintf(gen->output, "    mov rbx, rax\n");

    // Generate right operand (result in rax)
    code_gen_expression(gen, expr->right);
    // Save right ptr in rcx
    fprintf(gen->output, "    mov rcx, rax\n");

    Type *left_type = expr->left->expr_type;
    if (left_type == NULL)
    {
        DEBUG_ERROR("No type");
        exit(1);
    } // Add check

    switch (expr->operator)
    {
    case TOKEN_PLUS:
        if (left_type->kind == TYPE_STRING)
        {
            fprintf(gen->output, "    mov rdi, rbx\n"); // left ptr as first arg
            fprintf(gen->output, "    mov rsi, rcx\n"); // right ptr as second arg
            fprintf(gen->output, "    mov r15, rsp\n");
            fprintf(gen->output, "    and r15, 15\n");
            fprintf(gen->output, "    sub rsp, r15\n");
            fprintf(gen->output, "    call rt_str_concat\n");
            fprintf(gen->output, "    add rsp, r15\n");
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
            DEBUG_ERROR("Unsupported + type");
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
            DEBUG_ERROR("Unsupported - type");
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
            DEBUG_ERROR("Unsupported * type");
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
            DEBUG_ERROR("Unsupported / type");
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
            DEBUG_ERROR("Unsupported % type");
            exit(1);
        }
        break;
    case TOKEN_EQUAL_EQUAL:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL || left_type->kind == TYPE_STRING)
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
            DEBUG_ERROR("Unsupported == type");
            exit(1);
        }
        break;
    case TOKEN_BANG_EQUAL:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL || left_type->kind == TYPE_STRING)
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
            DEBUG_ERROR("Unsupported != type");
            exit(1);
        }
        break;
    case TOKEN_LESS:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
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
            DEBUG_ERROR("Unsupported < type");
            exit(1);
        }
        break;
    case TOKEN_LESS_EQUAL:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
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
            DEBUG_ERROR("Unsupported <= type");
            exit(1);
        }
        break;
    case TOKEN_GREATER:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
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
            DEBUG_ERROR("Unsupported > type");
            exit(1);
        }
        break;
    case TOKEN_GREATER_EQUAL:
        if (left_type->kind == TYPE_INT || left_type->kind == TYPE_LONG || left_type->kind == TYPE_CHAR || left_type->kind == TYPE_BOOL)
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
            DEBUG_ERROR("Unsupported >= type");
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
        DEBUG_ERROR("Unsupported binary operator\n");
        exit(1);
    }
}

void code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr)
{
    // Generate the operand
    code_gen_expression(gen, expr->operand);

    Type *op_type = expr->operand->expr_type;
    if (op_type == NULL)
    {
        DEBUG_ERROR("No type");
        exit(1);
    } // Add check

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
            DEBUG_ERROR("Unsupported - type");
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
        DEBUG_ERROR("Unsupported unary operator\n");
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
        // In x86-64, floating point values are typically in xmm0
        // For simplicity, we'll handle doubles as integers
        fprintf(gen->output, "    ; Double literals not fully implemented\n");
        fprintf(gen->output, "    mov rax, 0x%lx\n",
                *(int64_t *)&expr->value.double_value);
        break;
    case TYPE_CHAR:
        fprintf(gen->output, "    mov rax, %d\n", expr->value.char_value);
        break;
    case TYPE_STRING:
    {
        // Check if string value is NULL
        if (expr->value.string_value == NULL)
        {
            fprintf(gen->output, "    ; Warning: NULL string literal\n");
            fprintf(gen->output, "    xor rax, rax\n");
        }
        else
        {
            // Add the string to the string table and load its address
            int label = code_gen_add_string_literal(gen, expr->value.string_value);
            fprintf(gen->output, "    lea rax, [rel str_%d]\n", label);
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
        DEBUG_ERROR("Unsupported literal type\n");
        exit(1);
    }
}

void code_gen_variable_expression(CodeGen *gen, VariableExpr *expr)
{
    // Extract name for debugging
    char var_name[256];
    int name_len = expr->name.length < 255 ? expr->name.length : 255;
    strncpy(var_name, expr->name.start, name_len);
    var_name[name_len] = '\0';

    DEBUG_VERBOSE("Accessing variable '%s' in function '%s'",
                  var_name, gen->current_function ? gen->current_function : "global");

    int offset = code_gen_get_var_offset(gen, expr->name);
    if (offset == -1)
    {
        DEBUG_ERROR("Undefined variable '%s'", var_name);
        exit(1);
    }

    fprintf(gen->output, "    mov rax, [rbp + %d]\n", offset);
}

void code_gen_assign_expression(CodeGen *gen, AssignExpr *expr)
{
    // Extract name for debugging
    char var_name[256];
    int name_len = expr->name.length < 255 ? expr->name.length : 255;
    strncpy(var_name, expr->name.start, name_len);
    var_name[name_len] = '\0';

    DEBUG_VERBOSE("Assigning to variable '%s' in function '%s'",
                  var_name, gen->current_function ? gen->current_function : "global");

    // Evaluate the value
    code_gen_expression(gen, expr->value);

    int offset = code_gen_get_var_offset(gen, expr->name);
    if (offset == -1)
    {
        DEBUG_ERROR("Undefined variable '%s'", var_name);
        exit(1);
    }

    fprintf(gen->output, "    mov [rbp + %d], rax\n", offset);
}

static void code_gen_interpolated_print(CodeGen *gen, const char *content)
{
    const char *p = content;
    const char *start = p;

    while (*p != '\0')
    {
        if (*p == '{')
        {
            // Generate print for previous string part
            if (p > start)
            {
                int len = p - start;
                char *part_str = my_strndup(start, len);
                int part_label = code_gen_add_string_literal(gen, part_str);
                free(part_str);
                fprintf(gen->output, "    mov r15, rsp\n");
                fprintf(gen->output, "    and r15, 15\n");
                fprintf(gen->output, "    sub rsp, r15\n");
                fprintf(gen->output, "    lea rdi, [rel str_%d]\n", part_label);
                fprintf(gen->output, "    call rt_print_string\n");
                fprintf(gen->output, "    add rsp, r15\n");
            }

            // Parse the expression inside { }
            p++;
            const char *expr_begin = p;
            while (*p != '}' && *p != '\0')
                p++;
            if (*p == '\0')
            {
                DEBUG_ERROR("Unterminated { in interpolated string");
                return;
            }
            int expr_length = p - expr_begin;
            char *expr_source = my_strndup(expr_begin, expr_length);

            // Set up temporary lexer and parser for the expression
            Lexer temp_lexer;
            lexer_init(&temp_lexer, expr_source, "interpolated expression");
            temp_lexer.line = 0; // Placeholder line number

            Parser temp_parser;
            parser_init(&temp_parser, &temp_lexer);
            temp_parser.had_error = 0;
            temp_parser.panic_mode = 0;

            Expr *inner_expr = parser_expression(&temp_parser);
            if (inner_expr == NULL || temp_parser.had_error)
            {
                DEBUG_ERROR("Error parsing interpolated expression");
                ast_free_expr(inner_expr);
                free(expr_source);
                parser_cleanup(&temp_parser);
                return;
            }

            Type *inner_type = type_check_expr(inner_expr, gen->symbol_table);
            if (inner_type == NULL)
            {
                DEBUG_ERROR("Type check failed for interpolated expression");
                ast_free_expr(inner_expr);
                free(expr_source);
                parser_cleanup(&temp_parser);
                return;
            }

            // Generate code for the expression
            code_gen_expression(gen, inner_expr);

            // Generate runtime print call based on type (no \n)
            bool is_double = false;
            const char *rt_func = NULL;
            switch (inner_type->kind)
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
                DEBUG_ERROR("Unsupported type in interpolation");
                ast_free_expr(inner_expr);
                free(expr_source);
                parser_cleanup(&temp_parser);
                return;
            }
            if (rt_func)
            {
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
            }

            // Cleanup
            ast_free_expr(inner_expr);
            free(expr_source);
            parser_cleanup(&temp_parser);

            p++; // Skip '}'
            start = p;
        }
        else
        {
            p++;
        }
    }

    // Last string part
    if (p > start)
    {
        int len = p - start;
        char *part_str = my_strndup(start, len);
        int part_label = code_gen_add_string_literal(gen, part_str);
        free(part_str);
        fprintf(gen->output, "    mov r15, rsp\n");
        fprintf(gen->output, "    and r15, 15\n");
        fprintf(gen->output, "    sub rsp, r15\n");
        fprintf(gen->output, "    lea rdi, [rel str_%d]\n", part_label);
        fprintf(gen->output, "    call rt_print_string\n");
        fprintf(gen->output, "    add rsp, r15\n");
    }
}

void code_gen_call_expression(CodeGen *gen, Expr *expr)
{
    CallExpr *call = &expr->as.call;
    // Validate the number of arguments
    if (call->arg_count > 6)
    {
        DEBUG_ERROR("Too many arguments (max 6 supported)");
        return;
    }

    // Calculate function name
    const char *function_name = NULL;

    // Handle variable functions
    if (call->callee->type == EXPR_VARIABLE)
    {
        VariableExpr *var = &call->callee->as.variable;
        // Copy function name
        char *name = malloc(var->name.length + 1);
        if (name == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }

        strncpy(name, var->name.start, var->name.length);
        name[var->name.length] = '\0';
        function_name = name;
    }
    else
    {
        DEBUG_ERROR("Complex function calls not supported");
        exit(1);
    }

    fprintf(gen->output, "    ; Call to %s with %d arguments\n", function_name, call->arg_count);

    if (strcmp(function_name, "print") == 0 && call->arg_count == 1)
    {
        Expr *arg = call->arguments[0];
        if (arg->type == EXPR_LITERAL && arg->as.literal.type->kind == TYPE_STRING && arg->as.literal.is_interpolated)
        {
            code_gen_interpolated_print(gen, arg->as.literal.value.string_value);
            free((void *)function_name);
            return;
        }
        else
        {
            // Normal print: evaluate arg and call runtime print based on type
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
                DEBUG_ERROR("Unsupported type for print");
                break;
            }
            if (rt_func)
            {
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
            }
            free((void *)function_name);
            return;
        }
    }

    // Normal function call (existing code for evaluating args and call)
    // Evaluate arguments and move to registers in reverse order (x86-64 ABI)
    for (int i = call->arg_count - 1; i >= 0; i--)
    {
        fprintf(gen->output, "    ; Evaluating argument %d\n", i);
        code_gen_expression(gen, call->arguments[i]);

        // Use appropriate registers for the first 6 arguments
        switch (i)
        {
        case 0:
            fprintf(gen->output, "    mov rdi, rax\n");
            break;
        case 1:
            fprintf(gen->output, "    mov rsi, rax\n");
            break;
        case 2:
            fprintf(gen->output, "    mov rdx, rax\n");
            break;
        case 3:
            fprintf(gen->output, "    mov rcx, rax\n");
            break;
        case 4:
            fprintf(gen->output, "    mov r8, rax\n");
            break;
        case 5:
            fprintf(gen->output, "    mov r9, rax\n");
            break;
        default:
            // For >6 args (not supported yet), would push here
            break;
        }
    }

    // Ensure stack is 16-byte aligned before call (ABI requirement)
    fprintf(gen->output, "    ; Aligning stack for function call\n");
    fprintf(gen->output, "    mov r15, rsp\n");
    fprintf(gen->output, "    and r15, 15\n");
    fprintf(gen->output, "    sub rsp, r15\n");

    // Call the function
    fprintf(gen->output, "    call %s\n", function_name);

    // Restore stack alignment
    fprintf(gen->output, "    add rsp, r15\n");

    free((void *)function_name);
}

void code_gen_array_expression(CodeGen *gen, ArrayExpr *expr)
{
    // Arrays not implemented yet
    (void)expr; // Suppress unused parameter warning
    fprintf(gen->output, "    ; Arrays not fully implemented\n");
    fprintf(gen->output, "    xor rax, rax\n");
}

void code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr)
{
    // Array access not implemented yet
    (void)expr; // Suppress unused parameter warning
    fprintf(gen->output, "    ; Array access not fully implemented\n");
    fprintf(gen->output, "    xor rax, rax\n");
}

void code_gen_increment_expression(CodeGen *gen, Expr *expr)
{
    if (expr->as.operand->type != EXPR_VARIABLE)
    {
        fprintf(gen->output, "    ; Increment on non-variable not implemented\n");
        return;
    }

    Token name = expr->as.operand->as.variable.name;
    int offset = code_gen_get_var_offset(gen, name);
    if (offset == -1)
    {
        fprintf(gen->output, "    ; Invalid variable for increment\n");
        return;
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
        fprintf(gen->output, "    ; Decrement on non-variable not implemented\n");
        return;
    }

    Token name = expr->as.operand->as.variable.name;
    int offset = code_gen_get_var_offset(gen, name);
    if (offset == -1)
    {
        fprintf(gen->output, "    ; Invalid variable for decrement\n");
        return;
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

    // Save the current function context
    code_gen_push_function_context(gen);

    DEBUG_VERBOSE("Generating expression type %d in function context '%s'",
                  expr->type, gen->current_function ? gen->current_function : "global");

    switch (expr->type)
    {
    case EXPR_BINARY:
        DEBUG_VERBOSE("Binary expression with operator %d", expr->as.binary.operator);
        code_gen_binary_expression(gen, &expr->as.binary);

        // Boolean conversion for comparison operators
        if (expr->expr_type && expr->expr_type->kind == TYPE_BOOL)
        {
            fprintf(gen->output, "    test rax, rax\n");
            fprintf(gen->output, "    setne al\n");
            fprintf(gen->output, "    movzx rax, al\n");
        }
        break;
    case EXPR_UNARY:
        DEBUG_VERBOSE("Unary expression with operator %d", expr->as.unary.operator);
        code_gen_unary_expression(gen, &expr->as.unary);
        break;
    case EXPR_LITERAL:
        if (expr->as.literal.type)
        {
            DEBUG_VERBOSE("Literal expression of type %s",
                          ast_type_to_string(expr->as.literal.type));
        }
        code_gen_literal_expression(gen, &expr->as.literal);
        break;
    case EXPR_VARIABLE:
        DEBUG_VERBOSE("Variable expression");
        code_gen_variable_expression(gen, &expr->as.variable);
        break;
    case EXPR_ASSIGN:
        DEBUG_VERBOSE("Assignment expression");
        code_gen_assign_expression(gen, &expr->as.assign);
        break;
    case EXPR_CALL:
        DEBUG_VERBOSE("Call expression");
        code_gen_call_expression(gen, expr);
        break;
    case EXPR_ARRAY:
        DEBUG_VERBOSE("Array expression");
        code_gen_array_expression(gen, &expr->as.array);
        break;
    case EXPR_ARRAY_ACCESS:
        DEBUG_VERBOSE("Array access expression");
        code_gen_array_access_expression(gen, &expr->as.array_access);
        break;
    case EXPR_INCREMENT:
        DEBUG_VERBOSE("Increment expression");
        code_gen_increment_expression(gen, expr);
        break;
    case EXPR_DECREMENT:
        DEBUG_VERBOSE("Decrement expression");
        code_gen_decrement_expression(gen, expr);
        break;
    }

    DEBUG_VERBOSE("Finished generating expression in function '%s'",
                  gen->current_function ? gen->current_function : "global");

    code_gen_pop_function_context(gen);
}

void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt)
{
    code_gen_expression(gen, stmt->expression);
    // Result is discarded for expression statements
}

void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt)
{
    symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL);
    // If there's an initializer, evaluate it
    if (stmt->initializer != NULL)
    {
        code_gen_expression(gen, stmt->initializer);
    }
    else
    {
        // Default initialization to 0
        fprintf(gen->output, "    xor rax, rax\n");
    }

    // Get the variable offset from the symbol table
    int offset = code_gen_get_var_offset(gen, stmt->name);

    // Store it in the variable's location
    fprintf(gen->output, "    mov [rbp + %d], rax\n", offset);
}

void code_gen_block(CodeGen *gen, BlockStmt *stmt)
{
    // Generate code for each statement in the block
    symbol_table_push_scope(gen->symbol_table);
    for (int i = 0; i < stmt->count; i++)
    {
        code_gen_statement(gen, stmt->statements[i]);
    }
    symbol_table_pop_scope(gen->symbol_table);
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
        // No locals to add
        break;
    }
}

void code_gen_function(CodeGen *gen, FunctionStmt *stmt)
{
    DEBUG_VERBOSE("==== GENERATING FUNCTION '%.*s' ====", stmt->name.length, stmt->name.start);

    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;

    if (stmt->name.length < 256)
    {
        gen->current_function = malloc(stmt->name.length + 1);
        if (gen->current_function == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        strncpy(gen->current_function, stmt->name.start, stmt->name.length);
        gen->current_function[stmt->name.length] = '\0';
    }
    else
    {
        DEBUG_ERROR("Function name too long");
        exit(1);
    }
    gen->current_return_type = stmt->return_type;

    symbol_table_push_scope(gen->symbol_table);
    for (int i = 0; i < stmt->param_count; i++)
    {
        symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->params[i].name, stmt->params[i].type, SYMBOL_PARAM);
    }

    // Pre-pass to compute max stack usage
    for (int i = 0; i < stmt->body_count; i++)
    {
        pre_build_symbols(gen, stmt->body[i]);
    }

    // Compute aligned stack space (locals + padding for calls/alignments/pushes)
    int local_space = gen->symbol_table->current->next_local_offset;
    int stack_space = local_space + 128; // Padding for calls, alignments, pushes
    stack_space = ((stack_space + 15) / 16) * 16;
    if (stack_space < 128)
        stack_space = 128;

    // Generate prologue with computed space
    fprintf(gen->output, "%s:\n", gen->current_function);
    fprintf(gen->output, "    push rbp\n");
    fprintf(gen->output, "    mov rbp, rsp\n");
    fprintf(gen->output, "    sub rsp, %d\n", stack_space);

    // Store parameters in their stack slots
    const char *param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    for (int i = 0; i < stmt->param_count && i < 6; i++)
    {
        int offset = symbol_table_get_symbol_offset(gen->symbol_table, stmt->params[i].name);
        fprintf(gen->output, "    mov [rbp + %d], %s\n", offset, param_regs[i]);
    }

    // Generate body
    for (int i = 0; i < stmt->body_count; i++)
    {
        code_gen_statement(gen, stmt->body[i]);
    }

    // Epilogue
    if (strcmp(gen->current_function, "main") == 0)
    {
        fprintf(gen->output, "main_return:\n");
        fprintf(gen->output, "    mov rax, 0\n");
        fprintf(gen->output, "    mov rsp, rbp\n");
        fprintf(gen->output, "    pop rbp\n");
        fprintf(gen->output, "    ret\n");
    }
    else
    {
        fprintf(gen->output, "%s_return:\n", gen->current_function);
        fprintf(gen->output, "    mov rsp, rbp\n");
        fprintf(gen->output, "    pop rbp\n");
        fprintf(gen->output, "    ret\n");
    }

    symbol_table_pop_scope(gen->symbol_table);
    free(gen->current_function);
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;

    DEBUG_VERBOSE("==== COMPLETED FUNCTION '%.*s' (stack space: %d) ====", stmt->name.length, stmt->name.start, stack_space);
}

void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt)
{
    // Evaluate the return value if provided
    if (stmt->value != NULL)
    {
        code_gen_expression(gen, stmt->value);
    }
    else
    {
        // Default return value is 0
        fprintf(gen->output, "    xor rax, rax\n");
    }

    // Jump to the function's return label (special for main)
    if (gen->current_function && strcmp(gen->current_function, "main") == 0)
    {
        fprintf(gen->output, "    jmp main_return\n");
    }
    else
    {
        // For other functions, jump to their specific return label
        fprintf(gen->output, "    jmp %s_return\n", gen->current_function);
    }
}

void code_gen_if_statement(CodeGen *gen, IfStmt *stmt)
{
    int else_label = code_gen_new_label(gen);
    int end_label = code_gen_new_label(gen);

    // Generate condition
    code_gen_expression(gen, stmt->condition);

    // Test condition
    fprintf(gen->output, "    test rax, rax\n");
    fprintf(gen->output, "    jz .L%d\n", else_label);

    // Generate then branch
    code_gen_statement(gen, stmt->then_branch);

    // Jump to end if there's an else branch
    if (stmt->else_branch != NULL)
    {
        fprintf(gen->output, "    jmp .L%d\n", end_label);
    }

    // Generate else branch
    fprintf(gen->output, ".L%d:\n", else_label);
    if (stmt->else_branch != NULL)
    {
        code_gen_statement(gen, stmt->else_branch);
    }

    // End label if there was an else branch
    if (stmt->else_branch != NULL)
    {
        fprintf(gen->output, ".L%d:\n", end_label);
    }
}

void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt)
{
    int loop_start = code_gen_new_label(gen);
    int loop_end = code_gen_new_label(gen);

    // Loop start label
    fprintf(gen->output, ".L%d: ; Loop start\n", loop_start);

    // Generate and check condition
    code_gen_expression(gen, stmt->condition);

    // If condition is false, jump to end
    fprintf(gen->output, "    test rax, rax\n");
    fprintf(gen->output, "    jz .L%d ; Exit if condition is false\n", loop_end);

    // Generate the entire loop body
    if (stmt->body->type == STMT_BLOCK)
    {
        code_gen_block(gen, &stmt->body->as.block); // Handle block statements
    }
    else
    {
        code_gen_statement(gen, stmt->body); // Handle single statements
    }

    // Jump back to start of loop
    fprintf(gen->output, "    jmp .L%d ; Return to condition\n", loop_start);

    // Loop end label
    fprintf(gen->output, ".L%d: ; Loop end\n", loop_end);
}

void code_gen_for_statement(CodeGen *gen, ForStmt *stmt)
{
    int loop_start = code_gen_new_label(gen);
    int loop_end = code_gen_new_label(gen);

    // Initializer (create a new scope)
    symbol_table_push_scope(gen->symbol_table);
    if (stmt->initializer != NULL)
    {
        code_gen_statement(gen, stmt->initializer);
    }

    // Loop start label
    fprintf(gen->output, ".L%d: ; Loop start\n", loop_start);

    // Condition check (if present)
    if (stmt->condition != NULL)
    {
        code_gen_expression(gen, stmt->condition);
        fprintf(gen->output, "    test rax, rax\n");
        fprintf(gen->output, "    jz .L%d ; Exit if condition is false\n", loop_end);
    }

    // Generate loop body
    code_gen_statement(gen, stmt->body);

    // Increment (if present)
    if (stmt->increment != NULL)
    {
        code_gen_expression(gen, stmt->increment);
    }

    // Jump back to start of loop
    fprintf(gen->output, "    jmp .L%d ; Return to condition\n", loop_start);

    // Loop end label
    fprintf(gen->output, ".L%d: ; Loop end\n", loop_end);

    // Pop the scope
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
        // Import statements are handled at an earlier stage
        break;
    }
}

void code_gen_footer(CodeGen *gen)
{
    fprintf(gen->output, "\nsection .note.GNU-stack noalloc noexec nowrite progbits\n");
}

void code_gen_module(CodeGen *gen, Module *module)
{
    // Set up the assembly file
    code_gen_text_section(gen);

    // DON'T generate _start function (entry point) -
    // GCC's runtime will provide this

    // Generate code for each statement in the module
    for (int i = 0; i < module->count; i++)
    {
        code_gen_statement(gen, module->statements[i]);
    }

    // Generate data section (string literals, etc.)
    code_gen_data_section(gen);

    code_gen_footer(gen);
}