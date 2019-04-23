#! /bin/sh

mkdir -p build/include build/bin
./gen_label_table.sh > build/include/labels_gen.h
c++ -Ibuild/include -o build/bin/toilc main.cc
