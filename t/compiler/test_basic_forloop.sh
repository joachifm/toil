#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM basicforloop

VAR y INT

y := 0

FOR x FROM 0 TO 10
  y := y + 1
ENDFOR

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)y' a.out | grep -q '$1 = 10$'
