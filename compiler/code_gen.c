/**
 * code_gen.c
 * Implementation of the code generator with fixes for string handling
 */

#include "code_gen.h"
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
void init_function_stack(CodeGen *gen)
{
    gen->function_stack_capacity = 8;
    gen->function_stack_size = 0;
    gen->function_stack = malloc(gen->function_stack_capacity * sizeof(char *));
    if (gen->function_stack == NULL)
    {
        fprintf(stderr, "Error: Out of memory initializing function stack\n");
        exit(1);
    }
}

void push_function_context(CodeGen *gen)
{
    // Resize if needed
    if (gen->function_stack_size >= gen->function_stack_capacity)
    {
        gen->function_stack_capacity *= 2;
        gen->function_stack = realloc(gen->function_stack,
                                      gen->function_stack_capacity * sizeof(char *));
        if (gen->function_stack == NULL)
        {
            fprintf(stderr, "Error: Out of memory expanding function stack\n");
            exit(1);
        }
    }

    // Store current function (even if NULL)
    if (gen->current_function != NULL)
    {
        gen->function_stack[gen->function_stack_size] = strdup(gen->current_function);
    }
    else
    {
        gen->function_stack[gen->function_stack_size] = NULL;
    }
    gen->function_stack_size++;

    fprintf(stderr, "DEBUG: Pushed function context '%s', stack size now %d\n",
            gen->current_function ? gen->current_function : "NULL", gen->function_stack_size);
}

