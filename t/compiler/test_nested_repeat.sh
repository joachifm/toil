#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM nestedrepeat

VAR x INT
VAR y INT

x := 10
REPEAT
  x := x - 1

  y := 0
  WHILE y < 10
    y := y + 1
  ENDWHILE

UNTIL x = 0

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 0$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)y' a.out | grep -q '$1 = 10$'
