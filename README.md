# Three-Stage Compiler for a Statically Typed Language

This is a production-ready three-stage compiler for a statically typed language implemented in C. The compiler supports a variety of language features including variable declarations, functions, control flow statements, and more.

## Language Features

The language supports the following features:

- **Variables**: Declaration and initialization with types (`int`, `long`, `double`, `char`, `str`, `bool`)
- **Expressions**: Arithmetic, comparison, logical operations
- **Control Flow**: `if`, `while`, `for` statements
- **Functions**: Definition, parameters, return values
- **Imports**: Importing functions from other files
- **String Operations**: String concatenation

## Compiler Architecture

The compiler is structured in three stages:

1. **Lexical Analysis**: Implemented in `lexer.c`, converts source code into tokens.
2. **Parsing**: Implemented in `parser.c`, converts tokens into an Abstract Syntax Tree (AST).
3. **Code Generation**: Implemented in `code_gen.c`, converts the AST into x86-64 assembly code.

Additionally, the compiler performs type checking implemented in `type_checker.c` between parsing and code generation.

## Building the Compiler

To build the compiler, simply run:

```
make
```

This will compile all source files and create an executable called `compiler`.

## Usage

```
./compiler <source_file> [-o <output_file>] [-v]
```

Options:
- `-o <output_file>`: Specify the output assembly file (default: source_file.o)
- `-v`: Verbose mode, prints additional information during compilation

## Example

Here's an example program in the language:

```
fn add(x:int, y:int):int => 
   return x + y;

fn main(argv:str[], argc:int):int =>
   var a:int = 5;
   var b:int = 10;
   var result:int = add(a, b);
   
   print("The sum is: ");
   print(result);
   print("\n");
   
   return 0;
```

To compile and run this program:

```
./compiler example.sn -v
```

## Project Structure

- `token.h/c`: Token definitions and operations
- `lexer.h/c`: Lexical analyzer
- `ast.h/c`: Abstract Syntax Tree definitions
- `parser.h/c`: Parser implementation
- `symbol_table.h/c`: Symbol table for tracking variables and functions
- `type_checker.h/c`: Type checking implementation
- `code_gen.h/c`: Code generator implementation
- `compiler.h/c`: Main compiler module
- `main.c`: Entry point

## Limitations and Future Work

- Array implementation is limited
- No support for complex data structures or classes
- Limited optimization
- Basic error reporting
- Limited standard library

## License

This compiler is provided as-is with no warranty.