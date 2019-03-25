CC = gcc
CFLAGS := -pipe -Ofast -g -std=c99 -Wall

.PHONY: all
all: c

c: c.o
