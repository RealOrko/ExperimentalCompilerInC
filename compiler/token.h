/**
 * token.h
 * Token definitions for the compiler
 */

 #ifndef TOKEN_H
 #define TOKEN_H
 
 #include <stdint.h>
 
 // Token types
 typedef enum {
     // End of file
     TOKEN_EOF,
     
     // Literals
     TOKEN_INT_LITERAL,     // e.g. 42
     TOKEN_LONG_LITERAL,    // e.g. 42l
     TOKEN_DOUBLE_LITERAL,  // e.g. 3.14d
     TOKEN_CHAR_LITERAL,    // e.g. 'a'
     TOKEN_STRING_LITERAL,  // e.g. "hello"
     TOKEN_BOOL_LITERAL,    // true or false
     
     // Identifiers and keywords
     TOKEN_IDENTIFIER,      // e.g. foo
     TOKEN_FN,              // fn
     TOKEN_VAR,             // var
     TOKEN_RETURN,          // return
     TOKEN_IF,              // if
     TOKEN_ELSE,            // else
     TOKEN_FOR,             // for
     TOKEN_WHILE,           // while
     TOKEN_IMPORT,          // import
     TOKEN_NIL,             // nil
     
     // Types
     TOKEN_INT,             // int
     TOKEN_LONG,            // long
     TOKEN_DOUBLE,          // double
     TOKEN_CHAR,            // char
     TOKEN_STR,             // str
     TOKEN_BOOL,            // bool
     TOKEN_VOID,            // void
     
     // Operators
     TOKEN_PLUS,            // +
     TOKEN_MINUS,           // -
     TOKEN_STAR,            // *
     TOKEN_SLASH,           // /
     TOKEN_MODULO,          // %
     TOKEN_EQUAL,           // =
     TOKEN_EQUAL_EQUAL,     // ==
     TOKEN_BANG,            // !
     TOKEN_BANG_EQUAL,      // !=
     TOKEN_LESS,            // <
     TOKEN_LESS_EQUAL,      // <=
     TOKEN_GREATER,         // >
     TOKEN_GREATER_EQUAL,   // >=
     TOKEN_AND,             // &&
     TOKEN_OR,              // ||
     TOKEN_PLUS_PLUS,       // ++
     TOKEN_MINUS_MINUS,     // --
     
     // Punctuation
     TOKEN_LEFT_PAREN,      // (
     TOKEN_RIGHT_PAREN,     // )
     TOKEN_LEFT_BRACE,      // {
     TOKEN_RIGHT_BRACE,     // }
     TOKEN_LEFT_BRACKET,    // [
     TOKEN_RIGHT_BRACKET,   // ]
     TOKEN_SEMICOLON,       // ;
     TOKEN_COLON,           // :
     TOKEN_COMMA,           // ,
     TOKEN_DOT,             // .
     TOKEN_ARROW,           // =>
     
     // Special
     TOKEN_ERROR            // For lexer errors
 } TokenType;
 
 // Token struct
 typedef struct {
     TokenType type;
     const char* start;
     int length;
     int line;
     
     // For storing literal values
     union {
         int64_t int_value;
         double double_value;
         char char_value;
         const char* string_value;
         int bool_value;
     } literal;
 } Token;
 
 // Token functions
 void init_token(Token* token, TokenType type, const char* start, int length, int line);
 void token_set_int_literal(Token* token, int64_t value);
 void token_set_double_literal(Token* token, double value);
 void token_set_char_literal(Token* token, char value);
 void token_set_string_literal(Token* token, const char* value);
 void token_set_bool_literal(Token* token, int value);
 const char* token_type_to_string(TokenType type);
 void print_token(Token* token);
 
 #endif // TOKEN_H