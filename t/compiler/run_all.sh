#! /usr/bin/env bash

declare -i expected_fail_count=0
declare -i fail_count=0

for x in t/compiler/test_* ; do
    echo -n "$x: " >&2
    if ./$x ; then
        echo "ok" >&2
    else
        echo "fail" >&2
        ((++fail_count))
    fi
done

if (($fail_count > $expected_fail_count)) ; then
    echo "exceeded expected failure count" >&2
    exit 1
fi
