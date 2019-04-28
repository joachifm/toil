#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM basicarith_add

VAR x INT
VAR y INT

x := (4 - 4) OR  (4 - 2)
y := (4 - 2) OR  (2 - 4)

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 2$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)y' a.out | grep -q '$1 = 2$'
