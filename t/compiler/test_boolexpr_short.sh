#! /usr/bin/env bash
set -euo pipefail

# TODO more robust to use a side-effecting op at rhs and check flags

./tfront <<EOF
PROGRAM basicarith_div

VAR x INT
VAR y INT

x := (1 > 0) OR (1 < 0)
y := (1 < 0) AND (1 > 0)

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 1$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)y' a.out | grep -q '$1 = 0$'
