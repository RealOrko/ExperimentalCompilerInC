Language Specification for Custom C Compiler
This document outlines the language specification for a simple programming language designed to be compiled by a C-based compiler. The language supports basic constructs for functions, conditionals, loops, variables, and string interpolation, as demonstrated in the provided example code.
1. Syntax Overview
The language uses a concise, indentation-based syntax with => as a block delimiter, inspired by functional and scripting languages. It supports basic data types, functions, control flow, and string interpolation.
2. Data Types
The language supports the following primitive data types:

int: Integer values (e.g., 5, -10).
bool: Boolean values (true, false).
str: Strings for text data (e.g., "hello").
double: Floating-point numbers (e.g., 3.14159).
char: Single characters (e.g., 'A').

3. Variable Declarations
Variables are declared using the var keyword, followed by the variable name, type annotation, and optional initialization:
var name: type = value


Example: var num: int = 5
Variables can be reassigned to values of the same type: any = $"New value {num}".

4. Functions
Functions are defined using the fn keyword, followed by the function name, parameters with type annotations, return type, and body:
fn name(param1: type1, param2: type2): return_type =>
  // body


Parameters are declared with names and types: (n: int).
The return type is specified after a colon: : int.
The body is enclosed in => and uses return to yield a value.
Example:fn factorial(n: int): int =>
  if n <= 1 =>
    return 1
  return n * factorial(n - 1)



5. Control Flow
5.1 Conditionals
Conditional statements use if, else, and => for block delimitation:
if condition =>
  // true branch
else =>
  // false branch


Example:if is_prime(7) =>
  print("7 is prime")
else =>
  print("7 is not prime")



5.2 Loops
While Loop
while condition =>
  // body


Example:while i * i <= num =>
  if num % i == 0 =>
    return false
  i = i + 1



For Loop
for var name: type = start; condition; increment =>
  // body


Example:for var j: int = 0; j < count; j++ =>
  result = result + text



6. String Interpolation
Strings support interpolation using the $ prefix and {} for expressions:
$"text {expression}"


Example: $"Factorial of {num} is {fact}"
Interpolated expressions can include variables of any type (int, str, double, char, bool).

7. Operators

Arithmetic: +, -, *, /, % (modulo).
Comparison: ==, <=, >=, <, >.
Logical: Not explicitly shown but assumed to include &&, ||, !.
Assignment: =.
Increment: ++ (used in for loops).

8. Built-in Functions

print: Outputs a string to the console.
Example: print("Hello, world!")
Supports string interpolation: print($"Sum: {sum}").



9. Function Examples
Factorial
Recursive function to compute factorial:
fn factorial(n: int): int =>
  if n <= 1 =>
    return 1
  return n * factorial(n - 1)

Is Prime
Checks if a number is prime:
fn is_prime(num: int): bool =>
  if num <= 1 =>
    return false
  var i: int = 2
  while i * i <= num =>
    if num % i == 0 =>
      return false
    i = i + 1
  return true

Repeat String
Repeats a string a specified number of times:
fn repeat_string(text: str, count: int): str =>
  var result: str = ""
  for var j: int = 0; j < count; j++ =>
    result = result + text
  return result

10. Main Function
The main function serves as the entry point, with a return type of void:
fn main(): void =>
  // program logic


Example usage includes variable declarations, function calls, loops, and printing.

11. Example Program Output
Running the provided program would produce output like:
Factorial of 5 is 120
7 is prime
hello hello hello world!
Sum 1 to 10: 55
Pi approx: 3.14159
Char: A
Flag: true
This is a thing 5
This is a thing 120
This is a thing 55
This is a thing 3.14159
This is a thing A
This is a thing true

12. Implementation Notes for C Compiler

Lexical Analysis: Tokenize keywords (fn, var, if, else, while, for, return), types (int, bool, str, double, char), operators, and identifiers.
Parsing: Use a recursive descent parser to handle the => block structure and type annotations.
Type Checking: Ensure type consistency for assignments, function calls, and returns.
Code Generation: Map to C constructs (e.g., functions to C functions, str to char*, print to printf).
String Interpolation: Implement a runtime function to handle $"..." by parsing {} expressions and converting them to strings.

This specification provides a foundation for building a C-based compiler for this language.