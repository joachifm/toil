# !/usr/bin/env bash

cc -o cgen_demo cgen.c

while read -r line ; do
    in=$(cut -d ' ' -f1 <<< "$line")
    ex=$(cut -d ' ' -f2 <<< "$line")
    ou=$(./cgen_demo <<< "$in")
    if [[ "$ou" = "Item {type: CON, val: $ex}" ]] ; then
        echo "ok" >&2
    else
        echo "err: expected $ex; got $ou" >&2
    fi
done
