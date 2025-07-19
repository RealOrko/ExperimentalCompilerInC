#!/bin/bash

set -e  # Exit on any error

export INPUT_CODE=$(cat samples/hello-world/main.sn)
export MAKE_FILE=$(cat compiler/Makefile)
export BUILD_SCRIPT=$(cat scripts/build.sh)
export RUN_SCRIPT=$(cat scripts/run.sh)
export BUILD_OUTPUT=$(cat log/build-output.log)
export RUN_OUTPUT=$(cat log/run-output.log)
export PROMPT_TEMPLATE=$(cat scripts/prompt.template)

cat scripts/prompt.template | envsubst > log/prompt.txt
