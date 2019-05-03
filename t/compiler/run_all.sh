#! /usr/bin/env bash

declare -i expected_fail_count=0
declare -i fail_count=0
tmout=5s

export CXXFLAGS="--coverage"

for exe in ./t/compiler/test_* ; do
    [[ -x "$exe" ]] || continue
    echo -n "$exe: " >&2
    if timeout "$tmout" ./$exe ; then
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
