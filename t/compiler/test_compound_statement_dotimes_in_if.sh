#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM compoundstmt

VAR x INT

x := 0
IF x
  TIMES 10
    x := 0
  ENDTIMES
ELSE
  TIMES 10
    x := 1
  ENDTIMES
ENDIF

END
EOF

gdb -batch \
    -ex 'break _end_of_program' \
    -ex 'run' \
    -ex 'p (int)x' a.out | grep -q '$1 = 1$'
