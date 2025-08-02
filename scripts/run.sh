#!/bin/bash

set -ex

bin/sn samples/hello-world/test.sn -o bin/hello-world.asm -l 1 &> log/run-output.log

nasm -g -F dwarf -f elf64 bin/hello-world.asm -o bin/hello-world.o &> log/nasm-output.log

#gcc -no-pie -fsanitize=address -fno-omit-frame-pointer -g bin/arena.o bin/debug.o bin/runtime.o bin/hello-world.o -o bin/hello-world &> log/gcc-output.log
gcc -no-pie -g bin/arena.o bin/debug.o bin/runtime.o bin/hello-world.o -o bin/hello-world &> log/gcc-output.log

bin/hello-world &> log/hello-world-output.log || true
