#! /usr/bin/env bash

set -euo pipefail

./tfront <<EOF
PROGRAM procdecl

VAR x INT
VAR y INT

PROC square_x
  x := x * x
END

PROC square_y
  y := y * y
END

square_x()

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 4$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)y' a.out | grep -q '$1 = 16$'
