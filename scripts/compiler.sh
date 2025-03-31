#!/bin/bash

# valgrind --leak-check=full \
#          --show-leak-kinds=all \
#          --track-origins=yes \
#          --verbose \
#          --log-file=bin/valgrind-out.txt \
#          bin/sn samples/hello-world/main.sn -o bin/hello-world.asm -v -l 3


rm -rf bin/
set -e  # Exit on any error

# Create bin directory if it doesn't exist
mkdir -p bin/

# Build the compiler
pushd compiler/
make
popd

# Compile SN source to assembly
bin/sn samples/hello-world/main.sn -o bin/hello-world.asm -l 4

# Assemble with NASM (specify elf64 format)
nasm -f elf64 bin/hello-world.asm -o bin/hello-world.o

# Link with GCC (which handles C runtime properly)
gcc -no-pie -fsanitize=address bin/hello-world.o -o bin/hello-world

# Run the executable
#bin/hello-world
