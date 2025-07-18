#!/bin/bash

set -e  # Exit on any error

export SAMPLES_INPUT_CODE=$(cat samples/hello-world/simple.sn)
export COMPILER_MAKE_FILE=$(cat compiler/Makefile)
export SCRIPTS_COMPILER_BUILD_SCRIPT=$(cat scripts/compiler-build.sh)
export SCRIPTS_COMPILER_RUN_SCRIPT=$(cat scripts/compiler-run.sh)
export LOG_COMPILER_BUILD_OUTPUT=$(cat log/compiler-build-output.log)
export LOG_COMPILER_RUN_OUTPUT=$(cat log/compiler-run-output.log)
export PROMPT_TEMPLATE=$(cat scripts/prompt.template)

cat scripts/prompt.template | envsubst > bin/prompt.txt
