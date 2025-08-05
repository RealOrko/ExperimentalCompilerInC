#!/bin/bash

set -e

rm -f bin/*
rm -f log/*

mkdir -p bin/
mkdir -p log/

pushd compiler/
make clean
make &> ../log/build-output.log
make tests &>> ../log/build-output.log
popd

 . $PWD/scripts/test.sh
