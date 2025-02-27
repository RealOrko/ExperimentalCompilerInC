#!/bin/bash
set -x
rm -rf bin/

pushd compiler/
make
popd

bin/sn samples/hello-world/main.sn -o bin/hello-world
