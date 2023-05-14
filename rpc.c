/* =============================================================================
   rpc.c

   Implementation of the RPC library.

   References:
   - How SIGINT is handled: https://stackoverflow.com/a/19059462
   - How to pass structs through sockets: https://stackoverflow.com/a/1577174

   Author: David Sha
============================================================================= */
#define _POSIX_C_SOURCE 200112L
#include "rpc.h"
#include "config.h"
#include "hashtable.h"
#include "linkedlist.h"
#include "protocol.h"
#include "sockets.h"
#include <arpa/inet.h>
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

static volatile sig_atomic_t keep_running = 1;

/*
 * Int handler for SIGINT.
 */
static void sig_handler(int _) {
    (void)_;
    keep_running = 0;
}

typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
} rpc_client_state;

/*
 * Handle a request from the client.
 *
 * @param srv The server state.
 * @param cl The client state.
 */
void handle_request(rpc_server *srv, rpc_client_state *cl);

/*
 * Handle all requests from the client.
 *
 * @param srv The server state.
 * @param cl The client state.
 * @param client_addr The client's address.
 */
void handle_all_requests(rpc_server *srv, rpc_client_state *cl);

/*
 * Shuts down the server and frees the server state.
 *
 * @param srv The server state.
 */
void rpc_shutdown_server(rpc_server *srv);

/*
 * Checks if a socket is closed.
 *
 * @param sockfd The socket file descriptor.
 * @return 1 if the socket is closed, 0 otherwise.
 */
int is_socket_closed(int sockfd);

/*
 * Print client's IP address and port number.
 *
 * @param cl The client state.
 */
void print_client_info(rpc_client_state *cl);

/*
 * Print the handler.
 *
 * @param data The handler to print.
 */
void print_handler(void *data);

/*
 * Create a new RPC handle.
 *
 * @param name: name of RPC handle
 * @return new RPC handle.
 */
rpc_handle *new_rpc_handle(const char *name);

struct rpc_server {
    int port;
    int sockfd;
    hashtable_t *handlers;
    list_t *clients;
};

rpc_server *rpc_init_server(int port) {

    // allocate memory for the server state
    rpc_server *srv;
    srv = (rpc_server *)malloc(sizeof(*srv));
    assert(srv);

    // convert port from int to a string
    char sport[MAX_PORT_LENGTH + 1];
    sprintf(sport, "%d", port);

    // initialise the server state
    srv->port = port;
    srv->sockfd = create_listening_socket(sport);
    srv->handlers = hashtable_create(HASHTABLE_SIZE);
    srv->clients = create_empty_list();

    return srv;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    // check if any of the parameters are NULL
    if (srv == NULL || name == NULL || handler == NULL) {
        return FAILED;
    }

    // if the handler already exists, remove it
    if (hashtable_lookup(srv->handlers, name) != NULL) {
        hashtable_remove(srv->handlers, name, NULL);
    }

    // add handler to a hashtable
    hashtable_insert(srv->handlers, name, handler);

    // get the handler back from the hashtable
    rpc_handler h = hashtable_lookup(srv->handlers, name);

    // check if the handler was successfully added
    if (h == NULL) {
        return FAILED;
    }

    fprintf(stdout, "Registered \"%s\" function handler\n", name);

    return EXIT_SUCCESS;
}

