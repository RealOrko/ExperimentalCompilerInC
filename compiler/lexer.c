#include "lexer.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

static char error_buffer[128];

void lexer_init(Arena *arena, Lexer *lexer, const char *source, const char *filename)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->filename = filename;
    lexer->indent_capacity = 8;
    lexer->indent_stack = arena_alloc(arena, sizeof(int) * lexer->indent_capacity);
    lexer->indent_size = 1;
    lexer->indent_capacity = 8;
    lexer->indent_stack[0] = 0;
    lexer->at_line_start = 1;
    lexer->indent_unit = 0;
    lexer->arena = arena;
}

void lexer_cleanup(Lexer *lexer)
{
    lexer->indent_stack = NULL;
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
    char *dup_start = arena_strndup(lexer->arena, lexer->start, length);
    if (dup_start == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating lexeme");
        exit(1);
    }
    Token token;
    token_init(&token, type, dup_start, length, lexer->line, lexer->filename);
    return token;
}

Token lexer_error_token(Lexer *lexer, const char *message)
{
    char *dup_message = arena_strdup(lexer->arena, message);
    if (dup_message == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    Token token;
    token_init(&token, TOKEN_ERROR, dup_message, (int)strlen(dup_message), lexer->line, lexer->filename);
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
    case 'a':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'n':
                return lexer_check_keyword(lexer, 2, 1, "d", TOKEN_AND);
            }
        }
        break;
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
    case 'o':
        if (lexer->current - lexer->start > 1)
        {
            switch (lexer->start[1])
            {
            case 'r':
                return lexer_check_keyword(lexer, 2, 0, "", TOKEN_OR);
            }
        }
        break;
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

