#! /usr/bin/env bash

set -euo pipefail

./tfront <<EOF
PROGRAM multivar

VAR x INT
VAR y INT
VAR z INT

x := 1 + 2 * 3
y := ( 1 + 2 ) * 3
z := ( 1 + 2 ) * ( 1 + 2)

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 7$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)y' a.out | grep -q '$1 = 9$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)z' a.out | grep -q '$1 = 9$'
