#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "ast.h"
#include "symbol_table.h"

int type_check_module(Module *module, SymbolTable *table);
Type *type_check_expr(Expr *expr, SymbolTable *table);

#endif