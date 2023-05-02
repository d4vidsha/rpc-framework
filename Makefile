CC=gcc
CFLAGS=-c -Wall -g
LDLIBS=
RPC_SYSTEM=rpc.o
RPC_SYSTEM_A=rpc.a

.PHONY: all format clean

all: $(RPC_SYSTEM) $(RPC_SYSTEM_A)

$(RPC_SYSTEM): rpc.c rpc.h
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

$(RPC_SYSTEM_A): $(RPC_SYSTEM)
	ar rcs $@ $<

format:
	clang-format -style=file -i *.c *.h

clean:
	rm -f $(RPC_SYSTEM) $(RPC_SYSTEM_A)
