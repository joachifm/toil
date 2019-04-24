#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM basicdotimes

VAR x INT

x := 0
TIMES 10
  x := x + 1
ENDTIMES

END
EOF

gdb -batch \
    -ex 'break _end_of_program' \
    -ex 'run' \
    -ex 'p (int)x' a.out | grep -q '$1 = 10$'