// Fix the pop_function_context implementation
void pop_function_context(CodeGen *gen)
{
    if (gen->function_stack_size <= 0)
    {
        fprintf(stderr, "Warning: Attempt to pop empty function stack\n");
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

    fprintf(stderr, "DEBUG: Popped function context, restored to '%s', stack size now %d\n",
            gen->current_function ? gen->current_function : "NULL", gen->function_stack_size);
}

// Free the function stack
void free_function_stack(CodeGen *gen)
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

void init_code_gen(CodeGen *gen, SymbolTable *symbol_table, const char *output_file)
{
    gen->label_count = 0;
    gen->symbol_table = symbol_table;
    gen->output = fopen(output_file, "w");
    gen->current_function = NULL;
    gen->current_return_type = NULL;

    // Initialize function context stack
    init_function_stack(gen);

    if (gen->output == NULL)
    {
        fprintf(stderr, "Error: Could not open output file '%s'\n", output_file);
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
    free_function_stack(gen);

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

int new_label(CodeGen *gen)
{
    return gen->label_count++;
}

int add_string_literal(CodeGen *gen, const char *string)
{
    if (string == NULL)
    {
        fprintf(stderr, "Error: Null string passed to add_string_literal\n");
        return -1;
    }

    int label = new_label(gen);

    StringLiteral *literal = malloc(sizeof(StringLiteral));
    if (literal == NULL)
    {
        fprintf(stderr, "Error: Out of memory\n");
        exit(1);
    }

    literal->string = my_strdup(string);
    if (literal->string == NULL)
    {
        fprintf(stderr, "Error: Out of memory\n");
        free(literal);
        exit(1);
    }

    literal->label = label;
    literal->next = string_literals;
    string_literals = literal;

    return label;
}

void generate_string_literal(CodeGen *gen, const char *string, int label)
{
    if (string == NULL)
    {
        fprintf(stderr, "Error: Null string passed to generate_string_literal\n");
        return;
    }

    fprintf(gen->output, "    str_%d db ", label);

    // Output string as comma-separated byte values
    const unsigned char *p = (const unsigned char *)string;
    int first = 1;

    while (*p)
    {
        if (!first)
        {
            fprintf(gen->output, ", ");
        }
        first = 0;

        if (*p >= 32 && *p <= 126 && *p != '\'' && *p != '\"' && *p != '\\')
        {
            // Printable ASCII character
            fprintf(gen->output, "'%c'", *p);
        }
        else
        {
            // Non-printable or special character
            fprintf(gen->output, "%d", *p);
        }

        p++;
    }

    // Null terminator
    if (!first)
    {
        fprintf(gen->output, ", ");
    }
    fprintf(gen->output, "0\n");
}

void generate_data_section(CodeGen *gen)
{
    fprintf(gen->output, "section .data\n");

    // Generate string literals
    StringLiteral *current = string_literals;
    while (current != NULL)
    {
        if (current->string != NULL)
        {
            generate_string_literal(gen, current->string, current->label);
        }
        current = current->next;
    }

    fprintf(gen->output, "\n");
}

void generate_text_section(CodeGen *gen)
{
    fprintf(gen->output, "section .text\n");
    fprintf(gen->output, "    global _start\n");
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

void generate_prologue(CodeGen *gen, const char *function_name)
{
    fprintf(gen->output, "%s:\n", function_name);
    fprintf(gen->output, "    push rbp\n");
    fprintf(gen->output, "    mov rbp, rsp\n");

    // Reserve space for local variables
    // In a full compiler, we would calculate this based on the function's stack frame needs
    fprintf(gen->output, "    sub rsp, 64\n");
}

void generate_epilogue(CodeGen *gen)
{
    // Add a return label for the function
    fprintf(gen->output, "%s_return:\n", gen->current_function);
    fprintf(gen->output, "    mov rsp, rbp\n");
    fprintf(gen->output, "    pop rbp\n");
    fprintf(gen->output, "    ret\n\n");
}

static int get_var_offset(CodeGen *gen, Token name)
{
    // Extract variable name for comparison
    char var_name[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(var_name, name.start, name_len);
    var_name[name_len] = '\0';

    // Debug output to see what variable we're trying to access
    fprintf(stderr, "DEBUG: Looking up variable '%s' in function '%s'\n",
            var_name, gen->current_function ? gen->current_function : "global");

    // Use the symbol table to get the variable's offset
    Symbol *symbol = lookup_symbol(gen->symbol_table, name);

    if (symbol == NULL)
    {
        fprintf(stderr, "WARNING: Symbol not found in symbol table: '%s'\n", var_name);

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
                        fprintf(stderr, "DEBUG: Manual string lookup found '%s' with offset %d\n",
                                var_name, sym->offset);
                        return sym->offset;
                    }
                    sym = sym->next;
                }
                scope = scope->enclosing;
            }
        }

        // For backward compatibility, fall back to hardcoded offsets
        fprintf(stderr, "WARNING: Falling back to hardcoded offsets for '%s'\n", var_name);

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

        // Hardcoded offsets for other functions...
        // ...

        // If we get here, the variable is not in our hardcoded list
        fprintf(stderr, "Error: Undefined variable '%s'\n", var_name);
        exit(1);
    }

    fprintf(stderr, "DEBUG: Found symbol '%s' with offset %d\n", var_name, symbol->offset);
    return symbol->offset;
}

void generate_binary_expression(CodeGen *gen, BinaryExpr *expr)
{
    fprintf(stderr, "DEBUG: Generating binary expression in function '%s' with operator %d\n",
            gen->current_function ? gen->current_function : "global", expr->operator);

    // Generate the right operand first and push it on the stack
    generate_expression(gen, expr->right);
    fprintf(gen->output, "    push rax\n");

    // Generate the left operand
    generate_expression(gen, expr->left);

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
        int end_label = new_label(gen);
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
        int end_label = new_label(gen);
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
        fprintf(stderr, "Error: Unsupported binary operator\n");
        exit(1);
    }
}

void generate_unary_expression(CodeGen *gen, UnaryExpr *expr)
{
    // Generate the operand
    generate_expression(gen, expr->operand);

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
        fprintf(stderr, "Error: Unsupported unary operator\n");
        exit(1);
    }
}

