// type_checker.h
#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "ast.h"
#include "symbol_table.h"

int type_check_module(Module *module, SymbolTable *table);

#endif