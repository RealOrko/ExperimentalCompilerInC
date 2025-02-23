#!/bin/bash
set -x
rm -rf bin/
mkdir -p bin/tmp/
gcc -c compiler/lexer.c -o bin/tmp/lexer.o
gcc -c compiler/parser.c -o bin/tmp/parser.o
gcc -c compiler/codegen.c -o bin/tmp/codegen.o
gcc -c compiler/main.c -o bin/tmp/main.o
gcc bin/tmp/lexer.o bin/tmp/parser.o bin/tmp/codegen.o bin/tmp/main.o -o bin/sn
bin/sn samples/hello-world/main.sn -o bin/samples-hello-world
# cat output.asm
bin/samples-hello-world