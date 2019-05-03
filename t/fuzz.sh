#! /usr/bin/env bash

set -euo pipefail

CXXFLAGS="--coverage" ./do.sh
install -d build/objs
ghc -hidir build/objs -odir build/objs -o build/bin/fuzz -O2 --make Fuzz.hs

workdir=$(mktemp -d)

cleanup() {
    ret=$?
    echo "cleanup handler invoked: ret=$ret" >&2
    if [[ $ret -ne 0 ]] ; then
        echo "keeping workdir: $workdir" >&2
    else
        rm -f $workdir/*.txt
        rmdir --ignore-fail-on-non-empty $workdir
    fi
    trap - EXIT
    exit $ret
}
trap "cleanup" EXIT QUIT INT TERM

grind="valgrind --quiet --vgdb=no --exit-on-first-error=yes --error-exitcode=5 --undef-value-errors=yes --track-origins=yes"

echo "Fuzzing.  Press C-c to abort" >&2
echo -n "Progress: " >&2
while : ; do
    echo -n "#" >&2
    if ! timeout 1s ./build/bin/fuzz > $workdir/source.txt ; then
        continue
    fi
    if ! timeout 5s $grind ./toilc < $workdir/source.txt 2> $workdir/err.txt > $workdir/out.txt ; then
        ret=$?
        echo $ret > $workdir/ret.txt
        if grep -q "exhausted labels" $workdir/err.txt ; then
            : ignored exhausted labels error
        elif [[ $ret -eq 134 ]] ; then
            echo "time out!" >&2
            exit 1
        elif [[ $ret -eq 5 ]] ; then
            echo "valgrind error!" >&2
            exit 1
        else
            echo "failed to recognize input" >&2
            exit 1
        fi
    fi
done
