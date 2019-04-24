#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM basicarithsub

VAR x INT

x := 2
x := x - 2

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 0$'
