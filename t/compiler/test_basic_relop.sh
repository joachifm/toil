#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM basicarith_div

VAR x INT
VAR y INT
VAR z INT

x := 4 > 2
y := 2 < 4
z := 4 = 4

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 1$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)y' a.out | grep -q '$1 = 1$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)z' a.out | grep -q '$1 = 1$'
