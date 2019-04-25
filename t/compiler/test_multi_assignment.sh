#! /usr/bin/env bash

set -euo pipefail

./tfront <<EOF
PROGRAM multivar

VAR x INT
VAR y INT
VAR z INT

x := 1
y := 2
z := x

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)z' a.out | grep -q '$1 = 1$'
