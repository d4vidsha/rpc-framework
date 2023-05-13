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
#include "sockets.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <ctype.h>

static volatile sig_atomic_t keep_running = 1;

/*
 * Int handler for SIGINT.
 */
static void sig_handler(int _) {
    (void) _;
    keep_running = 0;

    char msg[] = "\nSignal received\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

typedef struct {
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
} rpc_client_state;

typedef struct {
    int request_id;
    enum {
        FIND,
        CALL,
        REPLY,
    } operation;
    char *function_name;
    rpc_data *data;
} rpc_message;

void handle_request(rpc_server *srv, rpc_client_state *cl);
void handle_all_requests(rpc_server *srv, rpc_client_state *cl);
void rpc_shutdown_server(rpc_server *srv);

void print_handler(void *data);
rpc_message *request(int sockfd, rpc_message *msg);
int write_bytes(int sockfd, const unsigned char *msg, int size);
int read_bytes(int sockfd, unsigned char *buffer, int size);
void print_bytes(const unsigned char *buffer, size_t len);
void send_rpc_message(int sockfd, rpc_message *msg);
rpc_message *receive_rpc_message(int sockfd);

unsigned char *serialise_int(unsigned char *buffer, int value);
int deserialise_int(unsigned char **buffer_ptr);
unsigned char *serialise_size_t(unsigned char *buffer, size_t value);
size_t deserialise_size_t(unsigned char **buffer_ptr);
unsigned char *serialise_string(unsigned char *buffer, const char *value);
char *deserialise_string(unsigned char **buffer_ptr);
unsigned char *serialise_rpc_data(unsigned char *buffer, const rpc_data *data);
rpc_data *deserialise_rpc_data(unsigned char **buffer_ptr);
unsigned char *serialise_rpc_message(unsigned char *buffer, const rpc_message *message);
rpc_message *deserialise_rpc_message(unsigned char **buffer_ptr);

char *new_string(const char *value);
rpc_handle *new_rpc_handle(const char *name);
void free_rpc_handle(rpc_handle *handle);
void print_rpc_handle(rpc_handle *handle);
rpc_data *new_rpc_data(int data1, size_t data2_len, void *data2);
void free_rpc_data(rpc_data *data);
void print_rpc_message(rpc_message *message);
rpc_message *new_rpc_message(int request_id, int operation, char *function_name, rpc_data *data);
void free_rpc_message(rpc_message *message);
void print_rpc_data(rpc_data *data);




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

    fprintf(stderr, "Added %s handler\n", name);

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
    struct sigaction act = { .sa_handler = sig_handler };
    sigaction(SIGINT, &act, NULL);

    // keep running until SIGINT is received
    while (keep_running) {
        // listen on socket, incoming connection requests will be queued
        if (listen(srv->sockfd, BACKLOG) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }

        // store client information
        rpc_client_state *cl = (rpc_client_state *)malloc(sizeof(*cl));
        assert(cl);
        cl->client_addr_size = sizeof(cl->client_addr);

        // accept a connection
        cl->sockfd = accept(srv->sockfd, (struct sockaddr *)&cl->client_addr, &cl->client_addr_size);
        if (cl->sockfd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // add to list of clients
        append(srv->clients, cl);

        // print the client's IP address
        getpeername(cl->sockfd, (struct sockaddr *)&cl->client_addr, &cl->client_addr_size);
        char ipstr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cl->client_addr.sin_addr, ipstr, sizeof ipstr);
        int port = ntohs(cl->client_addr.sin_port);
        fprintf(stderr, "Received connection from %s:%d on socket %d\n", ipstr, port, cl->sockfd);

        // handle requests from the client
        handle_all_requests(srv, cl);
    }

    printf("\nShutting down...\n");
    rpc_shutdown_server(srv);
    return;
}

/*
 * Handle all requests from the client.
 * 
 * @param srv The server state.
 * @param cl The client state.
 * @param client_addr The client's address.
 */
void handle_all_requests(rpc_server *srv, rpc_client_state *cl) {
    while (keep_running && cl->sockfd != -1) {
        fprintf(stderr, "===================================================\n");
        fprintf(stderr, "Waiting for request...\n");
        handle_request(srv, cl);
    }
}

/*
 * Handle a request from the client.
 * 
 * @param srv The server state.
 * @param cl The client state.
 */
