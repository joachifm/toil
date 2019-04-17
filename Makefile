CC = cc
CFLAGS += -MMD

.PHONY: all
all: polish toil

-include *.d
