/* =============================================================================
   rpc.c

   Implementation of the RPC library.

   References:
   - How SIGINT is handled: https://stackoverflow.com/a/54267342

   Author: David Sha
============================================================================= */
#define _POSIX_C_SOURCE 200112L
#include "rpc.h"
#include "config.h"
#include "hashtable.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

void int_handler(int _);
int create_listening_socket(char *port);
void print_handler(void *data);

static volatile sig_atomic_t keep_running = 1;

/*
 * Int handler for SIGINT.
 */
void int_handler(int _) {
    (void)_;
    keep_running = 0;
}

struct rpc_server {
    int port;
    int sockfd;
    hashtable_t *handlers;
};

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
    srv->handlers = hashtable_create(HASHTABLE_SIZE);

    return srv;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {

    // check if any of the parameters are NULL
    if (srv == NULL || name == NULL || handler == NULL) {
        return FAILED;
    }

    // add handler to a hashtable
    hashtable_insert(srv->handlers, name, handler);

    // get the handler back from the hashtable
    rpc_handler h = hashtable_lookup(srv->handlers, name);

    // check if the handler was successfully added
    if (h == NULL) {
        return FAILED;
    }

    return EXIT_SUCCESS;
}

void rpc_serve_all(rpc_server *srv) {

    // check if the server is NULL
    if (srv == NULL) {
        return;
    }

    // handle SIGINT
    signal(SIGINT, int_handler);

    // keep running until SIGINT is received
    while (keep_running) {
        // listen on socket, incoming connection requests will be queued
        if (listen(srv->sockfd, BACKLOG) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        // accept a connection
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        int newsockfd =
            accept(srv->sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
        if (newsockfd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // print the client's IP address
        getpeername(newsockfd, (struct sockaddr *)&client_addr, &client_addr_size);
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ipstr, sizeof ipstr);
        int port;
        port = ntohs(client_addr.sin_port);
        printf("Received connection from %s:%d on socket %d\n", ipstr, port, newsockfd);

        // read characters from the connection, then process
        char buffer[BUFSIZ];
        int n = read(newsockfd, buffer, BUFSIZ);
        if (n < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        // null-terminate the string
        buffer[n] = '\0';



    }

    printf("\nShutting down...\n");
    return;
}

struct rpc_client {
    char *addr;
    int port;
    int sockfd;
};

struct rpc_handle {};

rpc_client *rpc_init_client(char *addr, int port) {

    // check if any of the parameters are NULL
    if (addr == NULL || port < 0) {
        return NULL;
    }

    // allocate memory for the client state
    rpc_client *cl = malloc(sizeof(rpc_client));
    if (cl == NULL) {
        perror("malloc");
        return NULL;
    }

    // add the address and port to the client state
    cl->addr = addr;
    cl->port = port;

    // convert port from int to a string
    char sport[MAX_PORT_LENGTH + 1];
    sprintf(sport, "%d", port);

    // create a socket


    return NULL;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {

    // check if any of the parameters are NULL
    if (cl == NULL || name == NULL) {
        return NULL;
    }

    return NULL;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {

    return NULL;
}

void rpc_close_client(rpc_client *cl) {

    return;
}

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

/*
 * Create a socket given an address and port number.
 *
 * @param addr The address to connect to.
 * @param port The port number to connect to.
 * @return A file descriptor for the socket.
 * @note This function was adapted from week 9 tute.
 */
int create_connection_socket(char *addr, char *port) {
    int s, sockfd;
    struct addrinfo hints, *res;

    // create address we're going to connect to
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;      // use IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP sockets

    // get address info for addr
    if ((s = getaddrinfo(addr, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // create socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // connect to remote host
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    return sockfd;
}

/*
 * Print the handler.
 *
 * @param data The handler to print.
 */
void print_handler(void *data) {
    rpc_handler h = (rpc_handler)data;
    printf("%p\n", h);
}