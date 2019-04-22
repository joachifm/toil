#! /bin/sh

./gen_label_table.sh > labels_gen.h
c++ -DPARSER_TEST_MAIN -fvisibility=hidden -fno-exceptions -fno-rtti -o parser_test parser.cc
