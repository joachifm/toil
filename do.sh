#! /bin/sh

mkdir -p build/include build/bin
./gen_label_table.sh > build/include/labels_gen.h
c++ -pipe -Ibuild/include $CPPFLAGS -o build/bin/toilc $CXXFLAGS @cxxflags unity.cc
