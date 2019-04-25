#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM compoundstmt

VAR x INT

x := 0
TIMES 10
  TIMES 10
    x := 1
  ENDTIMES
ENDTIMES

END
EOF

gdb -batch \
    -ex 'break _end_of_program' \
    -ex 'run' \
    -ex 'p (int)x' a.out | grep -q '$1 = 1$'
