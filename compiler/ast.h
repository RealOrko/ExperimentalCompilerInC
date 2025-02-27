/**
 * ast.h
 * Abstract Syntax Tree node definitions
 */

 #ifndef AST_H
 #define AST_H
 
 #include "token.h"
 #include <stddef.h>
 
 // Forward declarations for circular types
 typedef struct Expr Expr;
 typedef struct Stmt Stmt;
 typedef struct Type Type;
 
 // Type definitions
 typedef enum {
     TYPE_INT,
     TYPE_LONG,
     TYPE_DOUBLE,
     TYPE_CHAR,
     TYPE_STRING,
     TYPE_BOOL,
     TYPE_VOID,
     TYPE_ARRAY,
     TYPE_FUNCTION,
     TYPE_NIL
 } TypeKind;
 
 struct Type {
     TypeKind kind;
     
     union {
         // For array types
         struct {
             Type* element_type;
         } array;
         
         // For function types
         struct {
             Type* return_type;
             Type** param_types;
             int param_count;
         } function;
     } as;
 };
 
 // Expression types
 typedef enum {
     EXPR_BINARY,
     EXPR_UNARY,
     EXPR_LITERAL,
     EXPR_VARIABLE,
     EXPR_ASSIGN,
     EXPR_CALL,
     EXPR_ARRAY,
     EXPR_ARRAY_ACCESS,
     EXPR_INCREMENT,
     EXPR_DECREMENT
 } ExprType;
 
 // Literal value union
 typedef union {
     int64_t int_value;
     double double_value;
     char char_value;
     const char* string_value;
     int bool_value;
 } LiteralValue;
 
 // Binary expression
 typedef struct {
     Expr* left;
     Expr* right;
     TokenType operator;
 } BinaryExpr;
 
 // Unary expression
 typedef struct {
     Expr* operand;
     TokenType operator;
 } UnaryExpr;
 
 // Literal expression
 typedef struct {
     LiteralValue value;
     Type* type;
 } LiteralExpr;
 
 // Variable expression
 typedef struct {
     Token name;
 } VariableExpr;
 
 // Assignment expression
 typedef struct {
     Token name;
     Expr* value;
 } AssignExpr;
 
 // Function call expression
 typedef struct {
     Expr* callee;
     Expr** arguments;
     int arg_count;
 } CallExpr;
 
 // Array expression
 typedef struct {
     Expr** elements;
     int element_count;
 } ArrayExpr;
 
 // Array access expression
 typedef struct {
     Expr* array;
     Expr* index;
 } ArrayAccessExpr;
 
 // Generic expression structure
 struct Expr {
     ExprType type;
     
     union {
         BinaryExpr binary;
         UnaryExpr unary;
         LiteralExpr literal;
         VariableExpr variable;
         AssignExpr assign;
         CallExpr call;
         ArrayExpr array;
         ArrayAccessExpr array_access;
         Expr* operand; // For increment/decrement
     } as;
     
     Type* expr_type; // Type of the expression after type checking
 };
 
 // Statement types
 typedef enum {
     STMT_EXPR,
     STMT_VAR_DECL,
     STMT_FUNCTION,
     STMT_RETURN,
     STMT_BLOCK,
     STMT_IF,
     STMT_WHILE,
     STMT_FOR,
     STMT_IMPORT
 } StmtType;
 
 // Expression statement
 typedef struct {
     Expr* expression;
 } ExprStmt;
 
 // Variable declaration
 typedef struct {
     Token name;
     Type* type;
     Expr* initializer;
 } VarDeclStmt;
 
 // Function parameter
 typedef struct {
     Token name;
     Type* type;
 } Parameter;
 
 // Function declaration
 typedef struct {
     Token name;
     Parameter* params;
     int param_count;
     Type* return_type;
     Stmt** body;
     int body_count;
 } FunctionStmt;
 
 // Return statement
 typedef struct {
     Token keyword;
     Expr* value;
 } ReturnStmt;
 
 // Block statement
 typedef struct {
     Stmt** statements;
     int count;
 } BlockStmt;
 
 // If statement
 typedef struct {
     Expr* condition;
     Stmt* then_branch;
     Stmt* else_branch;
 } IfStmt;
 
 // While statement
 typedef struct {
     Expr* condition;
     Stmt* body;
 } WhileStmt;
 
 // For statement
 typedef struct {
     Stmt* initializer;
     Expr* condition;
     Expr* increment;
     Stmt* body;
 } ForStmt;
 
 // Import statement
 typedef struct {
     Token module_name;
 } ImportStmt;
 
 // Generic statement structure
 struct Stmt {
     StmtType type;
     
     union {
         ExprStmt expression;
         VarDeclStmt var_decl;
         FunctionStmt function;
         ReturnStmt return_stmt;
         BlockStmt block;
         IfStmt if_stmt;
         WhileStmt while_stmt;
         ForStmt for_stmt;
         ImportStmt import;
     } as;
 };
 
 // AST module
 typedef struct {
     Stmt** statements;
     int count;
     int capacity;
     const char* filename;
 } Module;
 
 // Type functions
 Type* create_primitive_type(TypeKind kind);
 Type* create_array_type(Type* element_type);
 Type* create_function_type(Type* return_type, Type** param_types, int param_count);
 int type_equals(Type* a, Type* b);
 const char* type_to_string(Type* type);
 void free_type(Type* type);
 
 // Expression functions
 Expr* create_binary_expr(Expr* left, TokenType operator, Expr* right);
 Expr* create_unary_expr(TokenType operator, Expr* operand);
 Expr* create_literal_expr(LiteralValue value, Type* type);
 Expr* create_variable_expr(Token name);
 Expr* create_assign_expr(Token name, Expr* value);
 Expr* create_call_expr(Expr* callee, Expr** arguments, int arg_count);
 Expr* create_array_expr(Expr** elements, int element_count);
 Expr* create_array_access_expr(Expr* array, Expr* index);
 Expr* create_increment_expr(Expr* operand);
 Expr* create_decrement_expr(Expr* operand);
 void free_expr(Expr* expr);
 
 // Statement functions
 Stmt* create_expr_stmt(Expr* expression);
 Stmt* create_var_decl_stmt(Token name, Type* type, Expr* initializer);
 Stmt* create_function_stmt(Token name, Parameter* params, int param_count, 
                           Type* return_type, Stmt** body, int body_count);
 Stmt* create_return_stmt(Token keyword, Expr* value);
 Stmt* create_block_stmt(Stmt** statements, int count);
 Stmt* create_if_stmt(Expr* condition, Stmt* then_branch, Stmt* else_branch);
 Stmt* create_while_stmt(Expr* condition, Stmt* body);
 Stmt* create_for_stmt(Stmt* initializer, Expr* condition, Expr* increment, Stmt* body);
 Stmt* create_import_stmt(Token module_name);
 void free_stmt(Stmt* stmt);
 
 // Module functions
 void init_module(Module* module, const char* filename);
 void module_add_statement(Module* module, Stmt* stmt);
 void free_module(Module* module);
 
 #endif // AST_H