/**
 * lexer.c
 * Implementation of the lexical analyzer
 */

#include "lexer.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
    lexer->pending_dedents = 0;
    lexer->indent_unit = 0;
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
    char message[100];
    sprintf(message, "Indentation error: expected %d spaces, got %d spaces",
            expected, actual);
    lexer_error_token(lexer, message);
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
    Token token;
    token_init(&token, type, lexer->start, (int)(lexer->current - lexer->start), lexer->line);
    return token;
}

Token lexer_error_token(Lexer *lexer, const char *message)
{
    Token token;
    token_init(&token, TOKEN_ERROR, message, (int)strlen(message), lexer->line);
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
                // A comment goes until the end of the line
                while (lexer_peek(lexer) != '\n' && !lexer_is_at_end(lexer))
                {
                    lexer_advance(lexer);
                }
                // Now at newline, but don't consume it
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
    // Using a trie-like approach for keyword matching
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

    // See if the identifier is a reserved word
    TokenType type = lexer_identifier_type(lexer);

    // Special handling for boolean literals
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
    // Int part
    while (isdigit(lexer_peek(lexer)))
    {
        lexer_advance(lexer);
    }

    // Check for a decimal part
    if (lexer_peek(lexer) == '.' && isdigit(lexer_peek_next(lexer)))
    {
        // Consume the '.'
        lexer_advance(lexer);

        // Consume the fractional part
        while (isdigit(lexer_peek(lexer)))
        {
            lexer_advance(lexer);
        }

        // Check for double literal 'd' suffix
        if (lexer_peek(lexer) == 'd')
        {
            lexer_advance(lexer);
            Token token = lexer_make_token(lexer, TOKEN_DOUBLE_LITERAL);

            // Convert the substring to a double
            char buffer[256];
            int length = (int)(lexer->current - lexer->start - 1); // -1 to exclude the 'd'
            if (length >= (int)sizeof(buffer))
            {
                return lexer_error_token(lexer, "Number literal too long");
            }

            strncpy(buffer, lexer->start, length);
            buffer[length] = '\0';

            double value = strtod(buffer, NULL);
            token_set_double_literal(&token, value);

            return token;
        }

        // It's a double literal without the 'd' suffix
        Token token = lexer_make_token(lexer, TOKEN_DOUBLE_LITERAL);

        // Convert the substring to a double
        char buffer[256];
        int length = (int)(lexer->current - lexer->start);
        if (length >= (int)sizeof(buffer))
        {
            return lexer_error_token(lexer, "Number literal too long");
        }

        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';

        double value = strtod(buffer, NULL);
        token_set_double_literal(&token, value);

        return token;
    }

    // Check for long literal 'l' suffix
    if (lexer_peek(lexer) == 'l')
    {
        lexer_advance(lexer);
        Token token = lexer_make_token(lexer, TOKEN_LONG_LITERAL);

        // Convert the substring to a long
        char buffer[256];
        int length = (int)(lexer->current - lexer->start - 1); // -1 to exclude the 'l'
        if (length >= (int)sizeof(buffer))
        {
            return lexer_error_token(lexer, "Number literal too long");
        }

        strncpy(buffer, lexer->start, length);
        buffer[length] = '\0';

        int64_t value = strtoll(buffer, NULL, 10);
        token_set_int_literal(&token, value);

        return token;
    }

    // It's an int literal
    Token token = lexer_make_token(lexer, TOKEN_INT_LITERAL);

    // Convert the substring to an int
    char buffer[256];
    int length = (int)(lexer->current - lexer->start);
    if (length >= (int)sizeof(buffer))
    {
        return lexer_error_token(lexer, "Number literal too long");
    }

    strncpy(buffer, lexer->start, length);
    buffer[length] = '\0';

    int64_t value = strtoll(buffer, NULL, 10);
    token_set_int_literal(&token, value);

    return token;
}

/**
 * String handling fix for lexer.c
 * This addresses the strdup issue in the scan_string function
 */

Token lexer_scan_string(Lexer *lexer)
{
    // The opening quote is already consumed

    // Allocate buffer for the string value (dynamically resized if needed)
    int buffer_size = 256;
    char *buffer = malloc(buffer_size);
    if (buffer == NULL)
    {
        return lexer_error_token(lexer, "Memory allocation failed");
    }

    int buffer_index = 0;

    while (lexer_peek(lexer) != '"' && !lexer_is_at_end(lexer))
    {
        if (lexer_peek(lexer) == '\n')
        {
            lexer->line++;
        }

        // Handle escape sequences
        if (lexer_peek(lexer) == '\\')
        {
            lexer_advance(lexer); // consume the backslash
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
                return lexer_error_token(lexer, "Invalid escape sequence");
            }
        }
        else
        {
            buffer[buffer_index++] = lexer_peek(lexer);
        }

        lexer_advance(lexer);

        // Resize buffer if needed
        if (buffer_index >= buffer_size - 1)
        {
            buffer_size *= 2;
            char *new_buffer = realloc(buffer, buffer_size);
            if (new_buffer == NULL)
            {
                free(buffer);
                return lexer_error_token(lexer, "Memory allocation failed");
            }
            buffer = new_buffer;
        }
    }

    // Make sure we found the closing quote
    if (lexer_is_at_end(lexer))
    {
        free(buffer);
        return lexer_error_token(lexer, "Unterminated string");
    }

    // Consume the closing quote
    lexer_advance(lexer);

    // Null-terminate the string
    buffer[buffer_index] = '\0';

    // Create the token
    Token token = lexer_make_token(lexer, TOKEN_STRING_LITERAL);

    // Create a copy of the string that will persist
    char *str_copy = malloc(buffer_index + 1);
    if (str_copy == NULL)
    {
        free(buffer);
        return lexer_error_token(lexer, "Memory allocation failed");
    }

    strcpy(str_copy, buffer);
    token_set_string_literal(&token, str_copy);

    free(buffer);
    return token;
}

