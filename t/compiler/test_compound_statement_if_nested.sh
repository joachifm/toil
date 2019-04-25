#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM compoundstmt

VAR x INT

x := 1
IF x
  x := 1
  IF x
    x := 0
  ELSE
    x := 1
  ENDIF
ELSE
  x := 0
ENDIF

END
EOF

gdb -batch \
    -ex 'break _end_of_program' \
    -ex 'run' \
    -ex 'p (int)x' a.out | grep -q '$1 = 0$'