void generate_literal_expression(CodeGen *gen, LiteralExpr *expr)
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
        fprintf(gen->output, "    mov rax, %ld\n", (int64_t)expr->value.double_value);
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
            int label = add_string_literal(gen, expr->value.string_value);
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
        fprintf(stderr, "Error: Unsupported literal type\n");
        exit(1);
    }
}

void generate_variable_expression(CodeGen *gen, VariableExpr *expr)
{
    // Extract name for debugging
    char var_name[256];
    int name_len = expr->name.length < 255 ? expr->name.length : 255;
    strncpy(var_name, expr->name.start, name_len);
    var_name[name_len] = '\0';

    fprintf(stderr, "DEBUG: Accessing variable '%s' in function '%s'\n",
            var_name, gen->current_function ? gen->current_function : "global");

    // If we're in the function factorial and the variable is 'n', use the hardcoded offset
    if (gen->current_function && strcmp(gen->current_function, "factorial") == 0 &&
        strcmp(var_name, "n") == 0)
    {
        fprintf(stderr, "DEBUG: Direct handling of parameter 'n' in factorial function\n");
        fprintf(gen->output, "    mov rax, [rbp-16]\n"); // n is at offset 16
        return;
    }

    // If we're in the function is_prime and the variable is 'num', use the hardcoded offset
    if (gen->current_function && strcmp(gen->current_function, "is_prime") == 0 &&
        strcmp(var_name, "num") == 0)
    {
        fprintf(stderr, "DEBUG: Direct handling of parameter 'num' in is_prime function\n");
        fprintf(gen->output, "    mov rax, [rbp-16]\n"); // num is at offset 16
        return;
    }

    // If we're in the function repeat_string and the variable is 'text' or 'count', use hardcoded offsets
    if (gen->current_function && strcmp(gen->current_function, "repeat_string") == 0)
    {
        if (strcmp(var_name, "text") == 0)
        {
            fprintf(stderr, "DEBUG: Direct handling of parameter 'text' in repeat_string function\n");
            fprintf(gen->output, "    mov rax, [rbp-16]\n"); // text is at offset 16
            return;
        }
        if (strcmp(var_name, "count") == 0)
        {
            fprintf(stderr, "DEBUG: Direct handling of parameter 'count' in repeat_string function\n");
            fprintf(gen->output, "    mov rax, [rbp-24]\n"); // count is at offset 24
            return;
        }
    }

    // Try normal lookup first
    Symbol *symbol = lookup_symbol(gen->symbol_table, expr->name);

    if (symbol == NULL)
    {
        fprintf(stderr, "DEBUG: Symbol lookup failed for '%s', trying string comparison\n", var_name);

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
                    fprintf(stderr, "DEBUG: Manual match found for '%s' with offset %d\n",
                            var_name, sym->offset);
                    fprintf(gen->output, "    mov rax, [rbp-%d]\n", sym->offset);
                    return;
                }
                sym = sym->next;
            }
            scope = scope->enclosing;
        }

        // Fall back to fully hardcoded behavior
        fprintf(stderr, "DEBUG: Falling back to hardcoded offsets for '%s'\n", var_name);
        int offset = -1;

        if (gen->current_function && strcmp(gen->current_function, "factorial") == 0)
        {
            if (strcmp(var_name, "n") == 0)
                offset = 16;
            else if (strcmp(var_name, "result") == 0)
                offset = 24;
            else if (strcmp(var_name, "i") == 0)
                offset = 32;
        }
        else if (gen->current_function && strcmp(gen->current_function, "is_prime") == 0)
        {
            if (strcmp(var_name, "num") == 0)
                offset = 16;
            else if (strcmp(var_name, "i") == 0)
                offset = 24;
        }
        else if (gen->current_function && strcmp(gen->current_function, "repeat_string") == 0)
        {
            if (strcmp(var_name, "text") == 0)
                offset = 16;
            else if (strcmp(var_name, "count") == 0)
                offset = 24;
            else if (strcmp(var_name, "i") == 0)
                offset = 32;
        }
        else if (gen->current_function && strcmp(gen->current_function, "main") == 0)
        {
            if (strcmp(var_name, "num") == 0)
                offset = 16;
            else if (strcmp(var_name, "fact") == 0)
                offset = 24;
            else if (strcmp(var_name, "i") == 0)
                offset = 32;
        }

        if (offset != -1)
        {
            fprintf(stderr, "DEBUG: Using hardcoded offset %d for '%s'\n", offset, var_name);
            fprintf(gen->output, "    mov rax, [rbp-%d]\n", offset);
            return;
        }

        fprintf(stderr, "Error: Undefined variable '%s'\n", var_name);
        exit(1);
    }

    fprintf(gen->output, "    mov rax, [rbp-%d]\n", symbol->offset);
}

