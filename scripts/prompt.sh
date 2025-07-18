#!/bin/bash

set -e  # Exit on any error

export SAMPLES_INPUT_CODE=$(cat samples/hello-world/simple.sn)
export COMPILER_MAKE_FILE=$(cat compiler/Makefile)
export SCRIPTS_COMPILER_SCRIPT=$(cat scripts/compiler.sh)
export LOG_MAKE_OUTPUT=$(cat log/make-output.log)
export LOG_COMPILER_OUTPUT=$(cat log/compiler-output.log)
export PROMPT_TEMPLATE=$(cat scripts/prompt.template)

cat scripts/prompt.template | envsubst > bin/prompt.txt