Token lexer_scan_identifier(Lexer *lexer) {
    while (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_') {
        lexer_advance(lexer);
    }

    TokenType type = lexer_identifier_type(lexer);

    Token token = lexer_make_token(lexer, type);

    if (type == TOKEN_BOOL_LITERAL) {
        size_t len = (size_t)(lexer->current - lexer->start);
        if (len == 4 && memcmp(lexer->start, "true", 4) == 0) {
            token_set_bool_literal(&token, 1);
        } else if (len == 5 && memcmp(lexer->start, "false", 5) == 0) {
            token_set_bool_literal(&token, 0);
        } else {
            // This should not occur if keyword checks are correct, but for robustness
            DEBUG_ERROR("Invalid boolean literal lexeme");
            token.type = TOKEN_ERROR;
        }
    }

    DEBUG_VERBOSE("Line %d: Emitting identifier token type %d", lexer->line, token.type);
    return token;
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
    char *buffer = arena_alloc(lexer->arena, buffer_size);
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
            char *new_buffer = arena_alloc(lexer->arena, buffer_size);
            if (new_buffer == NULL)
            {
                snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
                return lexer_error_token(lexer, error_buffer);
            }
            memcpy(new_buffer, buffer, buffer_index);
            buffer = new_buffer;
        }
    }
    if (lexer_is_at_end(lexer))
    {
        snprintf(error_buffer, sizeof(error_buffer), "Unterminated string");
        return lexer_error_token(lexer, error_buffer);
    }
    lexer_advance(lexer);
    buffer[buffer_index] = '\0';
    Token token = lexer_make_token(lexer, TOKEN_STRING_LITERAL);
    char *str_copy = arena_strdup(lexer->arena, buffer);
    if (str_copy == NULL)
    {
        snprintf(error_buffer, sizeof(error_buffer), "Memory allocation failed");
        return lexer_error_token(lexer, error_buffer);
    }
    token_set_string_literal(&token, str_copy);
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

    // Handle EOF and pending dedents at the start
    if (lexer_is_at_end(lexer)) {
        if (lexer->indent_size > 1) {
            lexer->indent_size--;
            DEBUG_VERBOSE("Line %d: Emitting DEDENT at EOF", lexer->line);
            return lexer_make_token(lexer, TOKEN_DEDENT);
        } else {
            DEBUG_VERBOSE("Line %d: Emitting EOF", lexer->line);
            return lexer_make_token(lexer, TOKEN_EOF);
        }
    }

    if (lexer->at_line_start) {
        while (true) {
            // Check for EOF inside the loop to handle trailing blanks reaching EOF
            if (lexer_is_at_end(lexer)) {
                if (lexer->indent_size > 1) {
                    lexer->indent_size--;
                    DEBUG_VERBOSE("Line %d: Emitting DEDENT at EOF", lexer->line);
                    return lexer_make_token(lexer, TOKEN_DEDENT);
                } else {
                    DEBUG_VERBOSE("Line %d: Emitting EOF", lexer->line);
                    return lexer_make_token(lexer, TOKEN_EOF);
                }
            }

            int current_indent = 0;
            const char *indent_start = lexer->current;
            while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t') {
                current_indent++;
                lexer_advance(lexer);
            }
            DEBUG_VERBOSE("Line %d: Calculated indent = %d", lexer->line, current_indent);

            const char *content_start = lexer->current;
            const char *saved_current = lexer->current;
            lexer_skip_whitespace(lexer);
            bool ignore_line = lexer_is_at_end(lexer) || lexer_peek(lexer) == '\n';

            if (ignore_line) {
                // Consume the newline if present
                if (!lexer_is_at_end(lexer) && lexer_peek(lexer) == '\n') {
                    lexer_advance(lexer);
                    lexer->line++;
                }
                DEBUG_VERBOSE("Line %d: Ignoring blank or comment line", lexer->line - (lexer_peek(lexer) == '\n' ? 0 : 1));
                continue;  // Loop back to handle next line
            } else {
                // Not ignoring, reset current to before skip_whitespace
                lexer->current = saved_current;
                lexer->start = lexer->current;

                // Handle indentation
                int top = lexer->indent_stack[lexer->indent_size - 1];
                DEBUG_VERBOSE("Line %d: Top of indent_stack = %d, indent_size = %d",
                              lexer->line, top, lexer->indent_size);

                if (current_indent > top) {
                    if (lexer->indent_unit == 0) {
                        lexer->indent_unit = current_indent - top;
                    } else {
                        int expected = top + lexer->indent_unit;
                        if (current_indent != expected) {
                            lexer_report_indentation_error(lexer, expected, current_indent);
                            return lexer_error_token(lexer, error_buffer);
                        }
                    }
                    if (lexer->indent_size >= lexer->indent_capacity) {
                        lexer->indent_capacity *= 2;
                        lexer->indent_stack = arena_alloc(lexer->arena,
                                                          lexer->indent_capacity * sizeof(int));
                        if (lexer->indent_stack == NULL) {
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
                } else if (current_indent < top) {
                    lexer->indent_size--;
                    int new_top = lexer->indent_stack[lexer->indent_size - 1];
                    DEBUG_VERBOSE("Line %d: Popped indent level, new top = %d, indent_size = %d",
                                  lexer->line, new_top, lexer->indent_size);
                    if (current_indent == new_top) {
                        lexer->at_line_start = 0;
                        DEBUG_VERBOSE("Line %d: Emitting DEDENT, indentation matches stack",
                                      lexer->line);
                        return lexer_make_token(lexer, TOKEN_DEDENT);
                    } else if (current_indent > new_top) {
                        DEBUG_VERBOSE("Line %d: Error - Inconsistent indentation (current %d > new_top %d)",
                                      lexer->line, current_indent, new_top);
                        snprintf(error_buffer, sizeof(error_buffer), "Inconsistent indentation");
                        return lexer_error_token(lexer, error_buffer);
                    } else {
                        DEBUG_VERBOSE("Line %d: Emitting DEDENT, more dedents pending",
                                      lexer->line);
                        return lexer_make_token(lexer, TOKEN_DEDENT);
                    }
                } else {
                    lexer->at_line_start = 0;
                    DEBUG_VERBOSE("Line %d: Indentation unchanged, proceeding to scan token",
                                  lexer->line);
                }
                break;  // Proceed to scan the token
            }
        }
    }

    DEBUG_VERBOSE("Line %d: Skipping whitespace within the line", lexer->line);
    lexer_skip_whitespace(lexer);

    lexer->start = lexer->current;
    if (lexer_is_at_end(lexer)) {
        if (lexer->indent_size > 1) {
            lexer->indent_size--;
            DEBUG_VERBOSE("Line %d: Emitting DEDENT at EOF", lexer->line);
            return lexer_make_token(lexer, TOKEN_DEDENT);
        } else {
            DEBUG_VERBOSE("Line %d: Emitting EOF", lexer->line);
            return lexer_make_token(lexer, TOKEN_EOF);
        }
    }

    char c = lexer_advance(lexer);
    DEBUG_VERBOSE("Line %d: Scanning character '%c'", lexer->line, c);

    if (c == '\n') {
        lexer->line++;
        lexer->at_line_start = 1;
        DEBUG_VERBOSE("Line %d: Emitting NEWLINE", lexer->line - 1);
        return lexer_make_token(lexer, TOKEN_NEWLINE);
    }

    if (isalpha(c) || c == '_') {
        return lexer_scan_identifier(lexer);
    }

    if (isdigit(c)) {
        return lexer_scan_number(lexer);
    }

    switch (c) {
    case '%': return lexer_make_token(lexer, TOKEN_MODULO);
    case '/': return lexer_make_token(lexer, TOKEN_SLASH);
    case '*': return lexer_make_token(lexer, TOKEN_STAR);
    case '+':
        return lexer_make_token(lexer, lexer_match(lexer, '+') ? TOKEN_PLUS_PLUS : TOKEN_PLUS);
    case '(': return lexer_make_token(lexer, TOKEN_LEFT_PAREN);
    case ')': return lexer_make_token(lexer, TOKEN_RIGHT_PAREN);
    case '{': return lexer_make_token(lexer, TOKEN_LEFT_BRACE);
    case '}': return lexer_make_token(lexer, TOKEN_RIGHT_BRACE);
    case '[': return lexer_make_token(lexer, TOKEN_LEFT_BRACKET);
    case ']': return lexer_make_token(lexer, TOKEN_RIGHT_BRACKET);
    case ':': return lexer_make_token(lexer, TOKEN_COLON);
    case '-':
        if (lexer_match(lexer, '-')) return lexer_make_token(lexer, TOKEN_MINUS_MINUS);
        if (lexer_match(lexer, '>')) return lexer_make_token(lexer, TOKEN_ARROW);
        return lexer_make_token(lexer, TOKEN_MINUS);
    case '=':
        if (lexer_match(lexer, '=')) return lexer_make_token(lexer, TOKEN_EQUAL_EQUAL);
        if (lexer_match(lexer, '>')) return lexer_make_token(lexer, TOKEN_ARROW);
        return lexer_make_token(lexer, TOKEN_EQUAL);
    case '<':
        return lexer_make_token(lexer, lexer_match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
        return lexer_make_token(lexer, lexer_match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '!':
        return lexer_make_token(lexer, lexer_match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case ',': return lexer_make_token(lexer, TOKEN_COMMA);
    case '.': return lexer_make_token(lexer, TOKEN_DOT);
    case ';': return lexer_make_token(lexer, TOKEN_SEMICOLON);
    case '"': return lexer_scan_string(lexer);
    case '\'': return lexer_scan_char(lexer);
    case '$':
        if (lexer_peek(lexer) == '"') {
            lexer_advance(lexer);
            Token token = lexer_scan_string(lexer);
            token.type = TOKEN_INTERPOL_STRING;
            return token;
        }
        // Fall through
    default:
        snprintf(error_buffer, sizeof(error_buffer), "Unexpected character '%c'", c);
        DEBUG_VERBOSE("Line %d: Error - %s", lexer->line, error_buffer);
        return lexer_error_token(lexer, error_buffer);
    }
}
