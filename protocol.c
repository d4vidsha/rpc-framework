/* =============================================================================
   protocol.c

   Everything related to the protocol.

   Author: David Sha
============================================================================= */
#include "protocol.h"
#include "config.h"
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
    unsigned char size_buffer[BUFSIZ], *size_ptr = size_buffer;
    size_ptr = serialise_int(size_buffer, size);
    if (write_bytes(sockfd, size_buffer, size_ptr - size_buffer) < 0) {
        return FAILED;
    }

    // read that the number of bytes sent is correct
    int n;
    if (read_bytes(sockfd, size_buffer, size_ptr - size_buffer) <= 0) {
        return FAILED;
    }
    size_ptr = size_buffer;
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
    unsigned char size_buffer[BUFSIZ], *size_ptr = size_buffer;
    if (read_bytes(sockfd, size_buffer, BUFSIZ) <= 0) {
        return NULL;
    }
    size = deserialise_int(&size_ptr);
    debug_print("Sending back the expected size of %d bytes...\n", size);
    if (write_bytes(sockfd, size_buffer, size_ptr - size_buffer) < 0) {
        return NULL;
    }

    // read message
    unsigned char buffer[size], *ptr = buffer;
    if (read_bytes(sockfd, buffer, size) <= 0) {
        return NULL;
    }
    rpc_message *msg = deserialise_rpc_message(&ptr);
    debug_print_rpc_message(msg);

    return msg;
}

rpc_message *request(int sockfd, rpc_message *msg) {

    // send message
    if (send_rpc_message(sockfd, msg) == FAILED) {
        rpc_message_free(msg, NULL);
        return NULL;
    }
    rpc_message_free(msg, NULL);

    // return response
    return receive_rpc_message(sockfd);
}

unsigned char *serialise_int(unsigned char *buffer, int value) {
    buffer[0] = value >> 24;
    buffer[1] = value >> 16;
    buffer[2] = value >> 8;
    buffer[3] = value;
    return buffer + 4;
}

int deserialise_int(unsigned char **buffer_ptr) {
    unsigned char *buffer = *buffer_ptr;
    int value = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) |
                buffer[3];
    *buffer_ptr += 4;
    return value;
}

unsigned char *serialise_size_t(unsigned char *buffer, size_t value) {
    // we cannot take 0 as a valid value for Elias gamma coding so we 
    // increment all values by 1 and decrement them when deserialising
    value++;

    unsigned char *ptr = buffer;
    unsigned int length = 0;

    // calculate the number of bits required to represent the value
    while ((value >> length) > 0) {
        length++;
    }

    // encode the number of bits using unary code
    for (unsigned int i = 0; i < length - 1; i++) {
        *ptr++ = 0x00;
    }
    *ptr++ = 0x01;

    // encode the value using binary code
    for (unsigned int i = length - 1; i > 0; i--) {
        *ptr++ = (value >> (i - 1)) & 0x01;
    }

    return ptr;
}

size_t deserialise_size_t(unsigned char **buffer_ptr) {
    unsigned char *ptr = *buffer_ptr;
    unsigned int length = 0;

    // decode the number of bits using unary code
    while (*ptr == 0x00) {
        length++;
        ptr++;
    }

    // convert the binary code to a value
    size_t value = 0;
    for (unsigned int i = 0; i < length + 1; i++) {
        value = (value << 1) | *ptr++;
    }

    *buffer_ptr = ptr;
    return value - 1;
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
    int data1 = deserialise_int(buffer_ptr);
    size_t data2_len = deserialise_size_t(buffer_ptr);
    rpc_data *data = new_rpc_data(data1, data2_len, *buffer_ptr);
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
    int request_id = deserialise_int(buffer_ptr);
    int operation = deserialise_int(buffer_ptr);
    char *function_name = deserialise_string(buffer_ptr);
    rpc_data *data = deserialise_rpc_data(buffer_ptr);
    rpc_message *message = new_rpc_message(request_id, operation, function_name, data);
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

void rpc_message_free(rpc_message *message, void (*free_data)(rpc_data *)) {
    free(message->function_name);
    if (free_data) {
        free_data(message->data);
    }
    free(message);
}

void debug_print_rpc_data(rpc_data *data) {
    debug_print(" |- data1: %d\n", data->data1);
    debug_print(" |- data2_len: %zu\n", data->data2_len);
    debug_print("%s", " |- data2: ");
    for (size_t i = 0; i < data->data2_len; i++) {
        debug_print("%02x ", ((unsigned char *)data->data2)[i]);
    }
    debug_print("%s", "\n");
}

void debug_print_rpc_message(rpc_message *message) {
    debug_print("%s", "rpc_message\n");
    debug_print(" |- request_id: %d\n", message->request_id);
    debug_print(" |- operation: %d\n", message->operation);
    debug_print(" |- function_name: %s\n", message->function_name);
    debug_print_rpc_data(message->data);
}