void handle_request(rpc_server *srv, rpc_client_state *cl) {
    // receive rpc_message from the client and process it
    rpc_message *msg = receive_rpc_message(cl->sockfd);
    rpc_message *new_msg = NULL;
    switch (msg->operation) {

        case FIND:
            fprintf(stderr, "Received FIND request\n");
            fprintf(stderr, "Looking for handler: %s\n", msg->function_name);

            // get the handler from the hashtable
            rpc_handler h = hashtable_lookup(srv->handlers, msg->function_name);
            
            // check if the handler exists
            int exists = h != NULL;
            if (exists) {
                fprintf(stderr, "Handler found\n");
            } else {
                fprintf(stderr, "Handler not found\n");
            }
            new_msg = new_rpc_message(321, REPLY, msg->function_name, new_rpc_data(exists, 0, NULL));
            break;

        case CALL:
            fprintf(stderr, "Received CALL request\n");
            fprintf(stderr, "Calling handler: %s\n", msg->function_name);

            // get the handler from the hashtable
            rpc_handler handler = hashtable_lookup(srv->handlers, msg->function_name);

            // run the handler
            rpc_data *data = handler(msg->data);

            // create a new message to send back to the client
            new_msg = new_rpc_message(321, REPLY, msg->function_name, data);
            break;

        case REPLY:
            fprintf(stderr, "Received REPLY request\n");
            fprintf(stderr, "Doing nothing...\n");
            break;

        default:
            fprintf(stderr, "Received unknown request\n");
            fprintf(stderr, "Doing nothing...\n");
            break;
    }
    // send the message to the server
    send_rpc_message(cl->sockfd, new_msg);
}

/*
 * Shuts down the server and frees the server state.
 * 
 * @param srv The server state.
 */
