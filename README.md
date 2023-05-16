# Remote Procedure Call (RPC) System

In this project, we will implement a remote procedure call (RPC) system using a client-server architecture.

## Getting Started

`client.c` and `server.c` are the client and server programs which use the RPC system. The full API of the RPC system is defined in `rpc.h`, while the RPC system is implemented in `rpc.c`.

## Notable Mentions

- A hash table is used to store the function pointers in the server program.
- A protocol with a request and response message structure is used to communicate between the client and server programs. The serialisation and deserialisation of the messages are done through functions in `protocol.c`.
