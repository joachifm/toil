#! /usr/bin/env bash
set -euo pipefail

./tfront <<EOF
PROGRAM basicarith_div

VAR x INT
VAR y INT
VAR z INT
VAR a INT

x := 1 < 0 OR 1 > 0
y := 1 < 0 OR 2 < 0
z := 1 > 0 AND 1 < 0
a := 1 > 0 AND 0 < 1

END
EOF

gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)x' a.out | grep -q '$1 = 1$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)y' a.out | grep -q '$1 = 0$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)z' a.out | grep -q '$1 = 0$'
gdb -batch -ex 'break _end_of_program' -ex 'run' -ex 'p (int)a' a.out | grep -q '$1 = 1$'
