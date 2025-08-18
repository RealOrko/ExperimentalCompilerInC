# Sn Programming Language

The Sn programming language is a simple, procedural programming language designed for clarity and ease of use. This README provides a detailed overview of the language's syntax, features, and semantics based on the provided example program (`main.sn`). The language supports basic control structures, functions, and a variety of data types, with a focus on readable syntax and straightforward functionality.

## Table of Contents
- [Overview](#overview)
- [Syntax and Features](#syntax-and-features)
  - [Functions](#functions)
  - [Data Types](#data-types)
  - [Control Structures](#control-structures)
  - [Variable Declarations](#variable-declarations)
  - [Operators](#operators)
  - [String Interpolation](#string-interpolation)
  - [Print Statements](#print-statements)
- [Example Program Analysis](#example-program-analysis)
- [Running the Program](#running-the-program)
- [Limitations and Assumptions](#limitations-and-assumptions)

## Overview
Sn is a procedural programming language with a syntax that emphasizes readability and simplicity. It supports functions with return types, basic data types (integers, doubles, strings, characters, and booleans), and standard control structures like conditionals, loops, and function calls. The language uses a distinctive arrow-based syntax (`=>`) for defining function bodies and control flow blocks, which contributes to its clean and concise appearance.

The example program (`main.sn`) demonstrates key features of Sn, including:
- Function definitions for calculating factorials, checking prime numbers, and repeating strings.
- Use of variables, loops, conditionals, and string interpolation.
- Basic input/output via print statements.

## Syntax and Features

### Functions
Functions in Sn are defined using the `fn` keyword, followed by the function name, parameters with their types, a return type, and a body introduced by the `=>` operator.

**Syntax:**
```sn
fn function_name(param1: type, param2: type): return_type =>
  // function body
```

**Example:**
```sn
fn factorial(n: int): int =>
  print($"factorial: n={n}\n")
  if n <= 1 =>
    print($"factorial: n <= 1 returning 1\n")
    return 1
  var j: int = n * factorial(n - 1)
  print($"factorial: j={j}\n")
  return j
```
- **Parameters**: Parameters are declared with their types (e.g., `n: int`).
- **Return Type**: Specified after the parameter list (e.g., `: int`).
- **Body**: Enclosed in `=>`, with statements like `print`, `if`, or `return`.
- **Recursion**: Supported, as seen in the `factorial` function, which calls itself.

### Data Types
Sn supports the following data types, as inferred from the example:
- **int**: Integer numbers (e.g., `5`, `7`).
- **double**: Floating-point numbers (e.g., `3.14159`).
- **str**: Strings (e.g., `"hello "`).
- **char**: Single characters (e.g., `'A'`).
- **bool**: Boolean values (`true`, `false`).

Variables are explicitly typed using the `var` keyword and a type annotation.

### Control Structures
Sn provides standard control structures for conditionals and loops.

#### Conditionals
**If-Else Syntax:**
```sn
if condition =>
  // statements
else =>
  // statements
```
- Conditions are enclosed in `=>` blocks.
- The `else` clause is optional.
- Example:
  ```sn
  if is_prime(7) =>
    print("7 is prime\n")
  else =>
    print("7 is not prime\n")
  ```

#### Loops
Sn supports `while` and `for` loops.

**While Loop Syntax:**
```sn
while condition =>
  // statements
```
- Example:
  ```sn
  while i * i <= num =>
    if num % i == 0 =>
      return false
    i = i + 1
  ```

**For Loop Syntax:**
```sn
for var variable: type = start; condition; increment =>
  // statements
```
- Example:
  ```sn
  for var k: int = 1; k <= 10; k++ =>
    sum = sum + k
  ```
- The `for` loop includes initialization, a condition, and an increment operation, similar to C-style languages.

### Variable Declarations
Variables are declared using the `var` keyword, followed by the variable name, type, and an optional initial value.

**Syntax:**
```sn
var variable_name: type = value
```
- Example:
  ```sn
  var num: int = 5
  var result: str = ""
  ```
- Variables must be explicitly typed.
- Variables can be reassigned, as seen with the `any` variable in the example.

### Operators
Sn supports standard arithmetic, comparison, and logical operators:
- **Arithmetic**: `+`, `-`, `*`, `%` (e.g., `n * factorial(n - 1)`, `num % i`).
- **Comparison**: `<=`, `==` (e.g., `n <= 1`, `num % i == 0`).
- **Assignment**: `=` for assignment and `++` for increment (e.g., `i = i + 1`, `j++`).
- The language does not appear to support compound assignment operators like `+=` (based on the example, which uses `sum = sum + k`).

### String Interpolation
Sn supports string interpolation using the `$` prefix for strings, with expressions embedded in curly braces `{}`.

**Syntax:**
```sn
$"text {expression} text"
```
- Example:
  ```sn
  print($"Factorial of {num} is {fact}\n")
  ```
- Interpolated expressions can include variables of any type (e.g., `int`, `double`, `char`, `bool`).
- The `$` prefix is required for interpolation; plain strings use double quotes without `$`.

### Print Statements
The `print` function is used for output, supporting both plain strings and interpolated strings.

**Syntax:**
```sn
print("text")
print($"text {expression}")
```
- Example:
  ```sn
  print("Starting main method ... \n")
  print($"Sum 1 to 10: {sum}\n")
  ```
- Strings often include a newline (`\n`) for formatting.

## Example Program Analysis
The provided `main.sn` program demonstrates the core features of Sn. Below is a summary of its functionality:
- **Factorial Function**: Computes the factorial of a number using recursion.
- **Is Prime Function**: Checks if a number is prime by testing divisibility up to its square root.
- **Repeat String Function**: Concatenates a string a specified number of times.
- **Main Function**: 
  - Computes the factorial of 5.
  - Checks if 7 is prime.
  - Repeats the string `"hello "` three times and appends `"world!"`.
  - Calculates the sum of numbers from 1 to 10.
  - Demonstrates various data types (`int`, `double`, `char`, `bool`, `str`) with string interpolation.

The program outputs intermediate values for debugging (e.g., `print($"factorial: n={n}\n")`), making it useful for understanding execution flow.

## Running the Program
The example program assumes a compiler or interpreter for Sn that translates the code into executable form. To run `main.sn`:
1. Ensure a Sn compiler or interpreter is installed (not provided in the example).
2. Compile or interpret the program:
   ```bash
   snc main.sn
   ```
   (Assuming `snc` is the compiler command.)
3. The program will output the results of the factorial, prime check, string repetition, sum, and various variable values.

## Limitations and Assumptions
Based on the example, the following assumptions are made about Sn:
- **No Standard Library Details**: The example does not indicate a standard library beyond `print`.
- **No Error Handling**: The language does not show explicit error-handling mechanisms (e.g., try-catch).
- **No Input Mechanism**: The example lacks input functions, suggesting Sn may rely on hardcoded values or external input not shown.
- **Single-File Programs**: The example is a single file (`main.sn`), and there is no evidence of module imports or multi-file programs.
- **Type Safety**: The language appears to be statically typed, with explicit type annotations for variables and parameters.

Further exploration of Sn would require additional documentation or examples to clarify features like file I/O, libraries, or advanced data structures.
