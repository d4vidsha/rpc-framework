CC=gcc
CFLAGS=-Wall -g
LDLIBS=

RPC_SYSTEM=rpc.o
RPC_SYSTEM_A=rpc.a
RPC_SERVER=rpc-server
RPC_CLIENT=rpc-client

.PHONY: all format clean

all: $(RPC_SYSTEM) $(RPC_SYSTEM_A) $(RPC_SERVER) $(RPC_CLIENT)

$(RPC_SYSTEM): rpc.c rpc.h
	$(CC) -c $(CFLAGS) -o $@ $< $(LDLIBS)

$(RPC_SYSTEM_A): $(RPC_SYSTEM)
	ar rcs $@ $<

$(RPC_SERVER): server.c $(RPC_SYSTEM_A)
	$(CC) $(CFLAGS) -o $@ $^

$(RPC_CLIENT): client.c $(RPC_SYSTEM_A)
	$(CC) $(CFLAGS) -o $@ $^

format:
	clang-format -style=file -i *.c *.h

clean:
	rm -f $(RPC_SYSTEM) $(RPC_SYSTEM_A) $(RPC_SERVER) $(RPC_CLIENT)