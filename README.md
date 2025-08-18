# SN Programming Language

## Overview
SN is a simple, statically-typed programming language designed for clarity and ease of use. It supports basic data types, functions, conditionals, loops, and string interpolation, making it suitable for a variety of programming tasks. The language features a clean syntax inspired by modern programming paradigms, with a focus on readability and straightforward semantics.

This repository contains a sample SN program (`main.sn`) and a supporting library (`main.library.sn`) that demonstrate the language's core features.

## Features

### Syntax
- **Function Declaration**: Functions are declared using the `fn` keyword, followed by the function name, parameters with types, return type, and body. Example:
  ```
  fn factorial(n: int): int => ...
  ```
- **Variable Declaration**: Variables are declared with `var`, followed by the variable name, type, and optional initialization. Example:
  ```
  var num: int = 5
  ```
- **String Interpolation**: Strings support interpolation using the `$` prefix and `{}` for expressions. Example:
  ```
  print($"Factorial of {num} is {fact}\n")
  ```
- **Control Structures**:
  - **If-Else**: Conditional statements use `if` and `else` with the `=>` operator to denote the block. Example:
    ```
    if is_prime(7) => ...
    else => ...
    ```
  - **For Loop**: Loops iterate over ranges with `for` and support standard loop syntax. Example:
    ```
    for var k: int = 1; k <= 10; k++ => ...
    ```
  - **While Loop**: Loops continue while a condition is true. Example:
    ```
    while i * i <= num => ...
    ```
- **Block Delimiter**: The `=>` operator introduces a block of code, replacing traditional curly braces or indentation-based scoping in other languages.

### Data Types
SN supports the following data types:
- `int`: Integer numbers (e.g., `5`, `10`)
- `double`: Floating-point numbers (e.g., `3.14159`)
- `char`: Single characters (e.g., `'A'`)
- `bool`: Boolean values (`true`, `false`)
- `str`: Strings (e.g., `"hello"`)
- `void`: Represents no return value for functions

### Functions
- Functions are first-class citizens and can be defined with or without return values.
- Parameters and return types are explicitly declared using the `: type` syntax.
- Example:
  ```
  fn repeat_string(text: str, count: int): str => ...
  ```

### Example Program
The `main.sn` file demonstrates a program that:
1. Calculates the factorial of a number using the `factorial` function.
2. Checks if a number is prime using the `is_prime` function.
3. Repeats a string using the `repeat_string` function.
4. Computes the sum of numbers from 1 to 10 using a `for` loop.
5. Prints various data types using the `print_types` function.
6. Demonstrates a void function with `print_void`.

The `main.library.sn` file contains the implementation of these functions, showcasing recursive function calls, looping constructs, and type handling.

## Usage
To run the example program:
1. Ensure you have an SN compiler or interpreter installed (not provided in this repository).
2. Compile or interpret `main.sn`, which imports `main.library.sn`.
3. Observe the output, which includes debug prints, computed results, and type information.

## Sample Output
Running `main.sn` produces output similar to:
```
Starting main method ... 
factorial: n=5
factorial: n=4
factorial: n=3
factorial: n=2
factorial: n=1
factorial: n <= 1 returning 1
factorial: j=2
factorial: j=6
factorial: j=24
factorial: j=120
Factorial of 5 is 120
is_prime: i=2
is_prime: i=3 (after increment)
7 is prime
repeat_string: j=0
repeat_string: count=3
repeat_string: j=1
repeat_string: count=3
repeat_string: j=2
repeat_string: count=3
hello hello hello world!
Sum 1 to 10: 55
Pi approx: 3.14159
Char: A
Flag: true
Str: 5
Fact: 120
Sum: 55
Double: 3.14159
Char: A
Bool: true
void
Complete main method ... 
```

## Limitations
- The language is designed for simplicity, so it may lack advanced features like object-oriented programming or complex data structures.
- The provided implementation does not include error handling for invalid inputs (e.g., negative numbers in `factorial`).
- The compiler or interpreter for SN is not included in this repository.

## Contributing
Contributions are welcome! Please submit pull requests or open issues to suggest improvements, additional features, or bug fixes.

## License
This project is licensed under the MIT License.