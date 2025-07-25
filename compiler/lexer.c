#include "lexer.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char error_buffer[128];

void lexer_init(Lexer *lexer, const char *source, const char *filename)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->filename = filename;
    lexer->indent_capacity = 8;
    lexer->indent_stack = malloc(sizeof(int) * lexer->indent_capacity);
    lexer->indent_size = 1;
    lexer->indent_stack[0] = 0;
    lexer->at_line_start = 1;
}

void lexer_cleanup(Lexer *lexer)
{
    if (lexer->indent_stack != NULL)
    {
        free(lexer->indent_stack);
        lexer->indent_stack = NULL;
    }
}

void lexer_report_indentation_error(Lexer *lexer, int expected, int actual)
{
    snprintf(error_buffer, sizeof(error_buffer), "Indentation error: expected %d spaces, got %d spaces",
             expected, actual);
    lexer_error_token(lexer, error_buffer);
}

int lexer_is_at_end(Lexer *lexer)
{
    return *lexer->current == '\0';
}

char lexer_advance(Lexer *lexer)
{
    return *lexer->current++;
}

char lexer_peek(Lexer *lexer)
{
    return *lexer->current;
}

char lexer_peek_next(Lexer *lexer)
{
    if (lexer_is_at_end(lexer))
        return '\0';
    return lexer->current[1];
}

int lexer_match(Lexer *lexer, char expected)
{
    if (lexer_is_at_end(lexer))
        return 0;
    if (*lexer->current != expected)
        return 0;
    lexer->current++;
    return 1;
}

Token lexer_make_token(Lexer *lexer, TokenType type)
{
    int length = (int)(lexer->current - lexer->start);
    char *dup_start = strndup(lexer->start, length);
    if (dup_start == NULL) {
        DEBUG_ERROR("Out of memory duplicating lexeme");
        exit(1);
    }
    Token token;
    token_init(&token, type, dup_start, length, lexer->line);
    return token;
}

Token lexer_error_token(Lexer *lexer, const char *message)
{
    char *dup_message = strdup(message);
    if (dup_message == NULL) {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    Token token;
    token_init(&token, TOKEN_ERROR, dup_message, (int)strlen(dup_message), lexer->line);
    return token;
}

void lexer_skip_whitespace(Lexer *lexer)
{
    for (;;)
    {
        char c = lexer_peek(lexer);
        switch (c)
        {
        case ' ':
        case '\t':
        case '\r':
            lexer_advance(lexer);
            break;
        case '\n':
            return;
        case '/':
            if (lexer_peek_next(lexer) == '/')
            {
                while (lexer_peek(lexer) != '\n' && !lexer_is_at_end(lexer))
                {
                    lexer_advance(lexer);
                }
            }
            else
            {
                return;
            }
            break;
        default:
            return;
        }
    }
}

TokenType lexer_check_keyword(Lexer *lexer, int start, int length, const char *rest, TokenType type)
{
    int lexeme_length = (int)(lexer->current - lexer->start);
    if (lexeme_length == start + length &&
        memcmp(lexer->start + start, rest, length) == 0)
    {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

TokenType lexer_identifier_type(Lexer *lexer)
{
    switch (lexer->start[0])
    {
    case 'b':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                return lexer_check_keyword(lexer, 2, 2, "ol", TOKEN_BOOL);
            }
        }
        break;
    case 'c':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'h':
                return lexer_check_keyword(lexer, 2, 2, "ar", TOKEN_CHAR);
            }
        }
        break;
    case 'd':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'o':
                return lexer_check_keyword(lexer, 2, 4, "uble", TOKEN_DOUBLE);
            }
        }
        break;
    case 'e':
        return lexer_check_keyword(lexer, 1, 3, "lse", TOKEN_ELSE);
    case 'f':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                return lexer_check_keyword(lexer, 2, 3, "lse", TOKEN_BOOL_LITERAL);
            case 'n':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_FN);
            case 'o':
                return lexer_check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
            }
        }
        break;
    case 'i':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'f':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_IF);
            case 'm':
                return lexer_check_keyword(lexer, 2, 4, "port", TOKEN_IMPORT);
            case 'n':
                return lexer_check_keyword(lexer, 2, 1, "t", TOKEN_INT);
            }
        }
        break;
    case 'l':
        return lexer_check_keyword(lexer, 1, 3, "ong", TOKEN_LONG);
    case 'n':
        return lexer_check_keyword(lexer, 1, 2, "il", TOKEN_NIL);
    case 'r':
        return lexer_check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
    case 's':
        return lexer_check_keyword(lexer, 1, 2, "tr", TOKEN_STR);
    case 't':
        return lexer_check_keyword(lexer, 1, 3, "rue", TOKEN_BOOL_LITERAL);
    case 'v':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'a':
                return lexer_check_keyword(lexer, 2, 1, "r", TOKEN_VAR);
            case 'o':
                return lexer_check_keyword(lexer, 2, 2, "id", TOKEN_VOID);
            }
        }
        break;
    case 'w':
        return lexer_check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
    }
    return TOKEN_IDENTIFIER;
}