void rpc_shutdown_server(rpc_server *srv) {

    // check if the server is NULL
    if (srv == NULL) {
        return;
    }

    // close the socket
    close(srv->sockfd);

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
    char *name;
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
    cl->sockfd = create_connection_socket(addr, sport);

    return cl;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {

    // check if any of the parameters are NULL
    if (cl == NULL || name == NULL) {
        return NULL;
    }

    // send message to the server and wait for a reply
    rpc_message *reply = request(cl->sockfd, 
                                 new_rpc_message(
                                    123, 
                                    FIND, 
                                    new_string(name), 
                                    new_rpc_data(
                                        0, 
                                        0, 
                                        NULL
                                    )
                                  )
                                );

    // check if the handler exists
    if (reply->operation == REPLY && reply->data->data1 == TRUE) {
        return new_rpc_handle(name);
    }

    return NULL;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {

    // check if any of the parameters are NULL
    if (cl == NULL || h == NULL || payload == NULL) {
        return NULL;
    }

    // use socket to send a message to the server
    rpc_message *reply = request(cl->sockfd, 
                                 new_rpc_message(
                                    123, 
                                    CALL, 
                                    new_string(h->name), 
                                    payload
                                  )
                                );

    // send the reply back to the client
    if (reply->operation == REPLY) {
        return reply->data;
    }

    return NULL;
}

void rpc_close_client(rpc_client *cl) {

    // check if the client is NULL
    if (cl == NULL) {
        return;
    }

    // close the socket
    close(cl->sockfd);

    // free the client state
    free(cl);

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
 * Write a string of bytes to a socket.
 *
 * @param sockfd The socket to write to.
 * @param bytes The bytes to write.
 * @param size The number of bytes to write.
 */
int write_bytes(int sockfd, const unsigned char *bytes, int size) {
    fprintf(stderr, "Writing %d bytes\n", size);
    print_bytes(bytes, size);
    int n = write(sockfd, bytes, size);
    if (n < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    return n;
}

/*
 * Read a string of bytes from a socket.
 *
 * @param sockfd The socket to read from.
 * @param buffer The buffer to read into.
 * @param size The number of bytes to read.
 */
int read_bytes(int sockfd, unsigned char *buffer, int size) {
    fprintf(stderr, "\nReading %d bytes\n", size);
    int n = read(sockfd, buffer, size);
    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    print_bytes(buffer, size);
    return n;
}

/*
 * Print bytes.
 * 
 * @param buffer: buffer to print
 * @param len: length of buffer
 */
void print_bytes(const unsigned char *buffer, size_t len) {
    printf("Serialised message (%ld bytes):\n", len);
    int box_size = 16;
    for (int i = 0; i < len; i += box_size) {
        for (int j = 0; j < box_size; j++) {
            if (i + j < len)
                printf("%02X ", buffer[i + j]);
            else
                printf("   ");
        }
        printf("  ");
        for (int j = 0; j < box_size; j++) {
            if (i + j < len) {
                if (isprint(buffer[i + j]))
                    printf("%c", buffer[i + j]);
                else
                    printf(".");
            }
        }
        printf("\n");
    }
}

/*
 * Send an rpc_message through a socket.
 * 
 * @param sockfd The socket to send the message to.
 * @param msg The message to send.
 */
void send_rpc_message(int sockfd, rpc_message *msg) {

    // convert message to serialised form
    unsigned char buffer[BUFSIZ], *ptr;
    ptr = serialise_rpc_message(buffer, msg);

    // send an integer representing the size of the message
    int size = ptr - buffer;
    unsigned char size_buffer[sizeof(size)], *size_ptr = size_buffer;
    serialise_int(size_buffer, size);
    write_bytes(sockfd, size_buffer, sizeof(size));

    // read that the number of bytes sent is correct
    int n;
    read_bytes(sockfd, size_buffer, sizeof(size));
    n = deserialise_int(&size_ptr);
    if (n != size) {
        fprintf(stderr, "Error: sent %d bytes but received %d bytes before sending message\n", size, n);
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, "Looks good, sending payload...\n");
    }

    // send the message
    write_bytes(sockfd, buffer, size);
}

/*
 * Receive an rpc_message from a socket.
 * 
 * @param sockfd The socket to receive the message from.
 * @return The message received.
 */
rpc_message *receive_rpc_message(int sockfd) {

    // read size of message and send back to confirm
    int size;
    unsigned char size_buffer[sizeof(size)], *size_ptr = size_buffer;
    read_bytes(sockfd, size_buffer, sizeof(size));
    size = deserialise_int(&size_ptr);
    fprintf(stderr, "Sending back the expected size of %d bytes...\n", size);
    write_bytes(sockfd, size_buffer, sizeof(size));

    // read message
    unsigned char buffer[size], *ptr = buffer;
    read_bytes(sockfd, buffer, size);
    rpc_message *msg = deserialise_rpc_message(&ptr);

    return msg;
}

/*
 * Send a message through a socket and receive a response.
 * @param sockfd The socket to send the message to.
 * @param msg The message to send.
 * @return The response from the server.
 * @note We must be wary of serialisation and endianess.
 */
rpc_message *request(int sockfd, rpc_message *msg) {

    // send message
    send_rpc_message(sockfd, msg);

    // receive response
    rpc_message *response = receive_rpc_message(sockfd);

    return response;
}

/*
 * Serialise integer value into buffer.
 * 
 * @param buffer: buffer to serialise into
 * @param value: integer value to serialise
 * @return: pointer to next byte in buffer
 */
unsigned char *serialise_int(unsigned char *buffer, int value) {
    value = htonl(value);
    memcpy(buffer, &value, sizeof(int));
    return buffer + sizeof(int);
}

/*
 * Deserialise integer value from buffer.
 * 
 * @param buffer: buffer to deserialise from
 * @return: deserialised integer value
 * @note: buffer pointer is incremented
 */
int deserialise_int(unsigned char **buffer_ptr) {
    int value;
    memcpy(&value, *buffer_ptr, sizeof(int));
    *buffer_ptr += sizeof(int);
    return ntohl(value);
}

/*
 * Serialise size_t value into buffer.
 * 
 * @param buffer: buffer to serialise into
 * @param value: size_t value to serialise
 * @return: pointer to next byte in buffer
 */
unsigned char *serialise_size_t(unsigned char *buffer, size_t value) {
    value = htonl(value);
    memcpy(buffer, &value, sizeof(size_t));
    return buffer + sizeof(size_t);
}

/*
 * Deserialise size_t value from buffer.
 * 
 * @param buffer: buffer to deserialise from
 * @return: deserialised size_t value
 * @note: buffer pointer is incremented
 */
size_t deserialise_size_t(unsigned char **buffer_ptr) {
    size_t value;
    memcpy(&value, *buffer_ptr, sizeof(size_t));
    *buffer_ptr += sizeof(size_t);
    return ntohl(value);
}

/*
 * Serialise string value into buffer.
 * 
 * @param buffer: buffer to serialise into
 * @param value: string value to serialise
 * @return: pointer to next byte in buffer
 */
unsigned char *serialise_string(unsigned char *buffer, const char *value) {
    size_t len = strlen(value) + 1;
    buffer = serialise_size_t(buffer, len);
    memcpy(buffer, value, len);
    return buffer + len;
}

/*
 * Deserialise string value from buffer.
 * 
 * @param buffer: buffer to deserialise from
 * @return: deserialised string value
 * @note: buffer pointer is incremented
 */
char *deserialise_string(unsigned char **buffer_ptr) {
    size_t len = deserialise_size_t(buffer_ptr);
    char *value = new_string((char *)*buffer_ptr);
    *buffer_ptr += len;
    return value;
}

/*
 * Serialise rpc_data value into buffer.
 * 
 * @param buffer: buffer to serialise into
 * @param data: rpc_data value to serialise
 * @return: pointer to next byte in buffer
 */
unsigned char *serialise_rpc_data(unsigned char *buffer, const rpc_data *data) {
    buffer = serialise_int(buffer, data->data1);
    buffer = serialise_size_t(buffer, data->data2_len);
    
    // optionally serialise data2
    if (data->data2_len > 0 && data->data2 != NULL) {
        memcpy(buffer, data->data2, data->data2_len);
        buffer += data->data2_len;
    }

    return buffer;
}

/*
 * Deserialise rpc_data value from buffer.
 * 
 * @param buffer: buffer to deserialise from
 * @return: deserialised rpc_data value
 * @note: buffer pointer is incremented
 */ 
rpc_data *deserialise_rpc_data(unsigned char **buffer_ptr) {
    rpc_data *data = new_rpc_data(
        deserialise_int(buffer_ptr),
        deserialise_size_t(buffer_ptr),
        *buffer_ptr
    );
    *buffer_ptr += data->data2_len;
    return data;
}

unsigned char *serialise_rpc_message(unsigned char *buffer, const rpc_message *message) {
    buffer = serialise_int(buffer, message->request_id);
    buffer = serialise_int(buffer, message->operation);
    buffer = serialise_string(buffer, message->function_name);
    buffer = serialise_rpc_data(buffer, message->data);
    return buffer;
}

rpc_message *deserialise_rpc_message(unsigned char **buffer_ptr) {
    rpc_message *message = new_rpc_message(
        deserialise_int(buffer_ptr),
        deserialise_int(buffer_ptr),
        deserialise_string(buffer_ptr),
        deserialise_rpc_data(buffer_ptr)
    );
    return message;
}

/*
 * Create a new string.
 * 
 * @param value: string value
 * @return: new string
 */
char *new_string(const char *value) {
    char *string = (char *)malloc(sizeof(char) * (strlen(value) + 1));
    assert(string);
    strcpy(string, value);
    return string;
}

/*
 * Create a new RPC handle.
 * 
 * @param name: name of RPC handle
 * @return new RPC handle.
 */
rpc_handle *new_rpc_handle(const char *name) {
    rpc_handle *handle = (rpc_handle *)malloc(sizeof(*handle));
    assert(handle);
    handle->name = new_string(name);
    return handle;
}

/*
 * Create a new RPC data.
 *
 * @param data1 The first data value.
 * @param data2 The second data value.
 * @param data2_len The length of the second data value.
 * @return The new RPC data.
 */
rpc_data *new_rpc_data(int data1, size_t data2_len, void *data2) {
    rpc_data *data = (rpc_data *)malloc(sizeof(*data));
    assert(data);
    data->data1 = data1;
    data->data2_len = data2_len;
    if (data2_len == 0) {
        data->data2 = NULL;
    } else {
        data->data2 = malloc(data2_len);
        assert(data->data2);
        memcpy(data->data2, data2, data2_len);
    }
    return data;
}

/*
 * Create a new RPC message.
 *
 * @param request_id The request ID.
 * @param operation The operation to perform.
 * @param handle The handle to perform the operation on.
 * @param data The data to send.
 * @return The new RPC message.
 */
rpc_message *new_rpc_message(int request_id, int operation, char *function_name, rpc_data *data) {
    rpc_message *message = (rpc_message *)malloc(sizeof(*message));
    assert(message);
    message->request_id = request_id;
    message->operation = operation;
    message->function_name = function_name;
    message->data = data;
    return message;
}

/*
 * Free an RPC handle.
 *
 * @param handle The RPC handle to free.
 */
void free_rpc_handle(rpc_handle *handle) {
    free(handle->name);
    free(handle);
}

/*
 * Free an RPC data.
 *
 * @param data The RPC data to free.
 */
void free_rpc_data(rpc_data *data) {
    free(data->data2);
    free(data);
}

/*
 * Free an RPC message.
 *
 * @param message The RPC message to free.
 */
void free_rpc_message(rpc_message *message) {
    free(message->function_name);
    free_rpc_data(message->data);
    free(message);
}

/*
 * Print an RPC handle.
 *
 * @param handle The RPC handle to print.
 */
void print_rpc_handle(rpc_handle *handle) {
    printf("name: %s\n", handle->name);
}

/*
 * Print an RPC data.   
 *
 * @param data The RPC data to print.
 */
void print_rpc_data(rpc_data *data) {
    printf("data1: %d\n", data->data1);
    printf("data2_len: %zu\n", data->data2_len);
    printf("data2: %s\n", (char *)data->data2);
}

/*
 * Print an RPC message.
 *
 * @param message The RPC message to print.
 */
void print_rpc_message(rpc_message *message) {
    printf("request_id: %d\n", message->request_id);
    printf("operation: %d\n", message->operation);
    printf("function_name: %s\n", message->function_name);
    print_rpc_data(message->data);
}
