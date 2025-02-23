#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

static int current_token = 0;

static ASTNode* create_node(ASTNodeType type, char* value) {
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = type;
    node->value = value ? strdup(value) : NULL;
    node->params = NULL;
    node->body = NULL;
    node->next = NULL;
    return node;
}

static ASTNode* parse_call(Token* tokens, int token_count) {
    if (current_token >= token_count || tokens[current_token].type != TOK_IDENT) return NULL;

    char* name = strdup(tokens[current_token].value);
    current_token++;

    if (current_token < token_count && tokens[current_token].type == TOK_DOT) {
        current_token++;
        if (current_token < token_count && tokens[current_token].type == TOK_IDENT) {
            char* full_name = malloc(strlen(name) + strlen(tokens[current_token].value) + 2);
            sprintf(full_name, "%s.%s", name, tokens[current_token].value);
            free(name);
            name = full_name;
            current_token++;
        }
    }

    ASTNode* call = create_node(AST_CALL, name);
    if (current_token < token_count && tokens[current_token].type == TOK_LPAREN) {
        current_token++; // Skip '('
        ASTNode* arg = NULL;
        ASTNode* last_arg = NULL;
        while (current_token < token_count && tokens[current_token].type != TOK_RPAREN) {
            if (tokens[current_token].type == TOK_STRING) {
                ASTNode* new_arg = create_node(AST_STRING, tokens[current_token].value);
                if (!arg) arg = new_arg;
                else last_arg->next = new_arg;
                last_arg = new_arg;
            } else if (tokens[current_token].type == TOK_IDENT) {
                ASTNode* new_arg = create_node(AST_IDENT, tokens[current_token].value);
                if (!arg) arg = new_arg;
                else last_arg->next = new_arg;
                last_arg = new_arg;
            }
            current_token++;
        }
        if (current_token < token_count) current_token++; // Skip ')'
        call->params = arg;
    }
    return call;
}

static ASTNode* parse_stmt(Token* tokens, int token_count) {
    if (current_token >= token_count) return NULL;

    if (tokens[current_token].type == TOK_IDENT) {
        ASTNode* stmt = parse_call(tokens, token_count);
        if (stmt) {
            if (current_token < token_count && tokens[current_token].type == TOK_SEMICOLON) {
                current_token++;
            }
            return stmt;
        }
    }
    current_token++;
    return NULL;
}

static ASTNode* parse_func(Token* tokens, int token_count) {
    if (current_token >= token_count || tokens[current_token].type != TOK_FUNC) return NULL;

    current_token++; // Skip 'func'
    if (current_token >= token_count || tokens[current_token].type != TOK_IDENT) return NULL;

    ASTNode* func = create_node(AST_FUNC_DEF, tokens[current_token].value);
    current_token++;

    if (current_token < token_count && tokens[current_token].type == TOK_LPAREN) {
        current_token++; // Skip '('
        ASTNode* param = NULL;
        ASTNode* last_param = NULL;
        while (current_token < token_count && tokens[current_token].type != TOK_RPAREN) {
            if (tokens[current_token].type == TOK_IDENT) {
                ASTNode* new_param = create_node(AST_IDENT, tokens[current_token].value);
                if (!param) param = new_param;
                else last_param->next = new_param;
                last_param = new_param;
                current_token++;
                if (current_token < token_count && tokens[current_token].type == TOK_COLON) {
                    current_token += 2; // Skip ':type'
                }
            } else {
                current_token++;
            }
        }
        if (current_token < token_count) current_token++; // Skip ')'
        func->params = param;
    }

    if (current_token < token_count && tokens[current_token].type == TOK_COLON) {
        current_token++; // Skip ':'
        while (current_token < token_count && tokens[current_token].type != TOK_ARROW) current_token++;
        if (current_token < token_count) current_token++; // Skip '=>'
    }

    while (current_token < token_count && tokens[current_token].type == TOK_NEWLINE) {
        current_token++;
    }

    ASTNode* body = NULL;
    ASTNode* last_body = NULL;
    int base_indent = 0;
    if (debug_enabled) printf("Base indent for %s: %d\n", func->value, base_indent);
    while (current_token < token_count && tokens[current_token].type != TOK_EOF && 
           tokens[current_token].type != TOK_FUNC && tokens[current_token].indent_level > base_indent) {
        if (debug_enabled) printf("Processing token %d: %s (indent %d)\n", current_token, tokens[current_token].value, tokens[current_token].indent_level);
        if (tokens[current_token].type == TOK_INDENT) {
            current_token++;
            continue;
        }
        ASTNode* stmt = parse_stmt(tokens, token_count);
        if (stmt) {
            if (debug_enabled) printf("Adding statement to %s: %s\n", func->value, stmt->value);
            if (!body) body = stmt;
            else last_body->next = stmt;
            last_body = stmt;
        } else if (tokens[current_token].type == TOK_DEDENT || tokens[current_token].indent_level <= base_indent) {
            break;
        } else {
            current_token++;
        }
        while (current_token < token_count && tokens[current_token].type == TOK_NEWLINE) {
            current_token++;
        }
    }
    func->body = body;

    while (current_token < token_count && (tokens[current_token].type == TOK_DEDENT || tokens[current_token].type == TOK_NEWLINE)) {
        current_token++;
    }

    if (debug_enabled) printf("Parsed function: %s\n", func->value);
    return func;
}

ASTNode* parse(Token* tokens, int token_count) {
    ASTNode* root = NULL;
    ASTNode* last = NULL;
    current_token = 0;

    while (current_token < token_count && tokens[current_token].type != TOK_EOF) {
        if (tokens[current_token].type == TOK_IMPORT) {
            current_token++;
            if (current_token < token_count && tokens[current_token].type == TOK_IDENT) {
                ASTNode* import = create_node(AST_IMPORT, tokens[current_token].value);
                if (!root) root = import;
                else last->next = import;
                last = import;
                current_token++;
            }
            while (current_token < token_count && tokens[current_token].type != TOK_NEWLINE) current_token++;
            if (current_token < token_count) current_token++;
        } else if (tokens[current_token].type == TOK_FUNC) {
            ASTNode* func = parse_func(tokens, token_count);
            if (func) {
                if (!root) root = func;
                else last->next = func;
                last = func;
            }
        } else {
            current_token++;
        }
    }
    return root;
}

void free_ast(ASTNode* node) {
    while (node) {
        ASTNode* next = node->next;
        free(node->value);
        free_ast(node->params);
        free_ast(node->body);
        free(node);
        node = next;
    }
}