Token lexer_scan_identifier(Lexer *lexer)
{
    while (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_')
    {
        lexer_advance(lexer);
    }
    TokenType type = lexer_identifier_type(lexer);
    if (type == TOKEN_BOOL_LITERAL)
    {
        Token token = lexer_make_token(lexer, type);
        if (memcmp(lexer->start, "true", 4) == 0)
        {
            token_set_bool_literal(&token, 1);
        }
        else
        {
            token_set_bool_literal(&token, 0);
        }
        return token;
    }
    return lexer_make_token(lexer, type);
}

Token lexer_scan_number(Lexer *lexer)
{
    while (isdigit(lexer_peek(lexer)))
    {
        lexer_advance(lexer);
    }
    if (lexer_peek(lexer) == '.' && isdigit(lexer_peek_next(lexer)))
    {
        lexer_advance(lexer);
        while (isdigit(lexer_peek(lexer)))
        {
            lexer_advance(lexer);
        }
        if (lexer_peek(lexer) == 'd')
        {
            lexer_advance(lexer);
            Token token = lexer_make_token(lexer, TOKEN_DOUBLE_LITERAL);
            char buffer[256];
            int length = (int)(lexer->current - lexer->start - 1);
            if (length >= (int)sizeof(buffer))
            {
                snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
                return lexer_error_token(lexer, error_buffer);
            }
            strncpy(buffer, lexer->start, length);
            buffer[length] = '\0';
            double value = strtod(buffer, NULL);
            token_set_double_literal(&token, value);
            return token;
        }
        Token token = lexer_make_token(lexer, TOKEN_DOUBLE_LITERAL);
        char buffer[256];
        int length = (int)(lexer->current - lexer->start);
        if (length >= (int)sizeof(buffer))
        {
            snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
            return lexer_error_token(lexer, error_buffer);
        }
        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';
        double value = strtod(buffer, NULL);
        token_set_double_literal(&token, value);
        return token;
    }
    if (lexer_peek(lexer) == 'l')
    {
        lexer_advance(lexer);
        Token token = lexer_make_token(lexer, TOKEN_LONG_LITERAL);
        char buffer[256];
        int length = (int)(lexer->current - lexer->start - 1);
        if (length >= (int)sizeof(buffer))
        {
            snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
            return lexer_error_token(lexer, error_buffer);
        }
        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';
        int64_t value = strtoll(buffer, NULL, 10);
        token_set_int_literal(&token, value);
        return token;
    }
    Token token = lexer_make_token(lexer, TOKEN_INT_LITERAL);
    char buffer[256];
    int length = (int)(lexer->current - lexer->start);
    if (length >= (int)sizeof(buffer))
    {
        snprintf(error_buffer, sizeof(error_buffer), "Number literal too long");
        return lexer_error_token(lexer, error_buffer);
    }
    strncpy(buffer, lexer->start, length);
    buffer[length] = '\0';
    int64_t value = strtoll(buffer, NULL, 10);
    token_set_int_literal(&token, value);
    return token;
}

Token lexer_scan_string(Lexer *lexer)
{
    int buffer_size = 256;
    char *buffer = malloc(buffer_size);
    if (buffer == NULL)
    {
        snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
        return lexer_error_token(lexer, error_buffer);
    }
    int buffer_index = 0;
    while (lexer_peek(lexer) != '"' && !lexer_is_at_end(lexer))
    {
        if (lexer_peek(lexer) == '\n')
        {
            lexer->line++;
        }
        if (lexer_peek(lexer) == '\\')
        {
            lexer_advance(lexer);
            switch (lexer_peek(lexer))
            {
            case '\\':
                buffer[buffer_index++] = '\\';
                break;
            case 'n':
                buffer[buffer_index++] = '\n';
                break;
            case 'r':
                buffer[buffer_index++] = '\r';
                break;
            case 't':
                buffer[buffer_index++] = '\t';
                break;
            case '"':
                buffer[buffer_index++] = '"';
                break;
            default:
                free(buffer);
                snprintf(error_buffer, sizeof(error_buffer), "Invalid escape sequence");
                return lexer_error_token(lexer, error_buffer);
            }
        }
        else
        {
            buffer[buffer_index++] = lexer_peek(lexer);
        }
        lexer_advance(lexer);
        if (buffer_index >= buffer_size - 1)
        {
            buffer_size *= 2;
            char *new_buffer = realloc(buffer, buffer_size);
            if (new_buffer == NULL)
            {
                free(buffer);
                snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
                return lexer_error_token(lexer, error_buffer);
            }
            buffer = new_buffer;
        }
    }
    if (lexer_is_at_end(lexer))
    {
        free(buffer);
        snprintf(error_buffer, sizeof(error_buffer), "Unterminated string");
        return lexer_error_token(lexer, error_buffer);
    }
    lexer_advance(lexer);
    buffer[buffer_index] = '\0';
    Token token = lexer_make_token(lexer, TOKEN_STRING_LITERAL);
    char *str_copy = malloc(buffer_index + 1);
    if (str_copy == NULL)
    {
        free(buffer);
        snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
        return lexer_error_token(lexer, error_buffer);
    }
    strcpy(str_copy, buffer);
    token_set_string_literal(&token, str_copy);
    free(buffer);
    return token;
}

Token lexer_scan_char(Lexer *lexer)
{
    char value = '\0';
    if (lexer_peek(lexer) == '\\')
    {
        lexer_advance(lexer);
        switch (lexer_peek(lexer))
        {
        case '\\':
            value = '\\';
            break;
        case 'n':
            value = '\n';
            break;
        case 'r':
            value = '\r';
            break;
        case 't':
            value = '\t';
            break;
        case '\'':
            value = '\'';
            break;
        default:
            snprintf(error_buffer, sizeof(error_buffer), "Invalid escape sequence");
            return lexer_error_token(lexer, error_buffer);
        }
    }
    else if (lexer_peek(lexer) == '\'')
    {
        snprintf(error_buffer, sizeof(error_buffer), "Empty character literal");
        return lexer_error_token(lexer, error_buffer);
    }
    else
    {
        value = lexer_peek(lexer);
    }
    lexer_advance(lexer);
    if (lexer_peek(lexer) != '\'')
    {
        snprintf(error_buffer, sizeof(error_buffer), "Unterminated character literal");
        return lexer_error_token(lexer, error_buffer);
    }
    lexer_advance(lexer);
    Token token = lexer_make_token(lexer, TOKEN_CHAR_LITERAL);
    token_set_char_literal(&token, value);
    return token;
}

Token lexer_scan_token(Lexer *lexer)
{
    DEBUG_VERBOSE("Line %d: Starting lexer_scan_token, at_line_start = %d", lexer->line, lexer->at_line_start);
    if (lexer->at_line_start)
    {
        int current_indent = 0;
        const char *indent_start = lexer->current;
        while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t')
        {
            current_indent++;
            lexer_advance(lexer);
        }
        DEBUG_VERBOSE("Line %d: Calculated indent = %d", lexer->line, current_indent);
        const char *temp = lexer->current;
        while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t')
        {
            lexer_advance(lexer);
        }
        if (lexer_is_at_end(lexer) || lexer_peek(lexer) == '\n' ||
            (lexer_peek(lexer) == '/' && lexer_peek_next(lexer) == '/'))
        {
            DEBUG_VERBOSE("Line %d: Ignoring line (whitespace or comment only)", lexer->line);
            lexer->current = indent_start;
            lexer->start = indent_start;
        }
        else
        {
            lexer->current = temp;
            lexer->start = lexer->current;
            int top = lexer->indent_stack[lexer->indent_size - 1];
            DEBUG_VERBOSE("Line %d: Top of indent_stack = %d, indent_size = %d",
                          lexer->line, top, lexer->indent_size);
            if (current_indent > top)
            {
                if (lexer->indent_size >= lexer->indent_capacity)
                {
                    lexer->indent_capacity *= 2;
                    lexer->indent_stack = realloc(lexer->indent_stack,
                                                  lexer->indent_capacity * sizeof(int));
                    if (lexer->indent_stack == NULL)
                    {
                        DEBUG_ERROR("Out of memory");
                        exit(1);
                    }
                    DEBUG_VERBOSE("Line %d: Resized indent_stack, new capacity = %d",
                                  lexer->line, lexer->indent_capacity);
                }
                lexer->indent_stack[lexer->indent_size++] = current_indent;
                lexer->at_line_start = 0;
                DEBUG_VERBOSE("Line %d: Pushing indent level %d, emitting INDENT",
                              lexer->line, current_indent);
                return lexer_make_token(lexer, TOKEN_INDENT);
            }
            else if (current_indent < top)
            {
                lexer->indent_size--;
                int new_top = lexer->indent_stack[lexer->indent_size - 1];
                DEBUG_VERBOSE("Line %d: Popped indent level, new top = %d, indent_size = %d",
                              lexer->line, new_top, lexer->indent_size);
                if (current_indent == new_top)
                {
                    lexer->at_line_start = 0;
                    DEBUG_VERBOSE("Line %d: Emitting DEDENT, indentation matches stack",
                                  lexer->line);
                }
                else if (current_indent > new_top)
                {
                    DEBUG_VERBOSE("Line %d: Error - Inconsistent indentation (current %d > new_top %d)",
                                  lexer->line, current_indent, new_top);
                    snprintf(error_buffer, sizeof(error_buffer), "Inconsistent indentation");
                    return lexer_error_token(lexer, error_buffer);
                }
                else
                {
                    DEBUG_VERBOSE("Line %d: Emitting DEDENT, more dedents pending",
                                  lexer->line);
                }
                return lexer_make_token(lexer, TOKEN_DEDENT);
            }
            else
            {
                lexer->at_line_start = 0;
                DEBUG_VERBOSE("Line %d: Indentation unchanged, proceeding to scan token",
                              lexer->line);
            }
        }
    }
    DEBUG_VERBOSE("Line %d: Skipping whitespace within the line", lexer->line);
    lexer_skip_whitespace(lexer);
    lexer->start = lexer->current;
    if (lexer_is_at_end(lexer))
    {
        DEBUG_VERBOSE("Line %d: End of file reached", lexer->line);
        return lexer_make_token(lexer, TOKEN_EOF);
    }
    char c = lexer_advance(lexer);
    DEBUG_VERBOSE("Line %d: Scanning character '%c'", lexer->line, c);
    if (c == '\n')
    {
        lexer->line++;
        lexer->at_line_start = 1;
        DEBUG_VERBOSE("Line %d: Emitting NEWLINE", lexer->line - 1);
        return lexer_make_token(lexer, TOKEN_NEWLINE);
    }
    if (isalpha(c) || c == '_')
    {
        Token token = lexer_scan_identifier(lexer);
        DEBUG_VERBOSE("Line %d: Emitting identifier token type %d", lexer->line, token.type);
        return token;
    }
    if (isdigit(c))
    {
        Token token = lexer_scan_number(lexer);
        DEBUG_VERBOSE("Line %d: Emitting number token type %d", lexer->line, token.type);
        return token;
    }
    switch (c)
    {
    case '%':
        DEBUG_VERBOSE("Line %d: Emitting MODULO", lexer->line);
        return lexer_make_token(lexer, TOKEN_MODULO);
    case '/':
        DEBUG_VERBOSE("Line %d: Emitting SLASH", lexer->line);
        return lexer_make_token(lexer, TOKEN_SLASH);
    case '*':
        DEBUG_VERBOSE("Line %d: Emitting STAR", lexer->line);
        return lexer_make_token(lexer, TOKEN_STAR);
    case '+':
        if (lexer_match(lexer, '+'))
        {
            DEBUG_VERBOSE("Line %d: Emitting PLUS_PLUS", lexer->line);
            return lexer_make_token(lexer, TOKEN_PLUS_PLUS);
        }
        DEBUG_VERBOSE("Line %d: Emitting PLUS", lexer->line);
        return lexer_make_token(lexer, TOKEN_PLUS);
    case '(':
        DEBUG_VERBOSE("Line %d: Emitting LEFT_PAREN", lexer->line);
        return lexer_make_token(lexer, TOKEN_LEFT_PAREN);
    case ')':
        DEBUG_VERBOSE("Line %d: Emitting RIGHT_PAREN", lexer->line);
        return lexer_make_token(lexer, TOKEN_RIGHT_PAREN);
    case ':':
        DEBUG_VERBOSE("Line %d: Emitting COLON", lexer->line);
        return lexer_make_token(lexer, TOKEN_COLON);
    case '-':
        if (lexer_match(lexer, '-'))
        {
            DEBUG_VERBOSE("Line %d: Emitting MINUS_MINUS", lexer->line);
            return lexer_make_token(lexer, TOKEN_MINUS_MINUS);
        }
        else if (lexer_match(lexer, '>'))
        {
            DEBUG_VERBOSE("Line %d: Emitting ARROW", lexer->line);
            return lexer_make_token(lexer, TOKEN_ARROW);
        }
        DEBUG_VERBOSE("Line %d: Emitting MINUS", lexer->line);
        return lexer_make_token(lexer, TOKEN_MINUS);
    case '=':
        if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting EQUAL_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_EQUAL_EQUAL);
        }
        if (lexer_match(lexer, '>'))
        {
            DEBUG_VERBOSE("Line %d: Emitting ARROW (for '=>')", lexer->line);
            return lexer_make_token(lexer, TOKEN_ARROW);
        }
        DEBUG_VERBOSE("Line %d: Emitting EQUAL", lexer->line);
        return lexer_make_token(lexer, TOKEN_EQUAL);
    case '<':
        if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting LESS_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_LESS_EQUAL);
        }
        DEBUG_VERBOSE("Line %d: Emitting LESS", lexer->line);
        return lexer_make_token(lexer, TOKEN_LESS);
    case ',':
        DEBUG_VERBOSE("Line %d: Emitting COMMA", lexer->line);
        return lexer_make_token(lexer, TOKEN_COMMA);
    case ';':
        DEBUG_VERBOSE("Line %d: Emitting SEMICOLON", lexer->line);
        return lexer_make_token(lexer, TOKEN_SEMICOLON);
    case '"':
        Token string_token = lexer_scan_string(lexer);
        DEBUG_VERBOSE("Line %d: Emitting STRING_LITERAL", lexer->line);
        return string_token;
    case '\'':
        Token char_token = lexer_scan_char(lexer);
        DEBUG_VERBOSE("Line %d: Emitting CHAR_LITERAL", lexer->line);
        return char_token;
    case '$':
        if (lexer_peek(lexer) == '"')
        {
            lexer_advance(lexer);
            Token token = lexer_scan_string(lexer);
            token.type = TOKEN_INTERPOL_STRING;
            DEBUG_VERBOSE("Line %d: Emitting INTERPOL_STRING", lexer->line);
            return token;
        }
    default:
        snprintf(error_buffer, sizeof(error_buffer), "Unexpected character '%c'", c);
        DEBUG_VERBOSE("Line %d: Error - %s", lexer->line, error_buffer);
        return lexer_error_token(lexer, error_buffer);
    }
    if (lexer_is_at_end(lexer))
    {
        while (lexer->indent_size > 1)
        {
            lexer->indent_size--;
            return lexer_make_token(lexer, TOKEN_DEDENT);
        }
        return lexer_make_token(lexer, TOKEN_EOF);
    }
}