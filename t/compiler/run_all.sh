#! /usr/bin/env bash

for x in t/compiler/test_* ; do
    echo -n "$x: " >&2
    if ./$x ; then
        echo "ok" >&2
    else
        echo "fail" >&2
    fi
done