Token lexer_scan_char(Lexer *lexer)
{
    // The opening quote is already consumed

    char value = '\0';

    // Handle escape sequences
    if (lexer_peek(lexer) == '\\')
    {
        lexer_advance(lexer); // consume the backslash
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
            return lexer_error_token(lexer, "Invalid escape sequence");
        }
    }
    else if (lexer_peek(lexer) == '\'')
    {
        return lexer_error_token(lexer, "Empty character literal");
    }
    else
    {
        value = lexer_peek(lexer);
    }

    lexer_advance(lexer); // consume the character

    // Make sure we found the closing quote
    if (lexer_peek(lexer) != '\'')
    {
        return lexer_error_token(lexer, "Unterminated character literal");
    }

    // Consume the closing quote
    lexer_advance(lexer);

    // Create the token
    Token token = lexer_make_token(lexer, TOKEN_CHAR_LITERAL);
    token_set_char_literal(&token, value);

    return token;
}

Token lexer_scan_token(Lexer *lexer)
{
    // Log the start of token scanning
    DEBUG_VERBOSE("Line %d: Starting lexer_scan_token, at_line_start = %d", lexer->line, lexer->at_line_start);

    // Handle pending dedents first (for multi-level dedents)
    if (lexer->pending_dedents > 0)
    {
        lexer->pending_dedents--;
        DEBUG_VERBOSE("Line %d: Emitting pending DEDENT", lexer->line);
        return lexer_make_token(lexer, TOKEN_DEDENT);
    }

    // Handle indentation at the start of a line
    if (lexer->at_line_start)
    {
        // Calculate indentation level
        int current_indent = 0;
        const char *indent_start = lexer->current;
        while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t')
        {
            current_indent++;
            lexer_advance(lexer);
        }
        DEBUG_VERBOSE("Line %d: Calculated indent = %d", lexer->line, current_indent);

        // Check if the line is just whitespace or a comment
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
            int dedents = 0;
            int top = lexer->indent_stack[lexer->indent_size - 1];
            while (current_indent < top)
            {
                lexer->indent_size--;
                dedents++;
                top = lexer->indent_stack[lexer->indent_size - 1];
            }
            // After dedents, check for inconsistency
            if (current_indent != top)
            {
                lexer_report_indentation_error(lexer, top, current_indent);
                return lexer_error_token(lexer, "Inconsistent indentation");
            }
            lexer->at_line_start = 0;

            // Handle dedents
            if (dedents > 0)
            {
                lexer->pending_dedents = dedents - 1;
                DEBUG_VERBOSE("Line %d: Emitting DEDENT (with %d pending)", lexer->line, lexer->pending_dedents);
                return lexer_make_token(lexer, TOKEN_DEDENT);
            }

            // Handle indent if increased
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
                DEBUG_VERBOSE("Line %d: Pushing indent level %d, emitting INDENT",
                              lexer->line, current_indent);
                return lexer_make_token(lexer, TOKEN_INDENT);
            }
        }
    }

    // Skip whitespace within the line
    DEBUG_VERBOSE("Line %d: Skipping whitespace within the line", lexer->line);
    lexer_skip_whitespace(lexer);
    lexer->start = lexer->current;

    // Check for end of file
    if (lexer_is_at_end(lexer))
    {
        DEBUG_VERBOSE("Line %d: End of file reached", lexer->line);
        return lexer_make_token(lexer, TOKEN_EOF);
    }

    // Scan the next character
    char c = lexer_advance(lexer);
    DEBUG_VERBOSE("Line %d: Scanning character '%c'", lexer->line, c);

    // Handle newline
    if (c == '\n')
    {
        lexer->line++;
        lexer->at_line_start = 1;
        DEBUG_VERBOSE("Line %d: Emitting NEWLINE", lexer->line - 1);
        return lexer_make_token(lexer, TOKEN_NEWLINE);
    }

    // Handle identifiers and keywords
    if (isalpha(c) || c == '_')
    {
        Token token = lexer_scan_identifier(lexer);
        DEBUG_VERBOSE("Line %d: Emitting identifier token type %d", lexer->line, token.type);
        return token;
    }

    // Handle numbers
    if (isdigit(c))
    {
        Token token = lexer_scan_number(lexer);
        DEBUG_VERBOSE("Line %d: Emitting number token type %d", lexer->line, token.type);
        return token;
    }

    // Handle operators and punctuation
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
        if (lexer_match(lexer, '>'))
        {
            DEBUG_VERBOSE("Line %d: Emitting ARROW (for '->')", lexer->line);
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
    default:
        char msg[32];
        snprintf(msg, sizeof(msg), "Unexpected character '%c'", c);
        DEBUG_VERBOSE("Line %d: Error - %s", lexer->line, msg);
        return lexer_error_token(lexer, msg);
    }
}