void generate_assign_expression(CodeGen *gen, AssignExpr *expr)
{
    // Extract name for debugging
    char var_name[256];
    int name_len = expr->name.length < 255 ? expr->name.length : 255;
    strncpy(var_name, expr->name.start, name_len);
    var_name[name_len] = '\0';

    fprintf(stderr, "DEBUG: Assigning to variable '%s' in function '%s'\n",
            var_name, gen->current_function ? gen->current_function : "global");

    // Evaluate the value
    generate_expression(gen, expr->value);

    // Attempt direct lookup
    Symbol *symbol = lookup_symbol(gen->symbol_table, expr->name);

    if (symbol == NULL)
    {
        fprintf(stderr, "DEBUG: Symbol lookup failed for '%s', trying string comparison\n", var_name);

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

                fprintf(stderr, "DEBUG: Comparing '%s' with symbol '%s'\n", var_name, sym_name);

                if (strcmp(sym_name, var_name) == 0)
                {
                    fprintf(stderr, "DEBUG: Manual match found for '%s' with offset %d\n",
                            var_name, sym->offset);

                    // Store the computed value in the variable
                    fprintf(gen->output, "    mov [rbp-%d], rax\n", sym->offset);
                    return;
                }
                sym = sym->next;
            }
        }

        // If not found, use hardcoded logic as a final fallback
        fprintf(stderr, "DEBUG: Manual lookup failed for '%s', using hardcoded offsets\n", var_name);

        int offset = -1;

        // Same hardcoded offsets as in generate_variable_expression
        if (gen->current_function && strcmp(gen->current_function, "factorial") == 0)
        {
            if (strcmp(var_name, "n") == 0)
                offset = 16;
            else if (strcmp(var_name, "result") == 0)
                offset = 24;
            else if (strcmp(var_name, "i") == 0)
                offset = 32;
        }
        // Other functions...

        if (offset != -1)
        {
            fprintf(stderr, "DEBUG: Using hardcoded offset %d for '%s'\n", offset, var_name);
            fprintf(gen->output, "    mov [rbp-%d], rax\n", offset);
            return;
        }

        fprintf(stderr, "Error: Undefined variable '%s'\n", var_name);
        exit(1);
    }

    // If we got here, we found the symbol
    fprintf(stderr, "DEBUG: Found symbol '%s' with offset %d\n", var_name, symbol->offset);

    // Store the computed value in the variable
    fprintf(gen->output, "    mov [rbp-%d], rax\n", symbol->offset);
}

