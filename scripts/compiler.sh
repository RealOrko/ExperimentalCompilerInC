#!/bin/bash

set -e  # Exit on any error

# Clean up specific files instead of rm -rf bin/
rm -f bin/*

# Create bin directory if it doesn't exist
mkdir -p bin/

# Build the compiler
pushd compiler/
make
popd

# Compile SN source to assembly under Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=bin/valgrind-compiler.txt \
         bin/sn samples/hello-world/simple.sn -o bin/hello-world.asm -l 4 &> bin/compiler-output.txt

# Assemble with NASM (specify elf64 format)
nasm -f elf64 bin/hello-world.asm -o bin/hello-world.o

# Link with GCC (which handles C runtime properly, add frame pointer for better traces)
gcc -no-pie -fsanitize=address,undefined,leak -fno-omit-frame-pointer -g bin/hello-world.o -o bin/hello-world

# Run the executable (with ASan enabled)
bin/hello-world || true

# Optional: Run Valgrind on the final binary for extra checks (comment out if not needed)
valgrind --leak-check=full --track-origins=yes --log-file=bin/valgrind-binary.txt bin/hello-world || true

# Display outputs for easy checking
echo "Compiler Valgrind output:"
cat bin/valgrind-compiler.txt
echo "Compiler stdout/stderr:"
cat bin/compiler-output.txt
echo "Binary execution complete."

# Cleanup
rm -f compiler/*.o
