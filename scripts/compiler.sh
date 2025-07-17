#!/bin/bash

set -ex  # Exit on any error

# Clean up specific files instead of rm -rf bin/
rm -f bin/*
rm -f log/*

# Create bin directory if it doesn't exist
mkdir -p bin/
mkdir -p log/

# Build the compiler
pushd compiler/
make clean
make &> ../log/make-output.txt
popd

# Compile SN source to assembly under Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=bin/valgrind-compiler.txt \
         bin/sn samples/hello-world/simple.sn -o bin/hello-world.asm -l 4 &> log/compiler-output.txt

# Assemble with NASM (specify elf64 format)
nasm -f elf64 bin/hello-world.asm -o bin/hello-world.o &> log/nasm-output.txt

# Link with GCC (which handles C runtime properly, add frame pointer for better traces)
gcc -no-pie -fsanitize=address,undefined,leak -fno-omit-frame-pointer -g bin/hello-world.o -o bin/hello-world &> log/gcc-output.txt

# Run the executable (with ASan enabled)
bin/hello-world &> log/hello-world-output.txt || true

# Optional: Run Valgrind on the final binary for extra checks (comment out if not needed)
valgrind --leak-check=full --track-origins=yes --log-file=bin/valgrind-binary.txt bin/hello-world &> log/hello-world-valgrind-output.txt || true
