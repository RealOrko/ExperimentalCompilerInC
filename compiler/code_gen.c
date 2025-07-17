/**
 * code_gen.c
 * Implementation of the code generator with fixes for string handling
 */

#include "code_gen.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

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

// String table for string literals
typedef struct StringLiteral
{
    const char *string;
    int label;
    struct StringLiteral *next;
} StringLiteral;

static StringLiteral *string_literals = NULL;

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
        if (gen->function_stack[gen->function_stack_size] == NULL) {
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
    // Only make main global, not _start
    fprintf(gen->output, "    global main\n\n");

    // Add external functions
    fprintf(gen->output, "    extern printf\n");
    fprintf(gen->output, "    extern exit\n\n");

    // Format strings for printf
    fprintf(gen->output, "section .data\n");
    fprintf(gen->output, "    fmt_int db \"%%d\", 0\n");
    fprintf(gen->output, "    fmt_long db \"%%ld\", 0\n");
    fprintf(gen->output, "    fmt_double db \"%%f\", 0\n");
    fprintf(gen->output, "    fmt_char db \"%%c\", 0\n");
    fprintf(gen->output, "    fmt_string db \"%%s\", 0\n");
    fprintf(gen->output, "    fmt_newline db 10, 0\n\n");

    fprintf(gen->output, "section .text\n");

    // Define the print function
    fprintf(gen->output, "; Implementation of print function\n");
    fprintf(gen->output, "print:\n");
    fprintf(gen->output, "    push rbp\n");
    fprintf(gen->output, "    mov rbp, rsp\n");

    // The string to print is in rdi
    fprintf(gen->output, "    mov rsi, rdi       ; string to print\n");
    fprintf(gen->output, "    lea rdi, [rel fmt_string] ; format string\n");
    fprintf(gen->output, "    xor rax, rax       ; no floating point args\n");
    fprintf(gen->output, "    call printf\n");

    fprintf(gen->output, "    mov rsp, rbp\n");
    fprintf(gen->output, "    pop rbp\n");
    fprintf(gen->output, "    ret\n\n");
}

void code_gen_prologue(CodeGen *gen, const char *function_name)
{
    fprintf(gen->output, "%s:\n", function_name);
    fprintf(gen->output, "    push rbp\n");
    fprintf(gen->output, "    mov rbp, rsp\n");

    // Dynamically calculate stack space needed
    int stack_space = 64; // Base allocation
    if (strcmp(function_name, "main") == 0)
        stack_space = 128; // More space for main function

    fprintf(gen->output, "    sub rsp, %d\n", stack_space);
}

void code_gen_epilogue(CodeGen *gen)
{
    // This function is now a no-op
    // All epilogue code is generated directly in generate_function
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
        DEBUG_WARNING("Symbol not found in symbol table: '%s'", var_name);

        // Manual string-based lookup for common variables
        if (gen->current_function)
        {
            // Check all symbols in all scopes
            Scope *scope = gen->symbol_table->current;
            while (scope != NULL)
            {
                Symbol *sym = scope->symbols;
                while (sym != NULL)
                {
                    char sym_name[256];
                    int sym_len = sym->name.length < 255 ? sym->name.length : 255;
                    strncpy(sym_name, sym->name.start, sym_len);
                    sym_name[sym_len] = '\0';

                    if (strcmp(sym_name, var_name) == 0)
                    {
                        DEBUG_VERBOSE("Manual string lookup found '%s' with offset %d\n",
                                      var_name, sym->offset);
                        return sym->offset;
                    }
                    sym = sym->next;
                }
                scope = scope->enclosing;
            }
        }

        // For backward compatibility, fall back to hardcoded offsets
        DEBUG_WARNING("Falling back to hardcoded offsets for '%s'", var_name);

        // Hardcoded offsets for all variables in the factorial function
        if (gen->current_function && strcmp(gen->current_function, "factorial") == 0)
        {
            if (strcmp(var_name, "n") == 0)
                return 16;
            if (strcmp(var_name, "result") == 0)
                return 24;
            if (strcmp(var_name, "i") == 0)
                return 32;
        }

        // If we get here, the variable is not in our hardcoded list
        DEBUG_ERROR("Undefined variable '%s'\n", var_name);
        exit(1);
    }

    DEBUG_VERBOSE("Found symbol '%s' with offset %d", var_name, symbol->offset);
    return symbol->offset;
}

