#! /usr/bin/env bash

set -euo pipefail

./tfront <<EOF
PROGRAM procdecl

VAR x INT

PROC square_x
  x := x * x
ENDPROC

x := 2
square_x()

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 4$'
