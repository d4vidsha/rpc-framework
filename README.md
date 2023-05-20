# Remote Procedure Call (RPC) System

In this project, we will implement a remote procedure call (RPC) system using a client-server architecture.

## Getting Started

`client.c` and `server.c` are the client and server programs which use the RPC system. The full API of the RPC system is defined in `rpc.h`, while the RPC system is implemented in `rpc.c`.

## Notable Mentions

- A hash table is used to store the function pointers in the server program.
- A protocol with a request and response message structure is used to communicate between the client and server programs. The serialisation and deserialisation of the messages are done through functions in `protocol.c`.
- Elias Gamma Coding is used for the serialisation and deserialisation of `size_t` data types.

## Roadmap

- [ ] Mandle malloc errors without `assert()`, which exits the program.
- [ ] Limit concurrent connections to the server by using a queue. Currently, all open and closed connections (the `sockfd`s) are stored in a linked list.
- [ ] Use integer inside `rpc_handle` as opposed to a fixed-length array of characters.
