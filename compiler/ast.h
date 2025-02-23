#ifndef AST_H
#define AST_H

#include "lexer.h"

typedef enum {
    AST_IMPORT,
    AST_FUNC_DEF,
    AST_CALL,
    AST_STRING,
    AST_IDENT
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char* value;
    struct ASTNode* params;
    struct ASTNode* body;
    struct ASTNode* next;
} ASTNode;

extern int debug_enabled;

ASTNode* parse(Token* tokens, int token_count);
void free_ast(ASTNode* node);

#endif