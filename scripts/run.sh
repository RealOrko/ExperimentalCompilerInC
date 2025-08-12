#!/bin/bash

set -euo pipefail

bin/sn samples/main.sn -o bin/hello-world.c -l 1 &> log/run-output.log

gcc -no-pie -fsanitize=address -fno-omit-frame-pointer -g -Wall -Wextra -std=c99 -D_GNU_SOURCE bin/hello-world.c bin/arena.o bin/debug.o bin/runtime.o -o bin/hello-world &> log/gcc-output.log

bin/hello-world &> log/hello-world-output.log
cat log/hello-world-output.log
