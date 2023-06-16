# Remote Procedure Call (RPC) System

In this project, we will implement a remote procedure call (RPC) system using a client-server architecture.

## Getting Started

`client.c` and `server.c` are the client and server programs which use the RPC system. The full API of the RPC system is defined in `rpc.h`, while the RPC system is implemented in `rpc.c`.

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

## Roadmap

- [ ] Handle memory allocation errors without `assert()`, which will currently exit the program.
- [ ] Limit concurrent connections to the server by using a queue. Currently, all open and closed connections (the `sockfd`s) are stored in a linked list.
- [ ] Use integer inside `rpc_handle` as opposed to a fixed-length array of characters.
