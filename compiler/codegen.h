#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"

#define MAX_STRINGS 100

extern int debug_enabled;

void generate_code(ASTNode* ast, FILE* output);

#endif