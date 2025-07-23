Language Specification for Custom C Compiler
This document outlines the specification for a simple programming language designed for a C-based compiler. The language features a clean, indentation-based syntax with => block delimiters, supporting functions, conditionals, loops, variables, and string interpolation.
Table of Contents

Syntax Overview
Data Types
Variable Declarations
Functions
Control Flow
String Interpolation
Operators
Built-in Functions
Function Examples
Main Function
Example Program Output
Implementation Notes

Syntax Overview
The language uses a concise syntax inspired by functional and scripting languages. Blocks are delimited by =>, and indentation is significant. It supports basic data types, control structures, and string interpolation for expressive programming.
Data Types
The language supports the following primitive types:

int: Integer values (e.g., 5, -10)
bool: Boolean values (true, false)
str: Strings for text (e.g., "hello")
double: Floating-point numbers (e.g., 3.14159)
char: Single characters (e.g., 'A')

Variable Declarations
Variables are declared with the var keyword, a name, type annotation, and optional initialization:
var name: type = value


Example: var num: int = 5
Variables can be reassigned to values of the same type:any = $"New value {num}"



Functions
Functions are defined with the fn keyword, parameters, return type, and a body:
fn name(param1: type1, param2: type2): return_type =>
  // body


Parameters include names and types: (n: int)
Return type follows a colon: : int
Body uses => and return for output
Example:fn factorial(n: int): int =>
  if n <= 1 =>
    return 1
  return n * factorial(n - 1)



Control Flow
Conditionals
Conditionals use if, else, and => for blocks:
if condition =>
  // true branch
else =>
  // false branch


Example:if is_prime(7) =>
  print("7 is prime")
else =>
  print("7 is not prime")



Loops
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



String Interpolation
Strings support interpolation with a $ prefix and {} for expressions:
$"text {expression}"


Example: $"Factorial of {num} is {fact}"
Supports variables of any type (int, str, double, char, bool).

Operators

Arithmetic: +, -, *, /, % (modulo)
Comparison: ==, <=, >=, <, >
Logical: &&, ||, ! (assumed)
Assignment: =
Increment: ++ (used in for loops)

Built-in Functions

print: Outputs a string to the console
Example: print("Hello, world!")
Supports interpolation: print($"Sum: {sum}")



Function Examples
Factorial
Computes the factorial of a number recursively:
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

Main Function
The main function is the program entry point with a void return type:
fn main(): void =>
  // program logic


Includes variable declarations, function calls, loops, and printing.

Example Program Output
Running the example program produces:
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

Implementation Notes

Lexical Analysis: Tokenize keywords (fn, var, if, else, while, for, return), types, operators, and identifiers.
Parsing: Use recursive descent for => blocks and type annotations.
Type Checking: Ensure type consistency for assignments, calls, and returns.
Code Generation: Map to C constructs (e.g., str to char*, print to printf).
String Interpolation: Implement a runtime function to parse {} expressions and convert to strings.

This specification provides a clear foundation for implementing a C-based compiler for this language.