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
valgrind --leak-check=full --track-origins=yes bin/sn samples/hello-world/simple.sn -o bin/hello-world.asm -l 4 &> bin/output.txt

# Assemble with NASM (specify elf64 format)
nasm -f elf64 bin/hello-world.asm -o bin/hello-world.o

# Link with GCC (which handles C runtime properly)
gcc -no-pie -fsanitize=address,undefined,leak bin/hello-world.o -o bin/hello-world

# Run the executable
#ASAN_OPTIONS=detect_leaks=1:leak_check_at_exit=1 
bin/hello-world || true

rm compiler/*.o

