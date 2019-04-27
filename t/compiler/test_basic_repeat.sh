#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM basicrepeat

VAR x INT

x := 10

REPEAT
  x := x - 1
UNTIL x + 0 = 0

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 0$'
