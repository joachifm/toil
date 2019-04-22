#! /bin/sh

mkdir -p build/include build/bin
./gen_label_table.sh > build/include/labels_gen.h
c++ -DPARSER_TEST_MAIN -Ibuild/include -fvisibility=hidden -fno-exceptions -fno-rtti -o build/bin/parser_test parser.cc