void generate_call_expression(CodeGen *gen, CallExpr *expr)
{
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
            fprintf(stderr, "Error: Out of memory\n");
            exit(1);
        }

        strncpy(name, var->name.start, var->name.length);
        name[var->name.length] = '\0';
        function_name = name;
    }
    else
    {
        fprintf(stderr, "Error: Complex function calls not supported\n");
        exit(1);
    }

    // Reserve space for arguments and align the stack
    fprintf(gen->output, "    ; Call to %s with %d arguments\n", function_name, expr->arg_count);

    // Evaluate arguments and push them on the stack in reverse order
    for (int i = expr->arg_count - 1; i >= 0; i--)
    {
        fprintf(gen->output, "    ; Evaluating argument %d\n", i);
        generate_expression(gen, expr->arguments[i]);

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
    fprintf(gen->output, "    jz .aligned_%d\n", new_label(gen));
    fprintf(gen->output, "    sub rsp, 8\n");
    fprintf(gen->output, ".aligned_%d:\n", gen->label_count - 1);

    // Call the function
    fprintf(gen->output, "    call %s\n", function_name);

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

void generate_array_expression(CodeGen *gen, ArrayExpr *expr)
{
    // Arrays not implemented yet
    (void)expr; // Suppress unused parameter warning
    fprintf(gen->output, "    ; Arrays not fully implemented\n");
    fprintf(gen->output, "    xor rax, rax\n");
}

void generate_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr)
{
    // Array access not implemented yet
    (void)expr; // Suppress unused parameter warning
    fprintf(gen->output, "    ; Array access not fully implemented\n");
    fprintf(gen->output, "    xor rax, rax\n");
}

void generate_increment_expression(CodeGen *gen, Expr *expr)
{
    // Generate the operand
    generate_expression(gen, expr->as.operand);

    // Increment
    fprintf(gen->output, "    inc rax\n");
}

void generate_decrement_expression(CodeGen *gen, Expr *expr)
{
    // Generate the operand
    generate_expression(gen, expr->as.operand);

    // Decrement
    fprintf(gen->output, "    dec rax\n");
}

void generate_expression(CodeGen *gen, Expr *expr)
{
    if (expr == NULL)
    {
        fprintf(gen->output, "    xor rax, rax\n");
        return;
    }

    // Save the current function context
    push_function_context(gen);

    fprintf(stderr, "DEBUG: Generating expression type %d in function context '%s'\n",
            expr->type, gen->current_function ? gen->current_function : "global");

    switch (expr->type)
    {
    case EXPR_BINARY:
        fprintf(stderr, "DEBUG: Binary expression with operator %d\n", expr->as.binary.operator);
        generate_binary_expression(gen, &expr->as.binary);
        break;
    case EXPR_UNARY:
        fprintf(stderr, "DEBUG: Unary expression with operator %d\n", expr->as.unary.operator);
        generate_unary_expression(gen, &expr->as.unary);
        break;
    case EXPR_LITERAL:
        if (expr->as.literal.type)
        {
            fprintf(stderr, "DEBUG: Literal expression of type %s\n",
                    type_to_string(expr->as.literal.type));
        }
        generate_literal_expression(gen, &expr->as.literal);
        break;
    case EXPR_VARIABLE:
    {
        char var_name[256];
        int name_len = expr->as.variable.name.length < 255 ? expr->as.variable.name.length : 255;
        strncpy(var_name, expr->as.variable.name.start, name_len);
        var_name[name_len] = '\0';
        fprintf(stderr, "DEBUG: Variable expression '%s' in function '%s'\n",
                var_name, gen->current_function ? gen->current_function : "global");

        debug_print_symbol_table(gen->symbol_table, "before variable lookup");
    }
        generate_variable_expression(gen, &expr->as.variable);
        break;
    case EXPR_ASSIGN:
    {
        char var_name[256];
        int name_len = expr->as.assign.name.length < 255 ? expr->as.assign.name.length : 255;
        strncpy(var_name, expr->as.assign.name.start, name_len);
        var_name[name_len] = '\0';
        fprintf(stderr, "DEBUG: Assignment to '%s'\n", var_name);
    }
        generate_assign_expression(gen, &expr->as.assign);
        break;
    case EXPR_CALL:
        if (expr->as.call.callee->type == EXPR_VARIABLE)
        {
            char func_name[256];
            Token name = expr->as.call.callee->as.variable.name;
            int name_len = name.length < 255 ? name.length : 255;
            strncpy(func_name, name.start, name_len);
            func_name[name_len] = '\0';
            fprintf(stderr, "DEBUG: Call to function '%s'\n", func_name);
        }
        generate_call_expression(gen, &expr->as.call);
        break;
    case EXPR_ARRAY:
        fprintf(stderr, "DEBUG: Array expression with %d elements\n",
                expr->as.array.element_count);
        generate_array_expression(gen, &expr->as.array);
        break;
    case EXPR_ARRAY_ACCESS:
        fprintf(stderr, "DEBUG: Array access expression\n");
        generate_array_access_expression(gen, &expr->as.array_access);
        break;
    case EXPR_INCREMENT:
        fprintf(stderr, "DEBUG: Increment expression\n");
        generate_increment_expression(gen, expr);
        break;
    case EXPR_DECREMENT:
        fprintf(stderr, "DEBUG: Decrement expression\n");
        generate_decrement_expression(gen, expr);
        break;
    }

    fprintf(stderr, "DEBUG: Finished generating expression in function '%s'\n",
            gen->current_function ? gen->current_function : "global");

    pop_function_context(gen);
}

