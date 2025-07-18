#!/bin/bash

set -e  # Exit on any error

export INPUT_CODE=$(cat samples/hello-world/simple.sn)
export MAKE_FILE=$(cat compiler/Makefile)
export COMPILER_SCRIPT=$(cat scripts/compiler.sh)
export MAKE_LOG_OUTPUT=$(cat log/make-output.log)
export PROMPT_TEMPLATE=$(cat scripts/prompt.template)

cat scripts/prompt.template | envsubst > bin/prompt.txt
