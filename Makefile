CC=gcc
CFLAGS=-Wall -g
LDFLAGS=-lpthread -lm

BUILD_DIR=build
SRC_DIR=src
INCLUDE_DIR=includes
EXAMPLES_DIR=examples

SRC=$(wildcard $(SRC_DIR)/*.c)
OBJ=$(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC))

RPC_SYSTEM_A=rpc.a
RPC_SERVER=rpc-server
RPC_CLIENT=rpc-client

.PHONY: all format clean

all: directories $(RPC_SYSTEM_A) $(RPC_SERVER) $(RPC_CLIENT)

$(RPC_SYSTEM_A): $(OBJ)
	ar rcs $@ $^

$(RPC_SERVER): $(EXAMPLES_DIR)/server.c $(RPC_SYSTEM_A)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ $^ $(LDFLAGS)

$(RPC_CLIENT): $(EXAMPLES_DIR)/client.c $(RPC_SYSTEM_A)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c -o $@ $<

directories:
	mkdir -p $(BUILD_DIR)

format:
	clang-format -style=file -i $(SRC_DIR)/*.c $(INCLUDE_DIR)/*.h

clean:
	rm -rf $(BUILD_DIR) $(RPC_SYSTEM_A) $(RPC_SERVER) $(RPC_CLIENT)