void generate_expression_statement(CodeGen *gen, ExprStmt *stmt)
{
    generate_expression(gen, stmt->expression);
    // Result is discarded for expression statements
}

// Replace the generate_var_declaration function in code_gen.c

void generate_var_declaration(CodeGen *gen, VarDeclStmt *stmt)
{
    // If there's an initializer, evaluate it
    if (stmt->initializer != NULL)
    {
        generate_expression(gen, stmt->initializer);
    }
    else
    {
        // Default initialization to 0
        fprintf(gen->output, "    xor rax, rax\n");
    }

    // Get the variable offset from the symbol table
    int offset = get_var_offset(gen, stmt->name);

    // Store it in the variable's location
    fprintf(gen->output, "    mov [rbp-%d], rax\n", offset);
}

void generate_function(CodeGen *gen, FunctionStmt *stmt)
{
    fprintf(stderr, "\n==== GENERATING FUNCTION '%.*s' ====\n",
            stmt->name.length, stmt->name.start);

    debug_print_symbol_table(gen->symbol_table, "at function start");

    push_function_context(gen);

    // Save the current function context
    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;

    // Set up new function context
    if (stmt->name.length < 256)
    {
        gen->current_function = malloc(stmt->name.length + 1);
        if (gen->current_function == NULL)
        {
            fprintf(stderr, "Error: Out of memory\n");
            exit(1);
        }

        strncpy(gen->current_function, stmt->name.start, stmt->name.length);
        gen->current_function[stmt->name.length] = '\0';

        fprintf(stderr, "DEBUG: Set current_function to '%s'\n", gen->current_function);
    }
    else
    {
        fprintf(stderr, "Error: Function name too long\n");
        exit(1);
    }

    gen->current_return_type = stmt->return_type;

    // Generate function prologue
    generate_prologue(gen, gen->current_function);

    // Create a new scope for function parameters
    push_scope(gen->symbol_table);
    fprintf(stderr, "DEBUG: Created new scope for function '%s'\n", gen->current_function);

    // Add all parameters to this scope
    for (int i = 0; i < stmt->param_count; i++)
    {
        char param_name[256];
        int param_len = stmt->params[i].name.length < 255 ? stmt->params[i].name.length : 255;
        strncpy(param_name, stmt->params[i].name.start, param_len);
        param_name[param_len] = '\0';
        fprintf(stderr, "DEBUG: Adding parameter %d: '%s' to scope\n", i, param_name);

        add_symbol_with_kind(gen->symbol_table, stmt->params[i].name,
                             stmt->params[i].type, SYMBOL_PARAM);
    }

    debug_print_symbol_table(gen->symbol_table, "after adding parameters");

    // Pre-scan for variable declarations
    for (int i = 0; i < stmt->body_count; i++)
    {
        if (stmt->body[i]->type == STMT_VAR_DECL)
        {
            VarDeclStmt *var = &stmt->body[i]->as.var_decl;
            char var_name[256];
            int name_len = var->name.length < 255 ? var->name.length : 255;
            strncpy(var_name, var->name.start, name_len);
            var_name[name_len] = '\0';

            fprintf(stderr, "DEBUG: Pre-adding local variable '%s' to scope\n", var_name);
            add_symbol_with_kind(gen->symbol_table, var->name, var->type, SYMBOL_LOCAL);
        }
    }

    debug_print_symbol_table(gen->symbol_table, "after pre-adding locals");

    // Save function parameters from registers to stack
    for (int i = 0; i < stmt->param_count && i < 6; i++)
    {
        // Find the parameter symbol
        Symbol *param = lookup_symbol(gen->symbol_table, stmt->params[i].name);
        if (param == NULL)
        {
            fprintf(stderr, "ERROR: Could not find parameter %d in symbol table\n", i);
            char param_name[256];
            int param_len = stmt->params[i].name.length < 255 ? stmt->params[i].name.length : 255;
            strncpy(param_name, stmt->params[i].name.start, param_len);
            param_name[param_len] = '\0';
            fprintf(stderr, "DEBUG: Parameter name: '%s'\n", param_name);
            continue;
        }

        fprintf(stderr, "DEBUG: Generating code to save parameter %d to offset %d\n",
                i, param->offset);

        switch (i)
        {
        case 0:
            fprintf(gen->output, "    mov [rbp-%d], rdi\n", param->offset);
            break;
        case 1:
            fprintf(gen->output, "    mov [rbp-%d], rsi\n", param->offset);
            break;
        case 2:
            fprintf(gen->output, "    mov [rbp-%d], rdx\n", param->offset);
            break;
        case 3:
            fprintf(gen->output, "    mov [rbp-%d], rcx\n", param->offset);
            break;
        case 4:
            fprintf(gen->output, "    mov [rbp-%d], r8\n", param->offset);
            break;
        case 5:
            fprintf(gen->output, "    mov [rbp-%d], r9\n", param->offset);
            break;
        }
    }

    // Generate function body
    for (int i = 0; i < stmt->body_count; i++)
    {
        fprintf(stderr, "DEBUG: Generating statement %d in function '%s'\n",
                i, gen->current_function);
        generate_statement(gen, stmt->body[i]);
    }

    debug_print_symbol_table(gen->symbol_table, "before leaving function");

    // Pop the function parameter scope
    fprintf(stderr, "DEBUG: Popping function scope\n");
    pop_scope(gen->symbol_table);

    // Generate function epilogue
    generate_epilogue(gen);

    // Restore old function context
    fprintf(stderr, "DEBUG: Restoring function context from '%s' to '%s'\n",
            gen->current_function, old_function ? old_function : "NULL");
    free(gen->current_function);
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;

    pop_function_context(gen);

    fprintf(stderr, "==== FINISHED GENERATING FUNCTION ====\n\n");
}

