#! /usr/bin/env bash
testlog=$(mktemp)
for x in ./type_decl_*.txt ; do
    name=$(basename "$x" .txt)
    echo -n "running test: $name ... " >&2
    tcc -run ../c.c < "$x" &> "$testlog"
    ret=$?
    if [[ $ret != 0 ]] ; then
        echo "failed" >&2
        echo "raw test output:" >&2
        cat "$testlog"
    else
        echo "ok" >&2
        rm -f "$testlog"
    fi
done
