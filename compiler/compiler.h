/**
 * compiler.h
 * Main compiler module
 */

 #ifndef COMPILER_H
 #define COMPILER_H
 
 #include "lexer.h"
 #include "parser.h"
 #include "type_checker.h"
 #include "code_gen.h"
 #include <stdio.h>
 
 typedef struct
 {
     char *source_file;
     char *output_file;
     int verbose;
     int log_level;  // Added log level field
 } CompilerOptions;
 
 // Compiler initialization and cleanup
 void init_compiler_options(CompilerOptions *options);
 void compiler_options_cleanup(CompilerOptions *options);
 
 // Parse command line arguments
 int parse_args(int argc, char **argv, CompilerOptions *options);
 
 // Read source file
 char *read_file(const char *path);
 
 #endif // COMPILER_H