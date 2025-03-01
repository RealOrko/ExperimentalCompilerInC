#!/bin/bash
rm -rf bin/

pushd compiler/
make
popd

valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=bin/valgrind-out.txt \
         bin/sn samples/hello-world/main.sn -o bin/hello-world.asm -l 3

nasm bin/hello-world.asm -o bin/hello-world.o
ld bin/hello-world.o -o bin/hello-world
bin/hello-world