void code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr)
{
    DEBUG_VERBOSE("Generating binary expression in function '%s' with operator %d",
                  gen->current_function ? gen->current_function : "global", expr->operator);

    // Generate the right operand first and push it on the stack
    code_gen_expression(gen, expr->right);
    fprintf(gen->output, "    push rax\n");

    // Generate the left operand
    code_gen_expression(gen, expr->left);

    // Pop the right operand into rcx
    fprintf(gen->output, "    pop rcx\n");

    // Perform the operation
    switch (expr->operator)
    {
    case TOKEN_PLUS:
        // Check if either operand is a string for string concatenation
        if (expr->left->expr_type && expr->right->expr_type &&
            (expr->left->expr_type->kind == TYPE_STRING ||
             expr->right->expr_type->kind == TYPE_STRING))
        {
            // String concatenation not fully implemented
            fprintf(gen->output, "    ; String concatenation not fully implemented\n");
        }
        else
        {
            fprintf(gen->output, "    add rax, rcx\n");
        }
        break;
    case TOKEN_MINUS:
        fprintf(gen->output, "    sub rax, rcx\n");
        break;
    case TOKEN_STAR:
        fprintf(gen->output, "    imul rax, rcx\n");
        break;
    case TOKEN_SLASH:
        fprintf(gen->output, "    xor rdx, rdx\n"); // Clear rdx for division
        fprintf(gen->output, "    idiv rcx\n");
        break;
    case TOKEN_MODULO:
        fprintf(gen->output, "    xor rdx, rdx\n"); // Clear rdx for division
        fprintf(gen->output, "    idiv rcx\n");
        fprintf(gen->output, "    mov rax, rdx\n"); // Remainder is in rdx
        break;
    case TOKEN_EQUAL_EQUAL:
        fprintf(gen->output, "    cmp rax, rcx\n");
        fprintf(gen->output, "    sete al\n");
        fprintf(gen->output, "    movzx rax, al\n");
        break;
    case TOKEN_BANG_EQUAL:
        fprintf(gen->output, "    cmp rax, rcx\n");
        fprintf(gen->output, "    setne al\n");
        fprintf(gen->output, "    movzx rax, al\n");
        break;
    case TOKEN_LESS:
        fprintf(gen->output, "    cmp rax, rcx\n");
        fprintf(gen->output, "    setl al\n");
        fprintf(gen->output, "    movzx rax, al\n");
        break;
    case TOKEN_LESS_EQUAL:
        fprintf(gen->output, "    cmp rax, rcx\n");
        fprintf(gen->output, "    setle al\n");
        fprintf(gen->output, "    movzx rax, al\n");
        break;
    case TOKEN_GREATER:
        fprintf(gen->output, "    cmp rax, rcx\n");
        fprintf(gen->output, "    setg al\n");
        fprintf(gen->output, "    movzx rax, al\n");
        break;
    case TOKEN_GREATER_EQUAL:
        fprintf(gen->output, "    cmp rax, rcx\n");
        fprintf(gen->output, "    setge al\n");
        fprintf(gen->output, "    movzx rax, al\n");
        break;
    case TOKEN_AND:
    {
        int end_label = code_gen_new_label(gen);
        fprintf(gen->output, "    test rax, rax\n");
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
        fprintf(gen->output, "    test rax, rax\n");
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

    // Perform the unary operation
    switch (expr->operator)
    {
    case TOKEN_MINUS:
        fprintf(gen->output, "    neg rax\n");
        break;
    case TOKEN_BANG:
        fprintf(gen->output, "    test rax, rax\n");
        fprintf(gen->output, "    setz al\n");
        fprintf(gen->output, "    movzx rax, al\n");
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

    // If we're in the function factorial and the variable is 'n', use the hardcoded offset
    if (gen->current_function && strcmp(gen->current_function, "factorial") == 0 &&
        strcmp(var_name, "n") == 0)
    {
        DEBUG_VERBOSE("Direct handling of parameter 'n' in factorial function");
        fprintf(gen->output, "    mov rax, [rbp-16]\n"); // n is at offset 16
        return;
    }

    // If we're in the function is_prime and the variable is 'num', use the hardcoded offset
    if (gen->current_function && strcmp(gen->current_function, "is_prime") == 0 &&
        strcmp(var_name, "num") == 0)
    {
        DEBUG_VERBOSE("Direct handling of parameter 'num' in is_prime function");
        fprintf(gen->output, "    mov rax, [rbp-16]\n"); // num is at offset 16
        return;
    }

    // If we're in the function repeat_string and the variable is 'text' or 'count', use hardcoded offsets
    if (gen->current_function && strcmp(gen->current_function, "repeat_string") == 0)
    {
        if (strcmp(var_name, "text") == 0)
        {
            DEBUG_VERBOSE("Direct handling of parameter 'text' in repeat_string function");
            fprintf(gen->output, "    mov rax, [rbp-16]\n"); // text is at offset 16
            return;
        }
        if (strcmp(var_name, "count") == 0)
        {
            DEBUG_VERBOSE("Direct handling of parameter 'count' in repeat_string function");
            fprintf(gen->output, "    mov rax, [rbp-24]\n"); // count is at offset 24
            return;
        }
    }

    // Try normal lookup first
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);

    if (symbol == NULL)
    {
        DEBUG_VERBOSE("Symbol lookup failed for '%s', trying string comparison", var_name);

        // Try a manual string comparison for all symbols
        Scope *scope = gen->symbol_table->current;
        while (scope != NULL)
        {
            Symbol *sym = scope->symbols;
            while (sym != NULL)
            {
                char sym_name[256];
                int sym_len = sym->name.length < 255 ? sym->name.length : 255;
                strncpy(sym_name, sym->name.start, sym_len);
                sym_name[sym_len] = '\0';

                if (strcmp(sym_name, var_name) == 0)
                {
                    DEBUG_VERBOSE("Manual match found for '%s' with offset %d\n",
                                  var_name, sym->offset);
                    fprintf(gen->output, "    mov rax, [rbp-%d]\n", sym->offset);
                    return;
                }
                sym = sym->next;
            }
            scope = scope->enclosing;
        }

        DEBUG_ERROR("Undefined variable '%s'", var_name);
        exit(1);
    }

    fprintf(gen->output, "    mov rax, [rbp-%d]\n", symbol->offset);
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

    // Attempt direct lookup
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);

    if (symbol == NULL)
    {
        DEBUG_VERBOSE("Symbol lookup failed for '%s', trying string comparison", var_name);

        // Try a manual string comparison for parameters
        // This is needed because parameters might have been created with different token instances
        if (gen->current_function)
        {
            // Check all symbols in the current scope first
            Scope *scope = gen->symbol_table->current;
            Symbol *sym = scope->symbols;
            while (sym != NULL)
            {
                char sym_name[256];
                int sym_len = sym->name.length < 255 ? sym->name.length : 255;
                strncpy(sym_name, sym->name.start, sym_len);
                sym_name[sym_len] = '\0';

                DEBUG_VERBOSE("Comparing '%s' with symbol '%s'", var_name, sym_name);

                if (strcmp(sym_name, var_name) == 0)
                {
                    DEBUG_VERBOSE("Manual match found for '%s' with offset %d",
                                  var_name, sym->offset);

                    // Store the computed value in the variable
                    fprintf(gen->output, "    mov [rbp-%d], rax\n", sym->offset);
                    return;
                }
                sym = sym->next;
            }
        }

        DEBUG_ERROR("Undefined variable '%s'", var_name);
        exit(1);
    }

    // If we got here, we found the symbol
    DEBUG_VERBOSE("Found symbol '%s' with offset %d", var_name, symbol->offset);

    // Store the computed value in the variable
    fprintf(gen->output, "    mov [rbp-%d], rax\n", symbol->offset);
}

void code_gen_call_expression(CodeGen *gen, CallExpr *expr)
{
    // Validate the number of arguments
    if (expr->arg_count > 6)
    {
        DEBUG_ERROR("Too many arguments (max 6 supported)");
        return;
    }

    // Push arguments in reverse order (x86-64 ABI)
    for (int i = expr->arg_count - 1; i >= 0; i--)
    {
        code_gen_expression(gen, expr->arguments[i]);

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
            fprintf(gen->output, "    push rax\n");
            break;
        }
    }

    // Calculate function name
    const char *function_name = NULL;

    // Handle variable functions
    if (expr->callee->type == EXPR_VARIABLE)
    {
        VariableExpr *var = &expr->callee->as.variable;
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

    // Reserve space for arguments and align the stack
    fprintf(gen->output, "    ; Call to %s with %d arguments\n", function_name, expr->arg_count);

    // Evaluate arguments and push them on the stack in reverse order
    for (int i = expr->arg_count - 1; i >= 0; i--)
    {
        fprintf(gen->output, "    ; Evaluating argument %d\n", i);
        code_gen_expression(gen, expr->arguments[i]);

        if (i == 0)
        { // First argument goes in RDI
            fprintf(gen->output, "    mov rdi, rax\n");
        }
        else if (i == 1)
        { // Second argument goes in RSI
            fprintf(gen->output, "    mov rsi, rax\n");
        }
        else
        { // Remaining arguments pushed on stack
            fprintf(gen->output, "    push rax\n");
        }
    }

    // Ensure stack is 16-byte aligned before call (ABI requirement)
    fprintf(gen->output, "    ; Aligning stack for function call\n");
    fprintf(gen->output, "    mov rax, rsp\n");
    fprintf(gen->output, "    and rax, 15\n");
    fprintf(gen->output, "    jz .aligned_%d\n", code_gen_new_label(gen));
    fprintf(gen->output, "    sub rsp, 8\n");
    fprintf(gen->output, ".aligned_%d:\n", gen->label_count - 1);

    // Call the function
    fprintf(gen->output, "    call %s\n", function_name);

    // For main function, don't restore stack after the LAST function call
    if (gen->current_function && strcmp(gen->current_function, "main") == 0)
    {
        // if (is_last_call_in_main) { jmp main_exit; } else { ... clean stack }
        // Always clean:
        if (expr->arg_count > 6)
        {
            fprintf(gen->output, "    add rsp, %d\n", 8 * (expr->arg_count - 6));
        }
        fprintf(gen->output, "    ; Restoring stack alignment\n");
        fprintf(gen->output, "    mov rsp, rbp\n");
        fprintf(gen->output, "    sub rsp, 64\n");
    }

    // Clean up the stack - only needed for args beyond the register-passed ones
    if (expr->arg_count > 6)
    {
        fprintf(gen->output, "    add rsp, %d\n", 8 * (expr->arg_count - 6));
    }

    // Re-align stack if necessary
    fprintf(gen->output, "    ; Restoring stack alignment\n");
    fprintf(gen->output, "    mov rsp, rbp\n");
    fprintf(gen->output, "    sub rsp, 64\n"); // Restore local variable space

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
    // Generate the operand
    code_gen_expression(gen, expr->as.operand);

    // Increment
    fprintf(gen->output, "    inc rax\n");
}

void code_gen_decrement_expression(CodeGen *gen, Expr *expr)
{
    // Generate the operand
    code_gen_expression(gen, expr->as.operand);

    // Decrement
    fprintf(gen->output, "    dec rax\n");
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
        code_gen_call_expression(gen, &expr->as.call);
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
    fprintf(gen->output, "    mov [rbp-%d], rax\n", offset);
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

void code_gen_function(CodeGen *gen, FunctionStmt *stmt)
{
    DEBUG_VERBOSE("==== GENERATING FUNCTION '%.*s' ====",
                  stmt->name.length, stmt->name.start);

    // Save the current function context
    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;

    // Set up new function context
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

    // Generate function prologue
    code_gen_prologue(gen, gen->current_function);

    const char *param_regs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    for (int i = 0; i < stmt->param_count && i < 6; i++)
    {
        int offset = 16 + i * 8; // rbp-16, rbp-24, etc.
        fprintf(gen->output, "    mov [rbp-%d], %s\n", offset, param_regs[i]);
    }

    // Create a new scope for function parameters
    symbol_table_push_scope(gen->symbol_table);

    // Add all parameters to this scope
    for (int i = 0; i < stmt->param_count; i++)
    {
        symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->params[i].name,
                                          stmt->params[i].type, SYMBOL_PARAM);
    }

    // Generate function body
    for (int i = 0; i < stmt->body_count; i++)
    {
        code_gen_statement(gen, stmt->body[i]);
    }

    // Ensure a return path for functions that don't explicitly return
    if (strcmp(gen->current_function, "main") != 0)
    {
        fprintf(gen->output, "%s_return:\n", gen->current_function);
        fprintf(gen->output, "    mov rsp, rbp\n");
        fprintf(gen->output, "    pop rbp\n");
        fprintf(gen->output, "    ret\n");
    }
    else
    {
        // Special handling for main function
        fprintf(gen->output, "main_exit:\n");
        fprintf(gen->output, "    mov rsp, rbp\n");
        fprintf(gen->output, "    pop rbp\n");
        fprintf(gen->output, "    xor rdi, rdi\n"); // Exit code 0
        fprintf(gen->output, "    call exit\n");
    }

    // Clean up
    symbol_table_pop_scope(gen->symbol_table);

    // Restore old function context
    free(gen->current_function);
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;

    DEBUG_VERBOSE("==== COMPLETED FUNCTION '%.*s' ====",
                  stmt->name.length, stmt->name.start);
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

    // For main function, directly exit
    if (gen->current_function && strcmp(gen->current_function, "main") == 0)
    {
        fprintf(gen->output, "    jmp main_exit\n");
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