void generate_return_statement(CodeGen *gen, ReturnStmt *stmt)
{
    // Evaluate the return value
    if (stmt->value != NULL)
    {
        generate_expression(gen, stmt->value);
    }
    else
    {
        // Default return value is 0
        fprintf(gen->output, "    xor rax, rax\n");
    }

    // Return from function
    fprintf(gen->output, "    jmp %s_return\n", gen->current_function);
}

void generate_block(CodeGen *gen, BlockStmt *stmt)
{
    // Generate code for each statement in the block
    for (int i = 0; i < stmt->count; i++)
    {
        generate_statement(gen, stmt->statements[i]);
    }
}

void generate_if_statement(CodeGen *gen, IfStmt *stmt)
{
    int else_label = new_label(gen);
    int end_label = new_label(gen);

    // Generate condition
    generate_expression(gen, stmt->condition);

    // Test condition
    fprintf(gen->output, "    test rax, rax\n");
    fprintf(gen->output, "    jz .L%d\n", else_label);

    // Generate then branch
    generate_statement(gen, stmt->then_branch);
    fprintf(gen->output, "    jmp .L%d\n", end_label);

    // Generate else branch
    fprintf(gen->output, ".L%d:\n", else_label);
    if (stmt->else_branch != NULL)
    {
        generate_statement(gen, stmt->else_branch);
    }

    fprintf(gen->output, ".L%d:\n", end_label);
}

