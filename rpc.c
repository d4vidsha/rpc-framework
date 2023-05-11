/* =============================================================================
   rpc.c

   Implementation of the RPC library.

   References:
   - How SIGINT is handled: https://stackoverflow.com/a/54267342
   - How to pass structs through sockets: https://stackoverflow.com/a/1577174

   Author: David Sha
============================================================================= */
#include "rpc.h"
#include "config.h"
#include "hashtable.h"
#include "sockets.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

static volatile sig_atomic_t keep_running = 1;

/*
 * Int handler for SIGINT.
 */
static void sig_handler(int _) {
    (void)_;
    keep_running = 0;
}


typedef struct {
    int request_id;
    enum {
        FIND,
        CALL,
        REPLY,
    } operation;
    rpc_handle *handle;
    rpc_data *data;
} rpc_message;


struct rpc_client {
    char *addr;
    int port;
    int sockfd;
};

struct rpc_handle {
    char *name;
    size_t size;
};


void print_handler(void *data);
rpc_message *request(int sockfd, rpc_message *msg);
int send_message(int sockfd, const unsigned char *msg, int size);
int receive_message(int sockfd, unsigned char *buffer, int size);

unsigned char *serialise_int(unsigned char *buffer, int value);
int deserialise_int(unsigned char *buffer);
unsigned char *serialise_size_t(unsigned char *buffer, size_t value);
size_t deserialise_size_t(unsigned char *buffer);
unsigned char *serialise_char(unsigned char *buffer, char value);
char deserialise_char(unsigned char *buffer);
unsigned char *serialise_string(unsigned char *buffer, const char *value);
char *deserialise_string(unsigned char *buffer);
unsigned char *serialise_rpc_handle(unsigned char *buffer, const rpc_handle *handle);
rpc_handle *deserialise_rpc_handle(unsigned char *buffer);
unsigned char *serialise_rpc_data(unsigned char *buffer, const rpc_data *data);
rpc_data *deserialise_rpc_data(unsigned char *buffer);
unsigned char *serialise_rpc_message(unsigned char *buffer, const rpc_message *message);
rpc_message *deserialise_rpc_message(unsigned char *buffer);





struct rpc_server {
    int port;
    int sockfd;
    hashtable_t *handlers;
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

    return srv;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {

    fprintf(stderr, "Added handler for %s\n", name);

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

    return EXIT_SUCCESS;
}

void rpc_serve_all(rpc_server *srv) {

    // check if the server is NULL
    if (srv == NULL) {
        return;
    }

    // handle SIGINT
    signal(SIGINT, sig_handler);

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
        int port = ntohs(client_addr.sin_port);
        printf("Received connection from %s:%d on socket %d\n", ipstr, port, newsockfd);

        // read characters from the connection, then process
        unsigned char buffer[BUFSIZ];
        int n = read(newsockfd, buffer, BUFSIZ);
        if (n < 0) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        
        // null-terminate the string
        buffer[n] = '\0';

        // print the bytes received
        printf("Received message: %s\n", buffer);




        // create a message to send to the server
        char *name = "test";
        rpc_message *msg;
        msg = (rpc_message *)malloc(sizeof(*msg));
        assert(msg);

        // set the request id
        msg->request_id = 0;

        // set the operation
        msg->operation = REPLY;

        // set the handler name and size
        msg->handle = (rpc_handle *)malloc(sizeof(*msg->handle));
        assert(msg->handle);
        msg->handle->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
        assert(msg->handle->name);
        strcpy(msg->handle->name, name);
        msg->handle->size = strlen(name) + 1;


        // send the message to the server
        // convert message to serialised form
        unsigned char newbuf[BUFSIZ];
        serialise_rpc_message(newbuf, msg);

        // send message
        send_message(newsockfd, newbuf, BUFSIZ);





        // // get the action to perform
        // char *action = strtok(buffer, " ");
        
        // // if action is to find a handler
        // if (strcmp(action, "find") == 0) {
        //     // get the name of the handler
        //     char *name = strtok(NULL, " ");
        //     // get the handler from the hashtable
        //     rpc_handler h = hashtable_lookup(srv->handlers, name);
        //     // if the handler exists
        //     char msg[BUFSIZ];
        //     if (h != NULL) {
        //         // send a message to the client
        //         sprintf(msg, "found %s", name);
        //         n = write(newsockfd, msg, strlen(msg));
        //         if (n < 0) {
        //             perror("write");
        //             exit(EXIT_FAILURE);
        //         }
        //     } else {
        //         // send a message to the client
        //         sprintf(msg, "notfound %s", name);
        //         n = write(newsockfd, msg, strlen(msg));
        //         if (n < 0) {
        //             perror("write");
        //             exit(EXIT_FAILURE);
        //         }
        //     }
        // }

    }