void rpc_serve_all(rpc_server *srv) {

    // check if the server is NULL
    if (srv == NULL) {
        return;
    }

    // handle SIGINT
    struct sigaction act = {.sa_handler = sig_handler};
    sigaction(SIGINT, &act, NULL);

    // keep running until SIGINT is received
    while (keep_running) {
        // listen on socket, incoming connection requests will be queued
        if (listen(srv->sockfd, BACKLOG) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        // accept a connection non-blocking using select
        int sockfd;
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof client_addr;
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(srv->sockfd, &readfds);
        struct timeval tv = {.tv_sec = 0, .tv_usec = 0};
        int retval = select(srv->sockfd + 1, &readfds, NULL, NULL, &tv);
        if (retval == FAILED) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (retval) {
            // connection request received
            sockfd = accept(srv->sockfd, (struct sockaddr *)&client_addr,
                            &client_addr_size);
            if (sockfd < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
        } else {
            // no connection requests received
            continue;
        }

        // store client information
        rpc_client_state *cl = (rpc_client_state *)malloc(sizeof(*cl));
        assert(cl);
        cl->sockfd = sockfd;
        cl->client_addr = client_addr;
        cl->client_addr_size = client_addr_size;

        // add to list of clients
        append(srv->clients, cl);

        // print the client connection information
        print_client_info(cl);

        // handle requests from the client
        handle_all_requests(srv, cl);
    }

    printf("\nShutting down...\n");
    rpc_shutdown_server(srv);
    return;
}

void print_client_info(rpc_client_state *cl) {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char ip_str[INET6_ADDRSTRLEN];

    if (getpeername(cl->sockfd, (struct sockaddr *)&addr, &addr_len) == -1) {
        perror("getpeername");
        return;
    }

    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, &(s->sin_addr), ip_str, sizeof(ip_str));
        fprintf(stdout, "Connected %s:%d on socket %d\n", ip_str,
                ntohs(s->sin_port), cl->sockfd);
    } else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        inet_ntop(AF_INET6, &(s->sin6_addr), ip_str, sizeof(ip_str));
        if (strcmp(ip_str, "::1") == 0) {
            fprintf(stdout, "Client %s:%d (localhost) connected on socket %d\n",
                    ip_str, ntohs(s->sin6_port), cl->sockfd);
        } else {
            fprintf(stdout, "Client %s:%d connected on socket %d\n", ip_str,
                    ntohs(s->sin6_port), cl->sockfd);
        }
    } else {
        fprintf(stdout, "Unknown address family\n");
    }
}

void handle_all_requests(rpc_server *srv, rpc_client_state *cl) {
    while (!is_socket_closed(cl->sockfd)) {
        fprintf(stdout, "==================================================\n");
        fprintf(stdout, "Waiting for request...\n");
        handle_request(srv, cl);
    }
}

void handle_request(rpc_server *srv, rpc_client_state *cl) {
    // check that socket is not closed
    if (is_socket_closed(cl->sockfd)) {
        fprintf(stdout, "Client disconnected\n");
        return;
    }

    // receive rpc_message from the client and process it
    rpc_message *msg;
    if ((msg = receive_rpc_message(cl->sockfd)) == NULL) {
        fprintf(stdout, "Client disconnected\n");
        return;
    }

    rpc_message *new_msg = NULL;
    switch (msg->operation) {

    case FIND:
        fprintf(stdout, "Received FIND request\n");
        fprintf(stdout, "Looking for handler: %s\n", msg->function_name);

        // get the handler from the hashtable
        rpc_handler h = hashtable_lookup(srv->handlers, msg->function_name);

        // check if the handler exists
        int exists = h != NULL;
        if (exists) {
            fprintf(stdout, "Handler found\n");
        } else {
            fprintf(stdout, "Handler not found\n");
        }
        new_msg = new_rpc_message(321, REPLY, new_string(msg->function_name),
                                  new_rpc_data(exists, 0, NULL));
        break;

    case CALL:
        fprintf(stdout, "Received CALL request\n");
        fprintf(stdout, "Calling handler: %s\n", msg->function_name);

        // get the handler from the hashtable
        rpc_handler handler =
            hashtable_lookup(srv->handlers, msg->function_name);

        // run the handler
        rpc_data *data = handler(msg->data);

        // create a new message to send back to the client
        new_msg =
            new_rpc_message(321, REPLY, new_string(msg->function_name), data);
        break;

    case REPLY:
        fprintf(stdout, "Received REPLY request\n");
        fprintf(stdout, "Doing nothing...\n");
        break;

    default:
        fprintf(stdout, "Received unknown request\n");
        fprintf(stdout, "Doing nothing...\n");
        break;
    }
    // send the message to the server
    send_rpc_message(cl->sockfd, new_msg);

    // free the message
    free_rpc_message(msg, rpc_data_free);
    free_rpc_message(new_msg, rpc_data_free);
}

