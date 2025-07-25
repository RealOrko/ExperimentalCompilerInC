Language Specification for Custom C Compiler
This document outlines the language specification for a custom programming language designed to be compiled by a C-based compiler. The language supports a concise, readable syntax with functional and imperative features. Below is a detailed description of the language's syntax and semantics, based on the provided example code.
1. General Syntax

Indentation: The language uses indentation (2 spaces) to denote blocks, similar to Python, instead of curly braces.
Arrow Operator (=>): Used to define function bodies, conditionals, loops, and other control structures.
Semicolons: Statements within the same block do not require semicolons, as line breaks separate statements.
Comments: Not shown in the example, but the language supports single-line comments starting with // and multi-line comments with /* */.

2. Data Types
The language supports the following primitive data types:

int: 32-bit signed integer (e.g., 5, -10).
bool: Boolean values (true, false).
str: String type for text data (e.g., "hello").
double: 64-bit floating-point number (e.g., 3.14159).
char: Single character (e.g., 'A').

Variables are declared using the var keyword, followed by the variable name, a colon, and the type:
var num: int = 5
var text: str = "hello"

3. Functions
Functions are defined using the fn keyword, followed by the function name, parameters, return type, and body.
Syntax
fn <name>(<param1>: <type1>, <param2>: <type2>, ...): <return_type> =>
  <body>


Parameters are specified in parentheses, with each parameter followed by its type.
The return type is specified after a colon.
The function body is introduced by => and indented.
The return statement is used to return a value.

Example
fn factorial(n: int): int =>
  if n <= 1 =>
    return 1
  return n * factorial(n - 1)


The factorial function takes an int parameter n and returns an int.
It uses recursion to compute the factorial.

4. Control Structures
4.1 If-Else
Conditional statements use the if keyword, followed by a condition, =>, and an indented block. An optional else clause can follow.
if <condition> =>
  <body>
else =>
  <body>


Conditions are expressions evaluating to a bool.
Example:if is_prime(7) =>
  print("7 is prime")
else =>
  print("7 is not prime")



4.2 While Loop
The while loop executes a block while a condition is true.
while <condition> =>
  <body>


Example:while i * i <= num =>
  if num % i == 0 =>
    return false
  i = i + 1



4.3 For Loop
The for loop iterates over a range or sequence, with a loop variable declared inline.
for var <var>: <type> = <start>; <condition>; <increment> =>
  <body>


Example:for var j: int = 0; j < count; j++ =>
  result = result + text


The loop variable (j) is declared with var and a type.
The j++ syntax increments the variable by 1.

5. String Interpolation
Strings support interpolation using the $ prefix and {} to embed expressions.
$"This is a {variable}"


Example:print($"Factorial of {num} is {fact}")


The expression inside {} is evaluated and converted to a string.
Supported types for interpolation: int, double, char, bool, str.

6. Operators
The language supports standard arithmetic, comparison, and logical operators:

Arithmetic: +, -, *, /, % (modulus).
Comparison: ==, !=, <, <=, >, >=.
Logical: Not shown in the example, but assumed to include && (and), || (or), ! (not).
Assignment: = for assignment, +=, -=, etc., are not shown but may be supported.

7. Built-in Functions
The language includes a print function for console output.
print(<expression>)


The argument can be a string, interpolated string, or other printable type.
Example:print("7 is prime")
print($"Pi approx: {pi_approx}")



8. Variable Scoping

Variables declared with var are scoped to the block (function, loop, or conditional) in which they are defined.
Example:var i: int = 2  // Local to the is_prime function



9. Example Program Analysis
The provided main function demonstrates the language's features:
fn main(): void =>
  var num: int = 5
  var fact: int = factorial(num)
  print($"Factorial of {num} is {fact}")

  if is_prime(7) =>
    print("7 is prime")
  else =>
    print("7 is not prime")

  var repeated: str = repeat_string("hello ", 3)
  print(repeated + "world!")

  var sum: int = 0
  for var k: int = 1; k <= 10; k++ =>
    sum = sum + k
  print($"Sum 1 to 10: {sum}")

  var pi_approx: double = 3.14159
  print($"Pi approx: {pi_approx}")

  var ch: char = 'A'
  print($"Char: {ch}")

  var flag: bool = true
  print($"Flag: {flag}")

  var any: str = $"This is a thing {num}"
  print(any)
  ...


Declares variables of types int, double, char, bool, and str.
Calls functions (factorial, is_prime, repeat_string).
Uses string interpolation for output.
Demonstrates control structures (if, for).

10. Implementation Notes for C Compiler
To implement a compiler in C for this language:

Lexer/Parser: Use a lexer (e.g., Flex) to tokenize keywords (fn, var, if, etc.), operators, and literals. Use a parser (e.g., Bison) to handle the indentation-based grammar and => syntax.
Type System: Implement a type checker to enforce type annotations (int, str, etc.) and ensure type safety in operations.
Code Generation: Generate C or LLVM IR code, mapping language constructs to equivalent C code. For example, the => blocks can be translated to C's {} blocks.
Runtime: Provide a runtime library for string interpolation (using a format string approach) and the print function (mapping to printf).
Memory Management: Strings (str) may require dynamic memory management in C (e.g., using malloc and free).
Error Handling: Ensure the compiler reports meaningful errors for type mismatches, undefined variables, or incorrect indentation.

11. Limitations and Assumptions

The language does not appear to support arrays, structs, or pointers in the provided example.
No explicit support for exception handling or advanced data structures.
The string concatenation operator (+) is supported for strings, but its behavior for other types is undefined.
The language assumes single-threaded execution, as no concurrency features are shown.

12. Future Extensions
Potential extensions to the language could include:

Support for arrays or lists (e.g., var arr: [int]).
Function overloading or default parameters.
Modules or imports for code organization.
Additional control structures like switch or try-catch.

This specification provides a foundation for implementing a compiler in C that can process the given language's syntax and semantics. For further details or clarifications, refer to the example code or contact the language designer.