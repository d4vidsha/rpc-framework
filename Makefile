CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lpthread -lm

SRC=rpc.c hashtable.c linkedlist.c sockets.c protocol.c
OBJ = $(SRC:.c=.o)

RPC_SYSTEM_A=rpc.a
RPC_SERVER=rpc-server
RPC_CLIENT=rpc-client

.PHONY: all format clean

all: $(RPC_SYSTEM_A) $(RPC_SERVER) $(RPC_CLIENT)

$(RPC_SYSTEM_A): $(OBJ)
	ar rcs $@ $^

$(RPC_SERVER): server.c $(RPC_SYSTEM_A)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(RPC_CLIENT): client.c $(RPC_SYSTEM_A)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

format:
	clang-format -style=file -i *.c *.h

clean:
	rm -f $(RPC_SYSTEM_A) $(RPC_SERVER) $(RPC_CLIENT) $(OBJ)