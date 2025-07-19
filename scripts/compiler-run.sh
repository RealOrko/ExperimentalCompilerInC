#!/bin/bash

set -ex  # Exit on any error

# Compile SN source to assembly under Valgrind
bin/sn samples/hello-world/simple.sn -o bin/hello-world.asm -l 3 &> log/compiler-run-output.log

# Assemble with NASM (specify elf64 format)
nasm -f elf64 bin/hello-world.asm -o bin/hello-world.o &> log/compiler-run-nasm-output.log

# Link with GCC (which handles C runtime properly, add frame pointer for better traces)
gcc -pie -fsanitize=address -fno-omit-frame-pointer -g bin/hello-world.o -o bin/hello-world &> log/compiler-run-gcc-output.log

# Run the executable (with ASan enabled)
bin/hello-world &> log/compiler-run-hello-world-output.log || true
