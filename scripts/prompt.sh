#!/bin/bash

set -e

awk '
BEGIN {
    # Map placeholders (as they appear in the template) to their file paths
    files["${INPUT_CODE}"] = "samples/main.sn"
    files["${MAKE_FILE}"] = "compiler/Makefile"
    files["${BUILD_SCRIPT}"] = "scripts/build.sh"
    files["${RUN_SCRIPT}"] = "scripts/run.sh"
    files["${BUILD_OUTPUT}"] = "log/build-output.log"
    files["${RUN_OUTPUT}"] = "log/run-output.log"
    files["${NASM_OUTPUT}"] = "log/nasm-output.log"
    files["${GCC_OUTPUT}"] = "log/gcc-output.log"
    files["${HELLO_WORLD_OUTPUT}"] = "log/hello-world-output.log"
}
{
    # Trim whitespace from the line for matching (in case of extra spaces)
    trimmed = $0
    gsub(/^[ \t]+|[ \t]+$/, "", trimmed)
    
    if (trimmed in files) {
        # If the line matches a placeholder, read and print the file content line by line
        while ((getline line < files[trimmed]) > 0) {
            print line
        }
        close(files[trimmed])  # Close the file to avoid too many open files
    } else {
        # Otherwise, print the template line as-is
        print $0
    }
}
' scripts/prompt.template > log/prompt.txt
