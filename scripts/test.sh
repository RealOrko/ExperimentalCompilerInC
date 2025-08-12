#!/bin/bash

set -eou pipefail

bin/tests &> log/test-output.log
cat log/test-output.log