int is_socket_closed(int sockfd) {
    char buf[1];
    ssize_t n = recv(sockfd, buf, sizeof(buf), MSG_PEEK);
    if (n == 0) {
        // socket is closed
        close(sockfd);
        return 1;
    } else if (n == -1) {
        perror("recv");
        return 0;
    } else {
        // socket is open
        return 0;
    }
}

void rpc_shutdown_server(rpc_server *srv) {

    // check if the server is NULL
    if (srv == NULL) {
        return;
    }

    // close the socket
    close(srv->sockfd);

    // free the hashtable
    hashtable_destroy(srv->handlers, NULL);

    // free the list of clients
    free_list(srv->clients, free);

    // free the server state
    free(srv);
    srv = NULL;
}

struct rpc_client {
    char *addr;
    int port;
    int sockfd;
};

struct rpc_handle {
    char name[MAX_NAME_LENGTH + 1];
};

rpc_client *rpc_init_client(char *addr, int port) {

    // check if any of the parameters are NULL
    if (addr == NULL || port < 0) {
        return NULL;
    }

    // allocate memory for the client state
    rpc_client *cl;
    cl = (rpc_client *)malloc(sizeof(*cl));
    assert(cl);

    // add the address and port to the client state
    cl->addr = new_string(addr);
    cl->port = port;

    // convert port from int to a string
    char sport[MAX_PORT_LENGTH + 1];
    sprintf(sport, "%d", port);

    // create a socket
    if ((cl->sockfd = create_connection_socket(addr, sport)) == FAILED) {
        free(cl->addr);
        free(cl);
        return NULL;
    }

    return cl;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {

    // check if any of the parameters are NULL
    if (cl == NULL || name == NULL) {
        return NULL;
    }

    // send message to the server and wait for a reply
    rpc_data *data = new_rpc_data(0, 0, NULL);
    rpc_message *reply =
        request(cl->sockfd, new_rpc_message(123, FIND, new_string(name), data));
    rpc_data_free(data);
    if (reply == NULL) {
        return NULL;
    }

    // check if the handler exists
    rpc_handle *h = NULL;
    if (reply->operation == REPLY && reply->data->data1 == TRUE) {
        h = new_rpc_handle(name);
    }

    // free the reply
    free_rpc_message(reply, rpc_data_free);

    return h;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    rpc_data *data = NULL;

    // check if any of the parameters are NULL
    if (cl == NULL || h == NULL || payload == NULL) {
        return NULL;
    }

    // use socket to send a message to the server
    rpc_message *reply = request(
        cl->sockfd, new_rpc_message(123, CALL, new_string(h->name), payload));
    if (reply == NULL) {
        return NULL;
    }

    // send the reply back to the client
    if (reply->operation == REPLY) {
        data = reply->data;
    }

    // free the reply but not the data
    free_rpc_message(reply, NULL);

    return data;
}

void rpc_close_client(rpc_client *cl) {

    // check if the client is NULL
    if (cl == NULL) {
        return;
    }

    // close the socket
    close(cl->sockfd);

    // free the address
    free(cl->addr);

    // free the client state
    free(cl);
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

void print_handler(void *data) {
    rpc_handler h = (rpc_handler)data;
    printf("%p\n", h);
}

rpc_handle *new_rpc_handle(const char *name) {
    rpc_handle *handle = (rpc_handle *)malloc(sizeof(*handle));
    assert(handle);
    strncpy(handle->name, name, MAX_NAME_LENGTH);
    return handle;
}
