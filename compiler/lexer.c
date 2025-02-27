/**
 * lexer.c
 * Implementation of the lexical analyzer
 */

 #include "lexer.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 
 void init_lexer(Lexer* lexer, const char* source, const char* filename) {
     lexer->start = source;
     lexer->current = source;
     lexer->line = 1;
     lexer->filename = filename;
 }
 
 int is_at_end(Lexer* lexer) {
     return *lexer->current == '\0';
 }
 
 char advance(Lexer* lexer) {
     return *lexer->current++;
 }
 
 char peek(Lexer* lexer) {
     return *lexer->current;
 }
 
 char peek_next(Lexer* lexer) {
     if (is_at_end(lexer)) return '\0';
     return lexer->current[1];
 }
 
 int match(Lexer* lexer, char expected) {
     if (is_at_end(lexer)) return 0;
     if (*lexer->current != expected) return 0;
     
     lexer->current++;
     return 1;
 }
 
 Token make_token(Lexer* lexer, TokenType type) {
     Token token;
     init_token(&token, type, lexer->start, (int)(lexer->current - lexer->start), lexer->line);
     return token;
 }
 
 Token error_token(Lexer* lexer, const char* message) {
     Token token;
     init_token(&token, TOKEN_ERROR, message, (int)strlen(message), lexer->line);
     return token;
 }
 
 void skip_whitespace(Lexer* lexer) {
     for (;;) {
         char c = peek(lexer);
         switch (c) {
             case ' ':
             case '\r':
             case '\t':
                 advance(lexer);
                 break;
             case '\n':
                 lexer->line++;
                 advance(lexer);
                 break;
             case '/':
                 if (peek_next(lexer) == '/') {
                     // A comment goes until the end of the line
                     while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                         advance(lexer);
                     }
                 } else {
                     return;
                 }
                 break;
             default:
                 return;
         }
     }
 }
 
 TokenType check_keyword(Lexer* lexer, int start, int length, const char* rest, TokenType type) {
     int lexeme_length = (int)(lexer->current - lexer->start);
     if (lexeme_length == start + length && 
         memcmp(lexer->start + start, rest, length) == 0) {
         return type;
     }
     
     return TOKEN_IDENTIFIER;
 }
 
 TokenType identifier_type(Lexer* lexer) {
     // Using a trie-like approach for keyword matching
     switch (lexer->start[0]) {
         case 'b':
             if (lexer->current - lexer->start > 1) {
                 switch (lexer->start[1]) {
                     case 'o':
                         return check_keyword(lexer, 2, 2, "ol", TOKEN_BOOL);
                 }
             }
             break;
         case 'c':
             if (lexer->current - lexer->start > 1) {
                 switch (lexer->start[1]) {
                     case 'h':
                         return check_keyword(lexer, 2, 2, "ar", TOKEN_CHAR);
                 }
             }
             break;
         case 'd':
             if (lexer->current - lexer->start > 1) {
                 switch (lexer->start[1]) {
                     case 'o':
                         return check_keyword(lexer, 2, 4, "uble", TOKEN_DOUBLE);
                 }
             }
             break;
         case 'e':
             return check_keyword(lexer, 1, 3, "lse", TOKEN_ELSE);
         case 'f':
             if (lexer->current - lexer->start > 1) {
                 switch (lexer->start[1]) {
                     case 'a':
                         return check_keyword(lexer, 2, 3, "lse", TOKEN_BOOL_LITERAL);
                     case 'n':
                         return check_keyword(lexer, 2, 0, "", TOKEN_FN);
                     case 'o':
                         return check_keyword(lexer, 2, 1, "r", TOKEN_FOR);
                 }
             }
             break;
         case 'i':
             if (lexer->current - lexer->start > 1) {
                 switch (lexer->start[1]) {
                     case 'f':
                         return check_keyword(lexer, 2, 0, "", TOKEN_IF);
                     case 'm':
                         return check_keyword(lexer, 2, 4, "port", TOKEN_IMPORT);
                     case 'n':
                         return check_keyword(lexer, 2, 1, "t", TOKEN_INT);
                 }
             }
             break;
         case 'l':
             return check_keyword(lexer, 1, 3, "ong", TOKEN_LONG);
         case 'n':
             return check_keyword(lexer, 1, 2, "il", TOKEN_NIL);
         case 'r':
             return check_keyword(lexer, 1, 5, "eturn", TOKEN_RETURN);
         case 's':
             return check_keyword(lexer, 1, 2, "tr", TOKEN_STR);
         case 't':
             return check_keyword(lexer, 1, 3, "rue", TOKEN_BOOL_LITERAL);
         case 'v':
             if (lexer->current - lexer->start > 1) {
                 switch (lexer->start[1]) {
                     case 'a':
                         return check_keyword(lexer, 2, 1, "r", TOKEN_VAR);
                     case 'o':
                         return check_keyword(lexer, 2, 2, "id", TOKEN_VOID);
                 }
             }
             break;
         case 'w':
             return check_keyword(lexer, 1, 4, "hile", TOKEN_WHILE);
     }
     
     return TOKEN_IDENTIFIER;
 }
 
 Token scan_identifier(Lexer* lexer) {
     while (isalnum(peek(lexer)) || peek(lexer) == '_') {
         advance(lexer);
     }
     
     // See if the identifier is a reserved word
     TokenType type = identifier_type(lexer);
     
     // Special handling for boolean literals
     if (type == TOKEN_BOOL_LITERAL) {
         Token token = make_token(lexer, type);
         if (memcmp(lexer->start, "true", 4) == 0) {
             token_set_bool_literal(&token, 1);
         } else {
             token_set_bool_literal(&token, 0);
         }
         return token;
     }
     
     return make_token(lexer, type);
 }
 
 Token scan_number(Lexer* lexer) {
     // Int part
     while (isdigit(peek(lexer))) {
         advance(lexer);
     }
     
     // Check for a decimal part
     if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
         // Consume the '.'
         advance(lexer);
         
         // Consume the fractional part
         while (isdigit(peek(lexer))) {
             advance(lexer);
         }
         
         // Check for double literal 'd' suffix
         if (peek(lexer) == 'd') {
             advance(lexer);
             Token token = make_token(lexer, TOKEN_DOUBLE_LITERAL);
             
             // Convert the substring to a double
             char buffer[256];
             int length = (int)(lexer->current - lexer->start - 1); // -1 to exclude the 'd'
             if (length >= (int)sizeof(buffer)) {
                 return error_token(lexer, "Number literal too long");
             }
             
             strncpy(buffer, lexer->start, length);
             buffer[length] = '\0';
             
             double value = strtod(buffer, NULL);
             token_set_double_literal(&token, value);
             
             return token;
         }
         
         // It's a double literal without the 'd' suffix
         Token token = make_token(lexer, TOKEN_DOUBLE_LITERAL);
         
         // Convert the substring to a double
         char buffer[256];
         int length = (int)(lexer->current - lexer->start);
         if (length >= (int)sizeof(buffer)) {
             return error_token(lexer, "Number literal too long");
         }
         
         strncpy(buffer, lexer->start, length);
         buffer[length] = '\0';
         
         double value = strtod(buffer, NULL);
         token_set_double_literal(&token, value);
         
         return token;
     }
     
     // Check for long literal 'l' suffix
     if (peek(lexer) == 'l') {
         advance(lexer);
         Token token = make_token(lexer, TOKEN_LONG_LITERAL);
         
         // Convert the substring to a long
         char buffer[256];
         int length = (int)(lexer->current - lexer->start - 1); // -1 to exclude the 'l'
         if (length >= (int)sizeof(buffer)) {
             return error_token(lexer, "Number literal too long");
         }
         
         strncpy(buffer, lexer->start, length);
         buffer[length] = '\0';
         
         int64_t value = strtoll(buffer, NULL, 10);
         token_set_int_literal(&token, value);
         
         return token;
     }
     
     // It's an int literal
     Token token = make_token(lexer, TOKEN_INT_LITERAL);
     
     // Convert the substring to an int
     char buffer[256];
     int length = (int)(lexer->current - lexer->start);
     if (length >= (int)sizeof(buffer)) {
         return error_token(lexer, "Number literal too long");
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

 Token scan_string(Lexer* lexer) {
    // The opening quote is already consumed
    
    // Allocate buffer for the string value (dynamically resized if needed)
    int buffer_size = 256;
    char* buffer = malloc(buffer_size);
    if (buffer == NULL) {
        return error_token(lexer, "Memory allocation failed");
    }
    
    int buffer_index = 0;
    
    while (peek(lexer) != '"' && !is_at_end(lexer)) {
        if (peek(lexer) == '\n') {
            lexer->line++;
        }
        
        // Handle escape sequences
        if (peek(lexer) == '\\') {
            advance(lexer); // consume the backslash
            switch (peek(lexer)) {
                case '\\': buffer[buffer_index++] = '\\'; break;
                case 'n': buffer[buffer_index++] = '\n'; break;
                case 'r': buffer[buffer_index++] = '\r'; break;
                case 't': buffer[buffer_index++] = '\t'; break;
                case '"': buffer[buffer_index++] = '"'; break;
                default:
                    free(buffer);
                    return error_token(lexer, "Invalid escape sequence");
            }
        } else {
            buffer[buffer_index++] = peek(lexer);
        }
        
        advance(lexer);
        
        // Resize buffer if needed
        if (buffer_index >= buffer_size - 1) {
            buffer_size *= 2;
            char* new_buffer = realloc(buffer, buffer_size);
            if (new_buffer == NULL) {
                free(buffer);
                return error_token(lexer, "Memory allocation failed");
            }
            buffer = new_buffer;
        }
    }
    
    // Make sure we found the closing quote
    if (is_at_end(lexer)) {
        free(buffer);
        return error_token(lexer, "Unterminated string");
    }
    
    // Consume the closing quote
    advance(lexer);
    
    // Null-terminate the string
    buffer[buffer_index] = '\0';
    
    // Create the token
    Token token = make_token(lexer, TOKEN_STRING_LITERAL);
    
    // Create a copy of the string that will persist
    char* str_copy = malloc(buffer_index + 1);
    if (str_copy == NULL) {
        free(buffer);
        return error_token(lexer, "Memory allocation failed");
    }
    
    strcpy(str_copy, buffer);
    token_set_string_literal(&token, str_copy);
    
    free(buffer);
    return token;
}
 
 Token scan_char(Lexer* lexer) {
     // The opening quote is already consumed
     
     char value = '\0';
     
     // Handle escape sequences
     if (peek(lexer) == '\\') {
         advance(lexer); // consume the backslash
         switch (peek(lexer)) {
             case '\\': value = '\\'; break;
             case 'n': value = '\n'; break;
             case 'r': value = '\r'; break;
             case 't': value = '\t'; break;
             case '\'': value = '\''; break;
             default:
                 return error_token(lexer, "Invalid escape sequence");
         }
     } else if (peek(lexer) == '\'') {
         return error_token(lexer, "Empty character literal");
     } else {
         value = peek(lexer);
     }
     
     advance(lexer); // consume the character
     
     // Make sure we found the closing quote
     if (peek(lexer) != '\'') {
         return error_token(lexer, "Unterminated character literal");
     }
     
     // Consume the closing quote
     advance(lexer);
     
     // Create the token
     Token token = make_token(lexer, TOKEN_CHAR_LITERAL);
     token_set_char_literal(&token, value);
     
     return token;
 }
 
 Token scan_token(Lexer* lexer) {
     skip_whitespace(lexer);
     
     lexer->start = lexer->current;
     
     if (is_at_end(lexer)) {
         return make_token(lexer, TOKEN_EOF);
     }
     
     char c = advance(lexer);
     
     // Identifiers
     if (isalpha(c) || c == '_') {
         return scan_identifier(lexer);
     }
     
     // Numbers
     if (isdigit(c)) {
         return scan_number(lexer);
     }
     
     switch (c) {
         // Single-character tokens
         case '(': return make_token(lexer, TOKEN_LEFT_PAREN);
         case ')': return make_token(lexer, TOKEN_RIGHT_PAREN);
         case '{': return make_token(lexer, TOKEN_LEFT_BRACE);
         case '}': return make_token(lexer, TOKEN_RIGHT_BRACE);
         case '[': return make_token(lexer, TOKEN_LEFT_BRACKET);
         case ']': return make_token(lexer, TOKEN_RIGHT_BRACKET);
         case ';': return make_token(lexer, TOKEN_SEMICOLON);
         case ':': return make_token(lexer, TOKEN_COLON);
         case ',': return make_token(lexer, TOKEN_COMMA);
         case '.': return make_token(lexer, TOKEN_DOT);
         case '-': 
             if (match(lexer, '>')) {
                 return make_token(lexer, TOKEN_ARROW);
             } else if (match(lexer, '-')) {
                 return make_token(lexer, TOKEN_MINUS_MINUS);
             }
             return make_token(lexer, TOKEN_MINUS);
         case '+': 
             if (match(lexer, '+')) {
                 return make_token(lexer, TOKEN_PLUS_PLUS);
             }
             return make_token(lexer, TOKEN_PLUS);
         case '*': return make_token(lexer, TOKEN_STAR);
         case '/': return make_token(lexer, TOKEN_SLASH);
         case '%': return make_token(lexer, TOKEN_MODULO);
         
         // One or two character tokens
         case '!':
             return make_token(lexer, match(lexer, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
         case '=':
             if (match(lexer, '=')) {
                 return make_token(lexer, TOKEN_EQUAL_EQUAL);
             } else if (match(lexer, '>')) {
                 return make_token(lexer, TOKEN_ARROW);
             }
             return make_token(lexer, TOKEN_EQUAL);
         case '<':
             return make_token(lexer, match(lexer, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
         case '>':
             return make_token(lexer, match(lexer, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
         
         // Literals
         case '"': return scan_string(lexer);
         case '\'': return scan_char(lexer);
         
         // Two-character tokens
         case '&':
             if (match(lexer, '&')) {
                 return make_token(lexer, TOKEN_AND);
             }
             return error_token(lexer, "Expected '&' after '&'");
         case '|':
             if (match(lexer, '|')) {
                 return make_token(lexer, TOKEN_OR);
             }
             return error_token(lexer, "Expected '|' after '|'");
     }
     
     return error_token(lexer, "Unexpected character");
 }