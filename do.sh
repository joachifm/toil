#! /bin/sh

mkdir -p build/include build/bin
c++ -Ibuild/include -o build/bin/toilc $CXXFLAGS main.cc
