# Experimental Compiler in C

This is an experimental compiler written in C that translates a simple scripting language into x86-64 assembly for execution on Linux systems. It supports basic function definitions, parameter passing, and a `std.print` function for outputting strings. The project is modular, with separate components for lexing, parsing, and code generation.

**Note**: This compiler currently only works on Linux due to its use of Linux-specific syscalls (e.g., `sys_write`, `sys_exit`) and the ELF64 assembly format generated for `nasm` and `ld`.

## Current Progress

### Features
- **Language Syntax**:
  - Function definitions with single string parameters: `func name(param:str):void =>`.
  - Calls to functions with string literals: `name("string")`.
  - Import statement: `import std;` (currently only supports `std.print`).
  - `std.print(msg)`: Outputs a string followed by a newline.

- **Compilation Pipeline**:
  - **Lexer**: Tokenizes the input script into a sequence of tokens (e.g., `TOK_IDENT`, `TOK_STRING`, `TOK_INDENT`).
  - **Parser**: Builds an Abstract Syntax Tree (AST) from tokens, handling function definitions and calls.
  - **Code Generator**: Produces x86-64 assembly for Linux, using stack-based parameter passing.

- **Debug Mode**:
  - Supports a `-d` flag to enable detailed debug output, showing tokenization, statement parsing, and function generation steps.

- **Output**:
  - Generates an `output.asm` file, assembled with `nasm` and linked with `ld` into an executable binary.
  - Example script outputs "Hello World" as intended.

### Example Script
The following script demonstrates the current capabilities:

```
import std;
func hello(msg:str):void =>
std.print(msg);

func world(msg:str):void =>
std.print(msg);

func main():void =>
hello("Hello");
world(" World");
```

- **Output**: "Hello\n World" (two lines due to newlines in `std.print`).

### Limitations
- Only supports single string parameters per function.
- No type checking beyond basic syntax.
- Limited `std` library (only `std.print`).
- No support for expressions (e.g., string concatenation), multiple statements beyond calls, or complex control flow.
- Linux-specific due to assembly and syscall usage.

## Usage

### Prerequisites
- **Linux**: The compiler targets x86-64 Linux systems.
- **GCC**: To compile the C source code.
- **NASM**: To assemble the generated assembly (`nasm -f elf64`).
- **LD**: To link the object file into an executable.

### Building the Compiler

1. Place the source files (`lexer.c`, `lexer.h`, `parser.c`, `ast.h`, `codegen.c`, `codegen.h`, `main.c`) in a `compiler/` directory.
2. Use the provided build script (`scripts/compiler.sh`):
   ```bash
   chmod +x scripts/compiler.sh
   ./scripts/compiler.sh
