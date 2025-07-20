#!/bin/bash

set -ex  # Exit on any error

# Compile SN source to assembly under Valgrind
bin/sn samples/hello-world/main.sn -o bin/hello-world.asm -l 1 &> log/run-output.log

# Assemble with NASM (specify elf64 format)
nasm -g -F dwarf -f elf64 bin/hello-world.asm -o bin/hello-world.o &> log/nasm-output.log

# Link with GCC (which handles C runtime properly, add frame pointer for better traces)
# Will need pie at some point, but for now we use no-pie to avoid issues with ASan
#gcc -pie -fsanitize=address -fno-omit-frame-pointer -g bin/runtime.o -g bin/hello-world.o -o bin/hello-world &> log/gcc-output.log
gcc -no-pie -fsanitize=address -fno-omit-frame-pointer -g bin/runtime.o bin/hello-world.o -o bin/hello-world &> log/gcc-output.log

# Run the executable (with ASan enabled)
bin/hello-world &> log/hello-world-output.log || true
