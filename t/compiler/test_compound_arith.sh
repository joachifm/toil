#! /usr/bin/env bash

set -euo pipefail

./tfront <<EOF
PROGRAM compoundarith

VAR x INT

x := (((2 + 2) - (1 + 1)) + 1) - 0

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 3$'
