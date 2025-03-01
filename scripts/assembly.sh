#!/bin/bash

set -e  # Exit on any error

# Create bin directory if it doesn't exist
mkdir -p bin/

# Assemble with NASM (specify elf64 format)
nasm -f elf64 bin/hello-world.asm -o bin/hello-world.o

# Link with GCC (which handles C runtime properly)
gcc -no-pie bin/hello-world.o -o bin/hello-world

# Run the executable
./bin/hello-world
