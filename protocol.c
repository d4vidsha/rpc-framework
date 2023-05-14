/* =============================================================================
   protocol.c

   Everything related to the protocol.

   Author: David Sha
============================================================================= */
#include "protocol.h"
#include "config.h"
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int write_bytes(int sockfd, const unsigned char *bytes, int size) {
    debug_print("Writing %d bytes\n", size);
    debug_print_bytes(bytes, size);
    int n = write(sockfd, bytes, size);
    if (n < 0) {
        if (errno == EPIPE) {
            debug_print("%s", "Connection closed by client\n");
            close(sockfd);
        } else {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
    return n;
}

int read_bytes(int sockfd, unsigned char *buffer, int size) {
    debug_print("\nReading %d bytes\n", size);
    int n = read(sockfd, buffer, size);
    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    } else if (n == 0) {
        debug_print("%s", "Connection closed\n");
        close(sockfd);
    } else {
        debug_print_bytes(buffer, n);
    }
    return n;
}

void debug_print_bytes(const unsigned char *buffer, size_t len) {
    debug_print("Serialised message (%ld bytes):\n", len);
    int box_size = 16;
    for (int i = 0; i < len; i += box_size) {
        for (int j = 0; j < box_size; j++) {
            if (i + j < len)
                debug_print("%02X ", buffer[i + j]);
            else
                debug_print("%s", "   ");
        }
        debug_print("%s", "  ");
        for (int j = 0; j < box_size; j++) {
            if (i + j < len) {
                if (isprint(buffer[i + j]))
                    debug_print("%c", buffer[i + j]);
                else
                    debug_print("%s", ".");
            }
        }
        debug_print("%s", "\n");
    }
}

int send_rpc_message(int sockfd, rpc_message *msg) {

    // convert message to serialised form
    unsigned char buffer[BUFSIZ], *ptr;
    ptr = serialise_rpc_message(buffer, msg);

    // send an integer representing the size of the message
    int size = ptr - buffer;
    unsigned char size_buffer[sizeof(size)], *size_ptr = size_buffer;
    serialise_int(size_buffer, size);
    if (write_bytes(sockfd, size_buffer, sizeof(size)) < 0) {
        return FAILED;
    }

    // read that the number of bytes sent is correct
    int n;
    if (read_bytes(sockfd, size_buffer, sizeof(size)) <= 0) {
        return FAILED;
    }
    n = deserialise_int(&size_ptr);
    if (n != size) {
        debug_print("Error: sent %d bytes but received %d bytes before sending "
                    "message\n",
                    size, n);
        exit(EXIT_FAILURE);
    } else {
        debug_print("%s", "Looks good, sending payload...\n");
    }

    // send the message
    if (write_bytes(sockfd, buffer, size) < 0) {
        return FAILED;
    }

    return 0;
}

rpc_message *receive_rpc_message(int sockfd) {

    // read size of message and send back to confirm
    int size;
    unsigned char size_buffer[sizeof(size)], *size_ptr = size_buffer;
    if (read_bytes(sockfd, size_buffer, sizeof(size)) <= 0) {
        return NULL;
    }
    size = deserialise_int(&size_ptr);
    debug_print("Sending back the expected size of %d bytes...\n", size);
    if (write_bytes(sockfd, size_buffer, sizeof(size)) < 0) {
        return NULL;
    }

    // read message
    unsigned char buffer[size], *ptr = buffer;
    if (read_bytes(sockfd, buffer, size) <= 0) {
        return NULL;
    }
    rpc_message *msg = deserialise_rpc_message(&ptr);

    return msg;
}

rpc_message *request(int sockfd, rpc_message *msg) {

    // send message
    if (send_rpc_message(sockfd, msg) == FAILED) {
        free_rpc_message(msg, NULL);
        return NULL;
    }
    free_rpc_message(msg, NULL);

    // return response
    return receive_rpc_message(sockfd);
}

unsigned char *serialise_int(unsigned char *buffer, int value) {
    value = htonl(value);
    memcpy(buffer, &value, sizeof(int));
    return buffer + sizeof(int);
}

int deserialise_int(unsigned char **buffer_ptr) {
    int value;
    memcpy(&value, *buffer_ptr, sizeof(int));
    *buffer_ptr += sizeof(int);
    return ntohl(value);
}

unsigned char *serialise_size_t(unsigned char *buffer, size_t value) {
    value = htonl(value);
    memcpy(buffer, &value, sizeof(size_t));
    return buffer + sizeof(size_t);
}

size_t deserialise_size_t(unsigned char **buffer_ptr) {
    size_t value;
    memcpy(&value, *buffer_ptr, sizeof(size_t));
    *buffer_ptr += sizeof(size_t);
    return ntohl(value);
}

unsigned char *serialise_string(unsigned char *buffer, const char *value) {
    size_t len = strlen(value) + 1;
    buffer = serialise_size_t(buffer, len);
    memcpy(buffer, value, len);
    return buffer + len;
}

char *deserialise_string(unsigned char **buffer_ptr) {
    size_t len = deserialise_size_t(buffer_ptr);
    char *value = new_string((char *)*buffer_ptr);
    *buffer_ptr += len;
    return value;
}

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

rpc_data *deserialise_rpc_data(unsigned char **buffer_ptr) {
    rpc_data *data = new_rpc_data(deserialise_int(buffer_ptr),
                                  deserialise_size_t(buffer_ptr), *buffer_ptr);
    *buffer_ptr += data->data2_len;
    return data;
}

unsigned char *serialise_rpc_message(unsigned char *buffer,
                                     const rpc_message *message) {
    buffer = serialise_int(buffer, message->request_id);
    buffer = serialise_int(buffer, message->operation);
    buffer = serialise_string(buffer, message->function_name);
    buffer = serialise_rpc_data(buffer, message->data);
    return buffer;
}

rpc_message *deserialise_rpc_message(unsigned char **buffer_ptr) {
    rpc_message *message = new_rpc_message(
        deserialise_int(buffer_ptr), deserialise_int(buffer_ptr),
        deserialise_string(buffer_ptr), deserialise_rpc_data(buffer_ptr));
    return message;
}

char *new_string(const char *value) {
    char *string = (char *)malloc(sizeof(char) * (strlen(value) + 1));
    assert(string);
    strcpy(string, value);
    return string;
}

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

rpc_message *new_rpc_message(int request_id, int operation, char *function_name,
                             rpc_data *data) {
     rpc_message *message = (rpc_message *)malloc(sizeof(*message));
    assert(message);
    message->request_id = request_id;
    message->operation = operation;
    message->function_name = function_name;
    message->data = data;
    return message;
}

void free_rpc_message(rpc_message *message, void (*free_data)(rpc_data *)) {
    free(message->function_name);
    if (free_data) {
        free_data(message->data);
    }
    free(message);
}