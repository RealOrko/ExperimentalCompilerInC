#!/bin/bash

set -ex  # Exit on any error

# Clean up specific files instead of rm -rf bin/
rm -f bin/*
rm -f log/*

# Create bin directory if it doesn't exist
mkdir -p bin/
mkdir -p log/

# Build the compiler
pushd compiler/
make clean
make &> ../log/build-output.log
popd
