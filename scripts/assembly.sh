#!/bin/bash

set -e 

mkdir -p bin/

nasm -f elf64 bin/hello-world.asm -o bin/hello-world.o

gcc -no-pie bin/hello-world.o -o bin/hello-world

./bin/hello-world
