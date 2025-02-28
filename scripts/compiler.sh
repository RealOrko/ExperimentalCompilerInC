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
         bin/sn samples/hello-world/simple.sn -o bin/hello-world.asm
