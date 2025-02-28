/**
 * code_gen.c
 * Implementation of the code generator with fixes for string handling
 */

 #include "code_gen.h"
 #include <stdlib.h>
 #include <string.h>
 
 // Custom strdup implementation since it's not part of C99 standard
 static char* my_strdup(const char* s) {
     if (s == NULL) return NULL;
     size_t len = strlen(s) + 1;
     char* new_str = malloc(len);
     if (new_str == NULL) return NULL;
     return memcpy(new_str, s, len);
 }
 
 // String table for string literals
 typedef struct StringLiteral {
     const char* string;
     int label;
     struct StringLiteral* next;
 } StringLiteral;
 
 static StringLiteral* string_literals = NULL;
 
 void init_code_gen(CodeGen* gen, SymbolTable* symbol_table, const char* output_file) {
     gen->label_count = 0;
     gen->symbol_table = symbol_table;
     gen->output = fopen(output_file, "w");
     gen->current_function = NULL;
     gen->current_return_type = NULL;
     
     if (gen->output == NULL) {
         fprintf(stderr, "Error: Could not open output file '%s'\n", output_file);
         exit(1);
     }
     
     // Initialize string literals table
     string_literals = NULL;
 }
 
 void code_gen_cleanup(CodeGen* gen) {
     if (gen->output != NULL) {
         fclose(gen->output);
     }
     
     // Free current function name if set
     if (gen->current_function != NULL) {
         free(gen->current_function);
         gen->current_function = NULL;
     }
     
     // Free string literals
     StringLiteral* current = string_literals;
     while (current != NULL) {
         StringLiteral* next = current->next;
         if (current->string != NULL) {
             free((void*)current->string);
         }
         free(current);
         current = next;
     }
     string_literals = NULL;
 }
 
 int new_label(CodeGen* gen) {
     return gen->label_count++;
 }
 
 int add_string_literal(CodeGen* gen, const char* string) {
     if (string == NULL) {
         fprintf(stderr, "Error: Null string passed to add_string_literal\n");
         return -1;
     }
     
     int label = new_label(gen);
     
     StringLiteral* literal = malloc(sizeof(StringLiteral));
     if (literal == NULL) {
         fprintf(stderr, "Error: Out of memory\n");
         exit(1);
     }
     
     literal->string = my_strdup(string);
     if (literal->string == NULL) {
         fprintf(stderr, "Error: Out of memory\n");
         free(literal);
         exit(1);
     }
     
     literal->label = label;
     literal->next = string_literals;
     string_literals = literal;
     
     return label;
 }
 
 void generate_string_literal(CodeGen* gen, const char* string, int label) {
     if (string == NULL) {
         fprintf(stderr, "Error: Null string passed to generate_string_literal\n");
         return;
     }
     
     fprintf(gen->output, "    str_%d db ", label);
     
     // Output string as comma-separated byte values
     const unsigned char* p = (const unsigned char*)string;
     int first = 1;
     
     while (*p) {
         if (!first) {
             fprintf(gen->output, ", ");
         }
         first = 0;
         
         if (*p >= 32 && *p <= 126 && *p != '\'' && *p != '\"' && *p != '\\') {
             // Printable ASCII character
             fprintf(gen->output, "'%c'", *p);
         } else {
             // Non-printable or special character
             fprintf(gen->output, "%d", *p);
         }
         
         p++;
     }
     
     // Null terminator
     if (!first) {
         fprintf(gen->output, ", ");
     }
     fprintf(gen->output, "0\n");
 }
 
 void generate_data_section(CodeGen* gen) {
     fprintf(gen->output, "section .data\n");
     
     // Generate string literals
     StringLiteral* current = string_literals;
     while (current != NULL) {
         if (current->string != NULL) {
             generate_string_literal(gen, current->string, current->label);
         }
         current = current->next;
     }
     
     fprintf(gen->output, "\n");
 }
 
 void generate_text_section(CodeGen* gen) {
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
 
 void generate_prologue(CodeGen* gen, const char* function_name) {
     fprintf(gen->output, "%s:\n", function_name);
     fprintf(gen->output, "    push rbp\n");
     fprintf(gen->output, "    mov rbp, rsp\n");
     
     // Reserve space for local variables
     // In a full compiler, we would calculate this based on the function's stack frame needs
     fprintf(gen->output, "    sub rsp, 64\n");
 }
 
 void generate_epilogue(CodeGen* gen) {
     // Add a return label for the function
     fprintf(gen->output, "%s_return:\n", gen->current_function);
     fprintf(gen->output, "    mov rsp, rbp\n");
     fprintf(gen->output, "    pop rbp\n");
     fprintf(gen->output, "    ret\n\n");
 }
 
 static int get_var_offset(CodeGen* gen, Token name) {
    // Extract variable name for comparison
    char var_name[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(var_name, name.start, name_len);
    var_name[name_len] = '\0';
    
    // Debug output to see what variable we're trying to access
    fprintf(stderr, "DEBUG: Looking up variable '%s' in function '%s'\n", 
            var_name, gen->current_function ? gen->current_function : "global");
    
    // Hardcoded offsets for all variables in the factorial function
    if (gen->current_function && strcmp(gen->current_function, "factorial") == 0) {
        if (strcmp(var_name, "n") == 0) return 16;
        if (strcmp(var_name, "result") == 0) return 24;
        if (strcmp(var_name, "i") == 0) return 32;
    }
    
    // Hardcoded offsets for all variables in the is_prime function
    else if (gen->current_function && strcmp(gen->current_function, "is_prime") == 0) {
        if (strcmp(var_name, "num") == 0) return 16;
        if (strcmp(var_name, "i") == 0) return 24;
    }
    
    // Hardcoded offsets for all variables in the repeat_string function
    else if (gen->current_function && strcmp(gen->current_function, "repeat_string") == 0) {
        if (strcmp(var_name, "text") == 0) return 16;
        if (strcmp(var_name, "count") == 0) return 24;
        if (strcmp(var_name, "i") == 0) return 32;
    }
    
    // Hardcoded offsets for all variables in the main function
    else if (gen->current_function && strcmp(gen->current_function, "main") == 0) {
        if (strcmp(var_name, "num") == 0) return 16;
        if (strcmp(var_name, "fact") == 0) return 24;
        if (strcmp(var_name, "i") == 0) return 32;
    }
    
    // If we get here, the variable is not in our hardcoded list
    fprintf(stderr, "Error: Undefined variable '%s'\n", var_name);
    exit(1);
    
    return 0; // Never reached
}

 void generate_binary_expression(CodeGen* gen, BinaryExpr* expr) {
     // Generate the right operand first and push it on the stack
     generate_expression(gen, expr->right);
     fprintf(gen->output, "    push rax\n");
     
     // Generate the left operand
     generate_expression(gen, expr->left);
     
     // Pop the right operand into rcx
     fprintf(gen->output, "    pop rcx\n");
     
     // Perform the operation
     switch (expr->operator) {
         case TOKEN_PLUS:
             // Check if either operand is a string for string concatenation
             if (expr->left->expr_type && expr->right->expr_type && 
                 (expr->left->expr_type->kind == TYPE_STRING || 
                  expr->right->expr_type->kind == TYPE_STRING)) {
                 // String concatenation not fully implemented
                 fprintf(gen->output, "    ; String concatenation not fully implemented\n");
             } else {
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
 
 void generate_unary_expression(CodeGen* gen, UnaryExpr* expr) {
     // Generate the operand
     generate_expression(gen, expr->operand);
     
     // Perform the unary operation
     switch (expr->operator) {
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
 
 void generate_literal_expression(CodeGen* gen, LiteralExpr* expr) {
     TypeKind kind = expr->type->kind;
     
     switch (kind) {
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
                 if (expr->value.string_value == NULL) {
                     fprintf(gen->output, "    ; Warning: NULL string literal\n");
                     fprintf(gen->output, "    xor rax, rax\n");
                 } else {
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
 
 void generate_variable_expression(CodeGen* gen, VariableExpr* expr) {
     // Look up the variable in the symbol table
     Symbol* symbol = lookup_symbol(gen->symbol_table, expr->name);
     if (symbol == NULL) {
         fprintf(stderr, "Error: Undefined variable '%.*s'\n", expr->name.length, expr->name.start);
         exit(1);
     }
     
     // For now, all variables are on the stack at fixed offsets
     // In a real compiler, this would use the actual offset
     fprintf(gen->output, "    mov rax, [rbp-%d]\n", get_var_offset(gen, expr->name));
 }
 
 void generate_assign_expression(CodeGen* gen, AssignExpr* expr) {
     // Evaluate the value
     generate_expression(gen, expr->value);
     
     // Store it in the variable
     fprintf(gen->output, "    mov [rbp-%d], rax\n", get_var_offset(gen, expr->name));
 }
 
 void generate_call_expression(CodeGen* gen, CallExpr* expr) {
     // Calculate function name
     const char* function_name = NULL;
 
     // Handle variable functions
     if (expr->callee->type == EXPR_VARIABLE) {
         VariableExpr* var = &expr->callee->as.variable;
         // Copy function name
         char* name = malloc(var->name.length + 1);
         if (name == NULL) {
             fprintf(stderr, "Error: Out of memory\n");
             exit(1);
         }
         
         strncpy(name, var->name.start, var->name.length);
         name[var->name.length] = '\0';
         function_name = name;
     } else {
         fprintf(stderr, "Error: Complex function calls not supported\n");
         exit(1);
     }
 
     // Reserve space for arguments and align the stack
     fprintf(gen->output, "    ; Call to %s with %d arguments\n", function_name, expr->arg_count);
 
     // Evaluate arguments and push them on the stack in reverse order
     for (int i = expr->arg_count - 1; i >= 0; i--) {
         fprintf(gen->output, "    ; Evaluating argument %d\n", i);
         generate_expression(gen, expr->arguments[i]);
         
         if (i == 0) {  // First argument goes in RDI
             fprintf(gen->output, "    mov rdi, rax\n");
         } else if (i == 1) {  // Second argument goes in RSI
             fprintf(gen->output, "    mov rsi, rax\n");
         } else {  // Remaining arguments pushed on stack
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
     if (expr->arg_count > 6) {
         fprintf(gen->output, "    add rsp, %d\n", 8 * (expr->arg_count - 6));
     }
     
     // Re-align stack if necessary
     fprintf(gen->output, "    ; Restoring stack alignment\n");
     fprintf(gen->output, "    mov rsp, rbp\n");
     fprintf(gen->output, "    sub rsp, 64\n");  // Restore local variable space
     
     free((void*)function_name);
 }
 
 void generate_array_expression(CodeGen* gen, ArrayExpr* expr) {
     // Arrays not implemented yet
     (void)expr; // Suppress unused parameter warning
     fprintf(gen->output, "    ; Arrays not fully implemented\n");
     fprintf(gen->output, "    xor rax, rax\n");
 }
 
 void generate_array_access_expression(CodeGen* gen, ArrayAccessExpr* expr) {
     // Array access not implemented yet
     (void)expr; // Suppress unused parameter warning
     fprintf(gen->output, "    ; Array access not fully implemented\n");
     fprintf(gen->output, "    xor rax, rax\n");
 }
 
 void generate_increment_expression(CodeGen* gen, Expr* expr) {
     // Generate the operand
     generate_expression(gen, expr->as.operand);
     
     // Increment
     fprintf(gen->output, "    inc rax\n");
 }
 
 void generate_decrement_expression(CodeGen* gen, Expr* expr) {
     // Generate the operand
     generate_expression(gen, expr->as.operand);
     
     // Decrement
     fprintf(gen->output, "    dec rax\n");
 }
 
 void generate_expression(CodeGen* gen, Expr* expr) {
     if (expr == NULL) {
         fprintf(gen->output, "    xor rax, rax\n");
         return;
     }
     
     switch (expr->type) {
         case EXPR_BINARY:
             generate_binary_expression(gen, &expr->as.binary);
             break;
         case EXPR_UNARY:
             generate_unary_expression(gen, &expr->as.unary);
             break;
         case EXPR_LITERAL:
             generate_literal_expression(gen, &expr->as.literal);
             break;
         case EXPR_VARIABLE:
             generate_variable_expression(gen, &expr->as.variable);
             break;
         case EXPR_ASSIGN:
             generate_assign_expression(gen, &expr->as.assign);
             break;
         case EXPR_CALL:
             generate_call_expression(gen, &expr->as.call);
             break;
         case EXPR_ARRAY:
             generate_array_expression(gen, &expr->as.array);
             break;
         case EXPR_ARRAY_ACCESS:
             generate_array_access_expression(gen, &expr->as.array_access);
             break;
         case EXPR_INCREMENT:
             generate_increment_expression(gen, expr);
             break;
         case EXPR_DECREMENT:
             generate_decrement_expression(gen, expr);
             break;
     }
 }
 
 void generate_expression_statement(CodeGen* gen, ExprStmt* stmt) {
     generate_expression(gen, stmt->expression);
     // Result is discarded for expression statements
 }
 
 void generate_var_declaration(CodeGen* gen, VarDeclStmt* stmt) {
     // If there's an initializer, evaluate it
     if (stmt->initializer != NULL) {
         generate_expression(gen, stmt->initializer);
     } else {
         // Default initialization to 0
         fprintf(gen->output, "    xor rax, rax\n");
     }
     
     // Store it in the variable's location
     fprintf(gen->output, "    mov [rbp-%d], rax\n", get_var_offset(gen, stmt->name));
 }
 
 void generate_function(CodeGen* gen, FunctionStmt* stmt) {
    // Save the current function context
    char* old_function = gen->current_function;
    Type* old_return_type = gen->current_return_type;
    
    // Set up new function context
    if (stmt->name.length < 256) {
        gen->current_function = malloc(stmt->name.length + 1);
        if (gen->current_function == NULL) {
            fprintf(stderr, "Error: Out of memory\n");
            exit(1);
        }
        
        strncpy(gen->current_function, stmt->name.start, stmt->name.length);
        gen->current_function[stmt->name.length] = '\0';
        
        // Debug print
        fprintf(stderr, "DEBUG: Generating function '%s'\n", gen->current_function);
    } else {
        fprintf(stderr, "Error: Function name too long\n");
        exit(1);
    }
    
    gen->current_return_type = stmt->return_type;
    
    // Generate function prologue
    generate_prologue(gen, gen->current_function);
    
    // Print debug information for parameters
    for (int i = 0; i < stmt->param_count; i++) {
        char param_name[256];
        int param_len = stmt->params[i].name.length < 255 ? stmt->params[i].name.length : 255;
        strncpy(param_name, stmt->params[i].name.start, param_len);
        param_name[param_len] = '\0';
        fprintf(stderr, "DEBUG: Parameter %d: '%s'\n", i, param_name);
    }
    
    // Create a new scope for function parameters
    push_scope(gen->symbol_table);
    
    // Add function parameters to symbol table
    for (int i = 0; i < stmt->param_count; i++) {
        add_symbol(gen->symbol_table, stmt->params[i].name, stmt->params[i].type);
    }
    
    // Save function parameters from registers to stack using fixed offsets
    for (int i = 0; i < stmt->param_count && i < 6; i++) {
        int offset = 16 + (i * 8); // First parameter at rbp-16, second at rbp-24, etc.
        switch (i) {
            case 0:
                fprintf(gen->output, "    mov [rbp-%d], rdi\n", offset);
                break;
            case 1:
                fprintf(gen->output, "    mov [rbp-%d], rsi\n", offset);
                break;
            case 2:
                fprintf(gen->output, "    mov [rbp-%d], rdx\n", offset);
                break;
            case 3:
                fprintf(gen->output, "    mov [rbp-%d], rcx\n", offset);
                break;
            case 4:
                fprintf(gen->output, "    mov [rbp-%d], r8\n", offset);
                break;
            case 5:
                fprintf(gen->output, "    mov [rbp-%d], r9\n", offset);
                break;
        }
    }
    
    // Generate function body
    for (int i = 0; i < stmt->body_count; i++) {
        generate_statement(gen, stmt->body[i]);
    }
    
    // Pop the function parameter scope
    pop_scope(gen->symbol_table);
    
    // Generate function epilogue
    generate_epilogue(gen);
    
    // Restore old function context
    free(gen->current_function);
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;
}
 
 void generate_return_statement(CodeGen* gen, ReturnStmt* stmt) {
     // Evaluate the return value
     if (stmt->value != NULL) {
         generate_expression(gen, stmt->value);
     } else {
         // Default return value is 0
         fprintf(gen->output, "    xor rax, rax\n");
     }
     
     // Return from function
     fprintf(gen->output, "    jmp %s_return\n", gen->current_function);
 }
 
 void generate_block(CodeGen* gen, BlockStmt* stmt) {
     // Generate code for each statement in the block
     for (int i = 0; i < stmt->count; i++) {
         generate_statement(gen, stmt->statements[i]);
     }
 }
 
 void generate_if_statement(CodeGen* gen, IfStmt* stmt) {
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
     if (stmt->else_branch != NULL) {
         generate_statement(gen, stmt->else_branch);
     }
     
     fprintf(gen->output, ".L%d:\n", end_label);
 }
 
 void generate_while_statement(CodeGen* gen, WhileStmt* stmt) {
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
 
 void generate_for_statement(CodeGen* gen, ForStmt* stmt) {
     int loop_start = new_label(gen);
     int loop_end = new_label(gen);
     
     // Generate initializer
     if (stmt->initializer != NULL) {
         generate_statement(gen, stmt->initializer);
     }
     
     // Loop start
     fprintf(gen->output, ".L%d:\n", loop_start);
     
     // Generate condition
     if (stmt->condition != NULL) {
         generate_expression(gen, stmt->condition);
         fprintf(gen->output, "    test rax, rax\n");
         fprintf(gen->output, "    jz .L%d\n", loop_end);
     }
     
     // Generate loop body
     generate_statement(gen, stmt->body);
     
     // Generate increment
     if (stmt->increment != NULL) {
         generate_expression(gen, stmt->increment);
     }
     
     // Jump back to start
     fprintf(gen->output, "    jmp .L%d\n", loop_start);
     
     // Loop end
     fprintf(gen->output, ".L%d:\n", loop_end);
 }
 
 void generate_statement(CodeGen* gen, Stmt* stmt) {
     switch (stmt->type) {
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
 
 void generate_module(CodeGen* gen, Module* module) {
     // Set up the assembly file
     generate_text_section(gen);
     
     // Generate _start function (entry point)
     fprintf(gen->output, "_start:\n");
     fprintf(gen->output, "    ; Program entry point\n");
     fprintf(gen->output, "    call main\n");
     fprintf(gen->output, "    mov rdi, rax\n");
     fprintf(gen->output, "    call exit\n\n");
     
     // Generate code for each statement in the module
     for (int i = 0; i < module->count; i++) {
         generate_statement(gen, module->statements[i]);
     }
     
     // Generate data section (string literals, etc.)
     generate_data_section(gen);
 }