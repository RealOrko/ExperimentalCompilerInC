#!/bin/bash

set -ex 

rm -f bin/*
rm -f log/*

mkdir -p bin/
mkdir -p log/

pushd compiler/
make clean
make &> ../log/build-output.log
popd
