/* =============================================================================
   rpc.c

   Implementation of the RPC library.

   Author: David Sha
============================================================================= */
#define _POSIX_C_SOURCE 200112L
#include "rpc.h"
#include "config.h"
#include "linkedlist.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int create_listening_socket(char *port);

typedef struct handler {
    char *name;
    rpc_handler handler;
} handler_t;

struct rpc_server {
    int port;
    int sockfd;
    list_t *handlers;
};

/*
 * Initialises a rpc_server struct and binds to the given port.
 *
 * @param port The port to bind to.
 * @return A pointer to a rpc_server struct, or NULL on failure.
 * @note This function is called before rpc_register.
 */
rpc_server *rpc_init_server(int port) {

    // allocate memory for the server state
    rpc_server *srv = malloc(sizeof(rpc_server));
    if (srv == NULL) {
        perror("malloc");
        return NULL;
    }

    // convert port from int to a string
    char sport[MAX_PORT_LENGTH + 1];
    sprintf(sport, "%d", port);

    // initialise the server state
    srv->port = port;
    srv->sockfd = create_listening_socket(sport);
    srv->handlers = create_empty_list();

    return srv;
}

/*
 * Register a handler for a given name. At the server, let the
 * subsystem know what function to call when an incoming request
 * with the given name is received.
 *
 * @param srv The server to register the handler with.
 * @param name The name of the function.
 * @param handler The function to call when a request with the
 * given name is received.
 * @return 0 on success, FAILED on failure. If any of the parameters
 * are NULL, return FAILED.
 * @note TODO: Possibly use ID for the handler as the return value
 * instead of 0.
 */
int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {

    // check if any of the parameters are NULL
    if (srv == NULL || name == NULL || handler == NULL) {
        return FAILED;
    }

    // create a new handler struct
    handler_t *new_handler = malloc(sizeof(handler_t));
    if (new_handler == NULL) {
        perror("malloc");
        return FAILED;
    }

    // initialise the new handler
    new_handler->name = name;
    new_handler->handler = handler;

    return 0;
}

/*
 * Server function to handle incoming requests. This function will wait
 * for incoming requests for any registered functions, or rpc_find, on
 * the port specified in rpc_init_server of any interface. If it is a
 * function call request, it will call the requested function, send a
 * reply to the caller, and resume waiting for new requests. If it is
 * rpc_find, it will reply to the caller saying whether the name was
 * found or not, or possibly an error code.
 *
 * @param srv The server to serve requests for.
 * @note This function should only return if srv is NULL or handling
 * SIGINT.
 */
void rpc_serve_all(rpc_server *srv) {

    return;
}

struct rpc_client {};

struct rpc_handle {};

/*
 * Initialises a client state.
 *
 * @param addr The address of the server to connect to.
 * @param port The port of the server to connect to.
 * @return rpc_client*, or NULL on failure.
 * @note This function is called before rpc_find or rpc_call.
 */
rpc_client *rpc_init_client(char *addr, int port) {

    return NULL;
}

/*
 * Find the remote procedure with the given name.
 *
 * @param cl The client to use.
 * @param name The name of the function to call.
 * @return A handle for the remote procedure, or NULL if the name
 * is not registered or if any of the parameters are NULL, or if
 * the find request fails.
 */
rpc_handle *rpc_find(rpc_client *cl, char *name) {

    // check if any of the parameters are NULL
    if (cl == NULL || name == NULL) {
        return NULL;
    }

    return NULL;
}

/*
 * Call a remote procedure. This function causes the subsystem to
 * run the remote procedure, and returns the value.
 *
 * @param cl The client to use.
 * @param h The handle for the remote procedure to call.
 * @param payload The data to send to the remote procedure.
 * @return The data returned by the remote procedure, or NULL if
 * the call fails, or if any of the parameters are NULL.
 * @note The returned data should be freed by the caller using
 * rpc_data_free.
 */
rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {

    return NULL;
}

/*
 * Clean up the client state and close the connection. If an already
 * closed client is passed, do nothing.
 *
 * @param cl The client to close.
 * @note This function is called after the final rpc_call or rpc_find.
 */
void rpc_close_client(rpc_client *cl) {

    return;
}

/*
 * Free the memory allocated to rpc_data struct.
 *
 * @param data The struct to free.
 */
void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}

/*
 * Create a socket given a port number.
 *
 * @param port The port number to bind to.
 * @return A file descriptor for the socket.
 * @note This function was adapted from week 9 tute.
 */
int create_listening_socket(char *port) {
    int re, s, sockfd;
    struct addrinfo hints, *res;

    // create address we're going to listen on (with given port number)
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;      // use IPv6
    hints.ai_socktype = SOCK_STREAM; // use TCP
    hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept

    // get address info with above parameters
    if ((s = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // create socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // set socket to be reusable
    re = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof re) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind address to socket
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    return sockfd;
}