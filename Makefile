CC = cc
CFLAGS += -MMD

.PHONY: all
all: c

c: c.o lex.o

-include *.d