    printf("\nShutting down...\n");
    return;
}







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
    cl->addr = (char *)malloc(sizeof(char) * (strlen(addr) + 1));
    assert(cl->addr);
    strcpy(cl->addr, addr);
    cl->port = port;

    // convert port from int to a string
    char sport[MAX_PORT_LENGTH + 1];
    sprintf(sport, "%d", port);

    // create a socket
    cl->sockfd = create_connection_socket(addr, sport);

    return cl;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {

    // check if any of the parameters are NULL
    if (cl == NULL || name == NULL) {
        return NULL;
    }

    // create a message to send to the server
    rpc_message *msg;
    msg = (rpc_message *)malloc(sizeof(*msg));
    assert(msg);

    // set the request id
    msg->request_id = 0;

    // set the operation
    msg->operation = FIND;

    // set the handler name and size
    msg->handle = (rpc_handle *)malloc(sizeof(*msg->handle));
    assert(msg->handle);
    msg->handle->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
    assert(msg->handle->name);
    strcpy(msg->handle->name, name);
    msg->handle->size = strlen(name) + 1;

    // send message to the server and wait for a reply
    rpc_message *reply = request(cl->sockfd, msg);

    // check if the handler exists
    if (reply->operation == REPLY) {
        if (reply->data->data1 == TRUE) {
            // create a new rpc_handle
            rpc_handle *h;
            h = (rpc_handle *)malloc(sizeof(*h));
            assert(h);
            h->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
            assert(h->name);
            strcpy(h->name, name);
            h->size = strlen(name) + 1;
            return h;
        }
    }

    return NULL;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {

    // check if any of the parameters are NULL
    if (cl == NULL || h == NULL || payload == NULL) {
        return NULL;
    }

    // use socket to send a message to the server

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
 * Print the handler.
 *
 * @param data The handler to print.
 */
void print_handler(void *data) {
    rpc_handler h = (rpc_handler)data;
    printf("%p\n", h);
}

/*
 * Send a message through a socket.
 * @param sockfd The socket to send the message to.
 * @param msg The message to send.
 * @return The number of bytes sent.
 */
int send_message(int sockfd, const unsigned char *msg, int size) {
    int n = write(sockfd, msg, size);
    if (n < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    return n;
}

/*
 * Receive a message through a socket.
 * @param sockfd The socket to receive the message from.
 * @param buffer The buffer to store the message in.
 * @return The number of bytes received.
 */
int receive_message(int sockfd, unsigned char *buffer, int size) {
    int n = read(sockfd, buffer, size);
    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    return n;
}

/*
 * Send a message through a socket and receive a response.
 * @param sockfd The socket to send the message to.
 * @param msg The message to send.
 * @return The response from the server.
 * @note We must be wary of serialisation and endianess.
 */
rpc_message *request(int sockfd, rpc_message *msg) {

    // convert message to serialised form
    unsigned char buffer[BUFSIZ];
    serialise_rpc_message(buffer, msg);

    // send message
    send_message(sockfd, buffer, BUFSIZ);

    // receive response
    receive_message(sockfd, buffer, BUFSIZ);

    // deserialise response
    rpc_message *response = deserialise_rpc_message(buffer);

    return response;
}

unsigned char *serialise_int(unsigned char *buffer, int value) {
    value = htonl(value);
    memcpy(buffer, &value, sizeof(int));
    return buffer + sizeof(int);
}

int deserialise_int(unsigned char *buffer) {
    int value;
    memcpy(&value, buffer, sizeof(int));
    return ntohl(value);
}

unsigned char *serialise_size_t(unsigned char *buffer, size_t value) {
    value = htonl(value);
    memcpy(buffer, &value, sizeof(size_t));
    return buffer + sizeof(size_t);
}

size_t deserialise_size_t(unsigned char *buffer) {
    size_t value;
    memcpy(&value, buffer, sizeof(size_t));
    return ntohl(value);
}

unsigned char *serialise_char(unsigned char *buffer, char value) {
    *buffer = value;
    return buffer + sizeof(char);
}

char deserialise_char(unsigned char *buffer) {
    char value;
    memcpy(&value, buffer, sizeof(char));
    return value;
}

unsigned char *serialise_string(unsigned char *buffer, const char *value) {
    int size = strlen(value);
    buffer = serialise_int(buffer, size);
    memcpy(buffer, value, size);
    return buffer + size;
}

char *deserialise_string(unsigned char *buffer) {
    int size = deserialise_int(buffer);
    char *value = (char *)malloc(sizeof(char) * size);
    memcpy(value, buffer, size);
    return value;
}

unsigned char *serialise_rpc_handle(unsigned char *buffer, const rpc_handle *handle) {
    buffer = serialise_string(buffer, handle->name);
    return buffer;
}

rpc_handle *deserialise_rpc_handle(unsigned char *buffer) {
    rpc_handle *handle = (rpc_handle *)malloc(sizeof(*handle));
    handle->name = deserialise_string(buffer);
    return handle;
}

unsigned char *serialise_rpc_data(unsigned char *buffer, const rpc_data *data) {
    buffer = serialise_int(buffer, data->data1);
    
    // optionally serialise data2
    if (data->data2_len > 0 && data->data2 != NULL) {
        buffer = serialise_size_t(buffer, data->data2_len);
        memcpy(buffer, data->data2, data->data2_len);
        return buffer + data->data2_len;
    }
    return buffer;
}

rpc_data *deserialise_rpc_data(unsigned char *buffer) {
    rpc_data *data = (rpc_data *)malloc(sizeof(*data));
    data->data1 = deserialise_int(buffer);
    data->data2_len = deserialise_size_t(buffer);
    data->data2 = (char *)malloc(sizeof(char) * data->data2_len);
    memcpy(data->data2, buffer, data->data2_len);
    return data;
}

unsigned char *serialise_rpc_message(unsigned char *buffer, const rpc_message *message) {
    buffer = serialise_int(buffer, message->request_id);
    buffer = serialise_int(buffer, message->operation);
    buffer = serialise_rpc_handle(buffer, message->handle);
    buffer = serialise_rpc_data(buffer, message->data);
    return buffer;
}

rpc_message *deserialise_rpc_message(unsigned char *buffer) {
    rpc_message *message = (rpc_message *)malloc(sizeof(*message));
    message->request_id = deserialise_int(buffer);
    message->operation = deserialise_int(buffer);
    message->handle = deserialise_rpc_handle(buffer);
    message->data = deserialise_rpc_data(buffer);
    return message;
}