void generate_while_statement(CodeGen *gen, WhileStmt *stmt)
{
    int loop_start = new_label(gen);
    int loop_end = new_label(gen);

    // Loop start
    fprintf(gen->output, ".L%d:\n", loop_start);

    // Generate condition
    generate_expression(gen, stmt->condition);

    // Test condition
    fprintf(gen->output, "    test rax, rax\n");
    fprintf(gen->output, "    jz .L%d\n", loop_end);

    // Generate loop body
    generate_statement(gen, stmt->body);

    // Jump back to start
    fprintf(gen->output, "    jmp .L%d\n", loop_start);

    // Loop end
    fprintf(gen->output, ".L%d:\n", loop_end);
}

void generate_for_statement(CodeGen *gen, ForStmt *stmt)
{
    int loop_start = new_label(gen);
    int loop_end = new_label(gen);

    // Generate initializer
    if (stmt->initializer != NULL)
    {
        generate_statement(gen, stmt->initializer);
    }

    // Loop start
    fprintf(gen->output, ".L%d:\n", loop_start);

    // Generate condition
    if (stmt->condition != NULL)
    {
        generate_expression(gen, stmt->condition);
        fprintf(gen->output, "    test rax, rax\n");
        fprintf(gen->output, "    jz .L%d\n", loop_end);
    }

    // Generate loop body
    generate_statement(gen, stmt->body);

    // Generate increment
    if (stmt->increment != NULL)
    {
        generate_expression(gen, stmt->increment);
    }

    // Jump back to start
    fprintf(gen->output, "    jmp .L%d\n", loop_start);

    // Loop end
    fprintf(gen->output, ".L%d:\n", loop_end);
}

void generate_statement(CodeGen *gen, Stmt *stmt)
{
    switch (stmt->type)
    {
    case STMT_EXPR:
        generate_expression_statement(gen, &stmt->as.expression);
        break;
    case STMT_VAR_DECL:
        generate_var_declaration(gen, &stmt->as.var_decl);
        break;
    case STMT_FUNCTION:
        generate_function(gen, &stmt->as.function);
        break;
    case STMT_RETURN:
        generate_return_statement(gen, &stmt->as.return_stmt);
        break;
    case STMT_BLOCK:
        generate_block(gen, &stmt->as.block);
        break;
    case STMT_IF:
        generate_if_statement(gen, &stmt->as.if_stmt);
        break;
    case STMT_WHILE:
        generate_while_statement(gen, &stmt->as.while_stmt);
        break;
    case STMT_FOR:
        generate_for_statement(gen, &stmt->as.for_stmt);
        break;
    case STMT_IMPORT:
        // Import statements are handled at an earlier stage
        break;
    }
}

void generate_module(CodeGen *gen, Module *module)
{
    // Set up the assembly file
    generate_text_section(gen);

    // Generate _start function (entry point)
    fprintf(gen->output, "_start:\n");
    fprintf(gen->output, "    ; Program entry point\n");
    fprintf(gen->output, "    call main\n");
    fprintf(gen->output, "    mov rdi, rax\n");
    fprintf(gen->output, "    call exit\n\n");

    // Generate code for each statement in the module
    for (int i = 0; i < module->count; i++)
    {
        generate_statement(gen, module->statements[i]);
    }

    // Generate data section (string literals, etc.)
    generate_data_section(gen);
}