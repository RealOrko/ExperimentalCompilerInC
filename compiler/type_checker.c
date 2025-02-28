/**
 * type_checker.c
 * Implementation of the type checker
 */

 #include "type_checker.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 
 void init_type_checker(TypeChecker* checker, SymbolTable* symbol_table) {
     checker->symbol_table = symbol_table;
     checker->had_error = 0;
     checker->current_function_return_type = NULL;
     checker->in_loop = 0;
 }
 
 void type_checker_cleanup(TypeChecker* checker) {
     // No need to free symbol_table, as it's managed elsewhere
     (void)checker; // Avoid unused parameter warning
 }
 
 void error(TypeChecker* checker, const char* message) {
     fprintf(stderr, "Type Error: %s\n", message);
     checker->had_error = 1;
 }
 
 int can_convert(Type* from, Type* to) {
     if (from == NULL || to == NULL) return 0;
     if (type_equals(from, to)) return 1;
     
     // NIL can be converted to any type
     if (from->kind == TYPE_NIL) return 1;
     
     // Rules for primitive type conversions
     switch (to->kind) {
         case TYPE_INT:
             return from->kind == TYPE_CHAR || from->kind == TYPE_BOOL;
         case TYPE_LONG:
             return from->kind == TYPE_INT || from->kind == TYPE_CHAR || from->kind == TYPE_BOOL;
         case TYPE_DOUBLE:
             return from->kind == TYPE_INT || from->kind == TYPE_LONG || from->kind == TYPE_CHAR || from->kind == TYPE_BOOL;
         case TYPE_STRING:
             return from->kind == TYPE_CHAR;
         default:
             return 0;
     }
 }
 
 Type* common_type(Type* a, Type* b) {
     if (a == NULL || b == NULL) return NULL;
     if (type_equals(a, b)) return a;
     
     // If one is nil, return the other
     if (a->kind == TYPE_NIL) return b;
     if (b->kind == TYPE_NIL) return a;
     
     // Rules for finding common type
     if (a->kind == TYPE_DOUBLE || b->kind == TYPE_DOUBLE) {
         return create_primitive_type(TYPE_DOUBLE);
     } else if (a->kind == TYPE_LONG || b->kind == TYPE_LONG) {
         return create_primitive_type(TYPE_LONG);
     } else if (a->kind == TYPE_INT || b->kind == TYPE_INT) {
         return create_primitive_type(TYPE_INT);
     } else if (a->kind == TYPE_CHAR && b->kind == TYPE_CHAR) {
         return create_primitive_type(TYPE_CHAR);
     } else if (a->kind == TYPE_STRING && b->kind == TYPE_CHAR) {
         return create_primitive_type(TYPE_STRING);
     } else if (a->kind == TYPE_CHAR && b->kind == TYPE_STRING) {
         return create_primitive_type(TYPE_STRING);
     }
     
     return NULL; // No common type
 }
 
 Type* type_check_expression(TypeChecker* checker, Expr* expr) {
     if (expr == NULL) return NULL;
     
     // If the expression already has a type, return it
     if (expr->expr_type != NULL) {
         return expr->expr_type;
     }
     
     Type* type = NULL;
     
     switch (expr->type) {
         case EXPR_BINARY:
             type = type_check_binary_expression(checker, &expr->as.binary);
             break;
         case EXPR_UNARY:
             type = type_check_unary_expression(checker, &expr->as.unary);
             break;
         case EXPR_LITERAL:
             type = type_check_literal_expression(checker, &expr->as.literal);
             break;
         case EXPR_VARIABLE:
             type = type_check_variable_expression(checker, &expr->as.variable);
             break;
         case EXPR_ASSIGN:
             type = type_check_assign_expression(checker, &expr->as.assign);
             break;
         case EXPR_CALL:
             type = type_check_call_expression(checker, &expr->as.call);
             break;
         case EXPR_ARRAY:
             type = type_check_array_expression(checker, &expr->as.array);
             break;
         case EXPR_ARRAY_ACCESS:
             type = type_check_array_access_expression(checker, &expr->as.array_access);
             break;
         case EXPR_INCREMENT:
             type = type_check_increment_expression(checker, expr);
             break;
         case EXPR_DECREMENT:
             type = type_check_decrement_expression(checker, expr);
             break;
     }
     
     expr->expr_type = type;
     return type;
 }
 
 Type* type_check_binary_expression(TypeChecker* checker, BinaryExpr* expr) {
     Type* left_type = type_check_expression(checker, expr->left);
     Type* right_type = type_check_expression(checker, expr->right);
     
     if (left_type == NULL || right_type == NULL) {
         return NULL;
     }
     
     switch (expr->operator) {
         case TOKEN_PLUS:
             // Special case for string concatenation
             if (left_type->kind == TYPE_STRING || right_type->kind == TYPE_STRING) {
                 // Check if both can be converted to string
                 if (left_type->kind == TYPE_STRING || left_type->kind == TYPE_CHAR) {
                     if (right_type->kind == TYPE_STRING || right_type->kind == TYPE_CHAR) {
                         return create_primitive_type(TYPE_STRING);
                     }
                 }
                 error(checker, "Cannot concatenate these types");
                 return NULL;
             }
             
             // Arithmetic addition
             return common_type(left_type, right_type);
             
         case TOKEN_MINUS:
         case TOKEN_STAR:
         case TOKEN_SLASH:
         case TOKEN_MODULO:
             // Arithmetic operations
             if (left_type->kind == TYPE_STRING || right_type->kind == TYPE_STRING ||
                 left_type->kind == TYPE_BOOL || right_type->kind == TYPE_BOOL) {
                 error(checker, "Invalid operands for arithmetic operation");
                 return NULL;
             }
             return common_type(left_type, right_type);
             
         case TOKEN_EQUAL_EQUAL:
         case TOKEN_BANG_EQUAL:
             // Equality operators
             if (common_type(left_type, right_type) == NULL) {
                 error(checker, "Cannot compare these types");
                 return NULL;
             }
             return create_primitive_type(TYPE_BOOL);
             
         case TOKEN_LESS:
         case TOKEN_LESS_EQUAL:
         case TOKEN_GREATER:
         case TOKEN_GREATER_EQUAL:
             // Comparison operators
             if (left_type->kind == TYPE_STRING || left_type->kind == TYPE_BOOL ||
                 right_type->kind == TYPE_STRING || right_type->kind == TYPE_BOOL) {
                 error(checker, "Cannot compare these types");
                 return NULL;
             }
             return create_primitive_type(TYPE_BOOL);
             
         case TOKEN_AND:
         case TOKEN_OR:
             // Logical operators
             if (left_type->kind != TYPE_BOOL && left_type->kind != TYPE_INT &&
                 left_type->kind != TYPE_LONG && left_type->kind != TYPE_CHAR) {
                 error(checker, "Left operand of logical operator must be a boolean or numeric type");
                 return NULL;
             }
             if (right_type->kind != TYPE_BOOL && right_type->kind != TYPE_INT &&
                 right_type->kind != TYPE_LONG && right_type->kind != TYPE_CHAR) {
                 error(checker, "Right operand of logical operator must be a boolean or numeric type");
                 return NULL;
             }
             return create_primitive_type(TYPE_BOOL);
             
         default:
             error(checker, "Unknown binary operator");
             return NULL;
     }
 }
 
 Type* type_check_unary_expression(TypeChecker* checker, UnaryExpr* expr) {
     Type* operand_type = type_check_expression(checker, expr->operand);
     
     if (operand_type == NULL) {
         return NULL;
     }
     
     switch (expr->operator) {
         case TOKEN_MINUS:
             if (operand_type->kind != TYPE_INT && operand_type->kind != TYPE_LONG &&
                 operand_type->kind != TYPE_DOUBLE) {
                 error(checker, "Operand of unary '-' must be a numeric type");
                 return NULL;
             }
             return operand_type;
             
         case TOKEN_BANG:
             if (operand_type->kind != TYPE_BOOL && operand_type->kind != TYPE_INT &&
                 operand_type->kind != TYPE_LONG && operand_type->kind != TYPE_CHAR) {
                 error(checker, "Operand of unary '!' must be a boolean or numeric type");
                 return NULL;
             }
             return create_primitive_type(TYPE_BOOL);
             
         default:
             error(checker, "Unknown unary operator");
             return NULL;
     }
 }
 
 Type* type_check_literal_expression(TypeChecker* checker, LiteralExpr* expr) {
     (void)checker; // Avoid unused parameter warning
     return expr->type;
 }
 
 Type* type_check_variable_expression(TypeChecker* checker, VariableExpr* expr) {
     Symbol* symbol = lookup_symbol(checker->symbol_table, expr->name);
     
     if (symbol == NULL) {
         char name[256];
         strncpy(name, expr->name.start, expr->name.length < 255 ? expr->name.length : 255);
         name[expr->name.length < 255 ? expr->name.length : 255] = '\0';
         
         char error_msg[300];
         snprintf(error_msg, sizeof(error_msg), "Undefined variable '%s'", name);
         error(checker, error_msg);
         return NULL;
     }
     
     return symbol->type;
 }
 
 Type* type_check_assign_expression(TypeChecker* checker, AssignExpr* expr) {
     Type* var_type = NULL;
     Symbol* symbol = lookup_symbol(checker->symbol_table, expr->name);
     
     if (symbol == NULL) {
         char name[256];
         strncpy(name, expr->name.start, expr->name.length < 255 ? expr->name.length : 255);
         name[expr->name.length < 255 ? expr->name.length : 255] = '\0';
         
         char error_msg[300];
         snprintf(error_msg, sizeof(error_msg), "Undefined variable '%s'", name);
         error(checker, error_msg);
         return NULL;
     }
     
     var_type = symbol->type;
     Type* value_type = type_check_expression(checker, expr->value);
     
     if (value_type == NULL) {
         return NULL;
     }
     
     if (!can_convert(value_type, var_type)) {
         error(checker, "Cannot assign value to variable of different type");
         return NULL;
     }
     
     return var_type;
 }
 
 Type* type_check_call_expression(TypeChecker* checker, CallExpr* expr) {
     if (expr == NULL || checker == NULL) {
         return NULL;
     }
     
     Type* callee_type = type_check_expression(checker, expr->callee);
     
     if (callee_type == NULL) {
         return NULL;
     }
     
     // Special case for built-in functions
     if (expr->callee->type == EXPR_VARIABLE) {
         VariableExpr* var = &expr->callee->as.variable;
         
         if (var == NULL || var->name.start == NULL || var->name.length <= 0) {
             error(checker, "Invalid function name");
             return NULL;
         }
         
         char name[256];
         int name_len = var->name.length < 255 ? var->name.length : 255;
         strncpy(name, var->name.start, name_len);
         name[name_len] = '\0';
         
         // Handle print function
         if (strcmp(name, "print") == 0) {
             // Print accepts any type of arguments
             for (int i = 0; i < expr->arg_count; i++) {
                 // Just type check each argument but don't enforce any specific type
                 Type* arg_type = type_check_expression(checker, expr->arguments[i]);
                 (void)arg_type; // To avoid unused variable warning
             }
             return create_primitive_type(TYPE_VOID);
         }
     }
     
     // Regular function call
     if (callee_type->kind != TYPE_FUNCTION) {
         error(checker, "Cannot call non-function value");
         return NULL;
     }
     
     // Check argument count
     if (expr->arg_count != callee_type->as.function.param_count) {
         char error_msg[100];
         snprintf(error_msg, sizeof(error_msg), "Expected %d arguments but got %d",
                 callee_type->as.function.param_count, expr->arg_count);
         error(checker, error_msg);
         return NULL;
     }
     
     // Check argument types
     for (int i = 0; i < expr->arg_count; i++) {
         if (i >= callee_type->as.function.param_count) {
             break; // Avoid out-of-bounds access
         }
         
         Type* param_type = callee_type->as.function.param_types[i];
         if (param_type == NULL) {
             continue; // Skip NULL parameter types
         }
         
         Type* arg_type = type_check_expression(checker, expr->arguments[i]);
         if (arg_type == NULL) {
             continue; // Already reported error
         }
         
         // Special case for string literals passed to string parameters
         if (param_type->kind == TYPE_STRING && 
             (arg_type->kind == TYPE_STRING || 
              (expr->arguments[i]->type == EXPR_LITERAL && 
               expr->arguments[i]->as.literal.type != NULL &&
               expr->arguments[i]->as.literal.type->kind == TYPE_STRING))) {
             // String literals are compatible with string parameters
             continue;
         }
         
         // Regular type compatibility check
         if (!can_convert(arg_type, param_type)) {
             char error_msg[100];
             snprintf(error_msg, sizeof(error_msg), "Type mismatch in argument %d", i + 1);
             error(checker, error_msg);
         }
     }
     
     return callee_type->as.function.return_type;
 }
 
 Type* type_check_array_expression(TypeChecker* checker, ArrayExpr* expr) {
     if (expr->element_count == 0) {
         error(checker, "Cannot determine type of empty array");
         return NULL;
     }
     
     Type* element_type = type_check_expression(checker, expr->elements[0]);
     if (element_type == NULL) {
         return NULL;
     }
     
     // Check that all elements have compatible types
     for (int i = 1; i < expr->element_count; i++) {
         Type* type = type_check_expression(checker, expr->elements[i]);
         if (type == NULL) {
             continue; // Already reported error
         }
         
         if (!can_convert(type, element_type)) {
             error(checker, "Array elements must have compatible types");
             return NULL;
         }
     }
     
     return create_array_type(element_type);
 }
 
 Type* type_check_array_access_expression(TypeChecker* checker, ArrayAccessExpr* expr) {
     Type* array_type = type_check_expression(checker, expr->array);
     Type* index_type = type_check_expression(checker, expr->index);
     
     if (array_type == NULL || index_type == NULL) {
         return NULL;
     }
     
     if (array_type->kind != TYPE_ARRAY) {
         error(checker, "Cannot index non-array value");
         return NULL;
     }
     
     if (index_type->kind != TYPE_INT && index_type->kind != TYPE_LONG) {
         error(checker, "Array index must be an integer");
         return NULL;
     }
     
     return array_type->as.array.element_type;
 }
 
 Type* type_check_increment_expression(TypeChecker* checker, Expr* expr) {
     Type* operand_type = type_check_expression(checker, expr->as.operand);
     
     if (operand_type == NULL) {
         return NULL;
     }
     
     if (operand_type->kind != TYPE_INT && operand_type->kind != TYPE_LONG) {
         error(checker, "Cannot increment non-integer value");
         return NULL;
     }
     
     return operand_type;
 }
 
 Type* type_check_decrement_expression(TypeChecker* checker, Expr* expr) {
     Type* operand_type = type_check_expression(checker, expr->as.operand);
     
     if (operand_type == NULL) {
         return NULL;
     }
     
     if (operand_type->kind != TYPE_INT && operand_type->kind != TYPE_LONG) {
         error(checker, "Cannot decrement non-integer value");
         return NULL;
     }
     
     return operand_type;
 }
 
 void type_check_statement(TypeChecker* checker, Stmt* stmt) {
     if (stmt == NULL) return;
     
     switch (stmt->type) {
         case STMT_EXPR:
             type_check_expression_statement(checker, &stmt->as.expression);
             break;
         case STMT_VAR_DECL:
             type_check_var_declaration(checker, &stmt->as.var_decl);
             break;
         case STMT_FUNCTION:
             type_check_function(checker, &stmt->as.function);
             break;
         case STMT_RETURN:
             type_check_return_statement(checker, &stmt->as.return_stmt);
             break;
         case STMT_BLOCK:
             type_check_block(checker, &stmt->as.block);
             break;
         case STMT_IF:
             type_check_if_statement(checker, &stmt->as.if_stmt);
             break;
         case STMT_WHILE:
             type_check_while_statement(checker, &stmt->as.while_stmt);
             break;
         case STMT_FOR:
             type_check_for_statement(checker, &stmt->as.for_stmt);
             break;
         case STMT_IMPORT:
             type_check_import_statement(checker, &stmt->as.import);
             break;
     }
 }
 
 void type_check_expression_statement(TypeChecker* checker, ExprStmt* stmt) {
     type_check_expression(checker, stmt->expression);
 }
 
 void type_check_var_declaration(TypeChecker* checker, VarDeclStmt* stmt) {
     if (stmt->initializer != NULL) {
         Type* init_type = type_check_expression(checker, stmt->initializer);
         
         if (init_type != NULL && !can_convert(init_type, stmt->type)) {
             error(checker, "Cannot initialize variable with incompatible type");
         }
     }
 }
 
 void type_check_function(TypeChecker* checker, FunctionStmt* stmt) {
    // Save the current function return type
    Type* old_return_type = checker->current_function_return_type;
    checker->current_function_return_type = stmt->return_type;
    
    // Create a new scope for the function body
    push_scope(checker->symbol_table);
    
    // Add parameters to the symbol table
    for (int i = 0; i < stmt->param_count; i++) {
        add_symbol(checker->symbol_table, stmt->params[i].name, stmt->params[i].type);
    }
    
    // Type check the function body
    for (int i = 0; i < stmt->body_count; i++) {
        type_check_statement(checker, stmt->body[i]);
    }
    
    // Restore the outer scope
    pop_scope(checker->symbol_table);
    
    // Restore the previous function return type
    checker->current_function_return_type = old_return_type;
}
 
 void type_check_return_statement(TypeChecker* checker, ReturnStmt* stmt) {
     if (checker->current_function_return_type == NULL) {
         error(checker, "Return statement outside of function");
         return;
     }
     
     if (checker->current_function_return_type->kind == TYPE_VOID) {
         if (stmt->value != NULL) {
             error(checker, "Cannot return a value from void function");
         }
         return;
     }
     
     if (stmt->value == NULL) {
         error(checker, "Missing return value");
         return;
     }
     
     Type* value_type = type_check_expression(checker, stmt->value);
     
     if (value_type != NULL && !can_convert(value_type, checker->current_function_return_type)) {
         error(checker, "Return value type does not match function return type");
     }
 }
 
 void type_check_block(TypeChecker* checker, BlockStmt* stmt) {
     // Create a new scope
     push_scope(checker->symbol_table);
     
     // Type check each statement in the block
     for (int i = 0; i < stmt->count; i++) {
         type_check_statement(checker, stmt->statements[i]);
     }
     
     // Restore the outer scope
     pop_scope(checker->symbol_table);
 }
 
 void type_check_if_statement(TypeChecker* checker, IfStmt* stmt) {
     Type* condition_type = type_check_expression(checker, stmt->condition);
     
     if (condition_type != NULL && 
         condition_type->kind != TYPE_BOOL && 
         condition_type->kind != TYPE_INT && 
         condition_type->kind != TYPE_LONG &&
         condition_type->kind != TYPE_CHAR) {
         error(checker, "If condition must be a boolean or numeric type");
     }
     
     type_check_statement(checker, stmt->then_branch);
     
     if (stmt->else_branch != NULL) {
         type_check_statement(checker, stmt->else_branch);
     }
 }
 
 void type_check_while_statement(TypeChecker* checker, WhileStmt* stmt) {
     Type* condition_type = type_check_expression(checker, stmt->condition);
     
     if (condition_type != NULL && 
         condition_type->kind != TYPE_BOOL && 
         condition_type->kind != TYPE_INT && 
         condition_type->kind != TYPE_LONG &&
         condition_type->kind != TYPE_CHAR) {
         error(checker, "While condition must be a boolean or numeric type");
     }
     
     // Mark that we're in a loop
     int old_in_loop = checker->in_loop;
     checker->in_loop = 1;
     
     type_check_statement(checker, stmt->body);
     
     // Restore old loop state
     checker->in_loop = old_in_loop;
 }
 
 void type_check_for_statement(TypeChecker* checker, ForStmt* stmt) {
     // Create a new scope for the for loop
     push_scope(checker->symbol_table);
     
     // Type check initializer
     if (stmt->initializer != NULL) {
         type_check_statement(checker, stmt->initializer);
     }
     
     // Type check condition
     if (stmt->condition != NULL) {
         Type* condition_type = type_check_expression(checker, stmt->condition);
         
         if (condition_type != NULL && 
             condition_type->kind != TYPE_BOOL && 
             condition_type->kind != TYPE_INT && 
             condition_type->kind != TYPE_LONG &&
             condition_type->kind != TYPE_CHAR) {
             error(checker, "For condition must be a boolean or numeric type");
         }
     }
     
     // Type check increment
     if (stmt->increment != NULL) {
         type_check_expression(checker, stmt->increment);
     }
     
     // Mark that we're in a loop
     int old_in_loop = checker->in_loop;
     checker->in_loop = 1;
     
     // Type check body
     type_check_statement(checker, stmt->body);
     
     // Restore old loop state
     checker->in_loop = old_in_loop;
     
     // Restore the outer scope
     pop_scope(checker->symbol_table);
 }
 
 void type_check_import_statement(TypeChecker* checker, ImportStmt* stmt) {
     // Import statements are handled at an earlier stage
     // Just check that the module name is valid
     (void)checker; // Avoid unused parameter warning
     
     char name[256];
     strncpy(name, stmt->module_name.start, stmt->module_name.length < 255 ? stmt->module_name.length : 255);
     name[stmt->module_name.length < 255 ? stmt->module_name.length : 255] = '\0';
     
     // In a real compiler, we would verify the module exists
     // For simplicity, we just assume it's valid
 }
 
 int type_check_module(TypeChecker* checker, Module* module) {
     // Initialize predefined types and functions
     
     // Add built-in print function
     Token print_token;
     print_token.start = "print";
     print_token.length = 5;
     
     // Create variadic parameter list for print
     Type** param_types = malloc(sizeof(Type*));
     param_types[0] = create_primitive_type(TYPE_STRING);
     
     Type* print_type = create_function_type(create_primitive_type(TYPE_VOID), param_types, 1);
     add_symbol(checker->symbol_table, print_token, print_type);
     
     // Type check all statements in the module
     for (int i = 0; i < module->count; i++) {
         type_check_statement(checker, module->statements[i]);
     }
     
     return !checker->had_error;
 }