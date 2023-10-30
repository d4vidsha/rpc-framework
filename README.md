# Remote Procedure Call (RPC) System

In this project, we will implement a remote procedure call (RPC) system using a client-server architecture.

## Getting Started

`client.c` and `server.c` are the client and server programs which use the RPC system. The full API of the RPC system is defined in `rpc.h`, while the RPC system is implemented in `rpc.c`.

### Quick Start

```bash
# compile the client and server programs
make

# run the server program on port 8080
./rpc-server -p 8080

# in another terminal, run the client program on ipv6 loopback address ::1 on port 8080
./rpc-client -i ::1 -p 8080
```

```bash
# clean up the compiled files
make clean

# format the source code
make format
```

### Usage

#### Server

```bash
./rpc-server [-p port]
```

The server program will listen for incoming connections on the specified port. If no port is specified, then the server will listen on port 3000.

#### Client

```bash
./rpc-client [-i ip_address] [-p port]
```

The client program will connect to the specified IP address and port. If no IP address is specified, then the client will connect to the ipv6 loopback address `::1`. If no port is specified, then the client will connect to port 3000.

### Development

If you want to debug the RPC system, then `#define DEBUG TRUE` in `config.h`. This will print out debug messages to `stdout`.

Ensure you are using Valgrind frequently to check for memory leaks:

```bash
valgrind --leak-check=full --show-leak-kinds=all ./rpc-server -p 8080
valgrind --leak-check=full --show-leak-kinds=all ./rpc-client -i ::1 -p 8080
```

## About the RPC System

### Protocol Design

We will use a single `rpc_message` struct to represent both requests and responses.

#### Operations

1. Client sends `rpc_message` to server.
2. Server receives `rpc_message`.
3. Server sends a new `rpc_message` to client.
4. Client receives `rpc_message`.

#### Message Structure

The `rpc_message` struct will contain the following fields:
| Field           | Data Type  | Description                                                                                                                                                               |
|-----------------|------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `request_id`    | `int`      | The ID of the request. Useful for matching requests to responses. The current implementation does not use this but including this may be useful for future extendibility. |
| `op`            | `enum`     | The operation can be either FIND, CALL, REPLY_SUCCESS, or REPLY_FAILURE.                                                                                                  |
| `function_name` | `char *`   | The name of the function to be called or returned.                                                                                                                        |
| `data`          | `rpc_data` | The data to be passed to the function or returned by the function.                                                                                                        |

This `rpc_message` struct will be serialised into a byte array and sent over the network. When serialising, we already ensure that there are no padding or endianness issues.

## Notable Mentions

- A hash table is used to store the function pointers in the server program.
- A protocol with a request and response message structure is used to communicate between the client and server programs. The serialisation and deserialisation of the messages are done through functions in `protocol.c`.
- Elias Gamma Coding is used for the serialisation and deserialisation of `size_t` data types.
