#ifndef RPC_H
#define RPC_H

#include <stddef.h>

/*
 * Server and client states.
 */
typedef struct rpc_server rpc_server;
typedef struct rpc_client rpc_client;

/*
 * The payload for requests/responses.
 */
typedef struct {
    int data1;
    size_t data2_len;
    void *data2;
} rpc_data;

/*
 * Handle for remote function.
 */
typedef struct rpc_handle rpc_handle;

/*
 * Handler for remote functions, which takes rpc_data* as input and produces
 * rpc_data* as output.
 */
typedef rpc_data *(*rpc_handler)(rpc_data *);

/* ---------------- */
/* Server functions */
/* ---------------- */
rpc_server *rpc_init_server(int port);
int rpc_register(rpc_server *srv, char *name, rpc_handler handler);
void rpc_serve_all(rpc_server *srv);

/* ---------------- */
/* Client functions */
/* ---------------- */
rpc_client *rpc_init_client(char *addr, int port);
rpc_handle *rpc_find(rpc_client *cl, char *name);
rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload);
void rpc_close_client(rpc_client *cl);

/* ---------------- */
/* Shared functions */
/* ---------------- */
void rpc_data_free(rpc_data *data);

#endif
