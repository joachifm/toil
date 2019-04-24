#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM basicwhile

VAR x INT

x := 10

WHILE x
  x := x - 1
ENDWHILE

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 0$'
