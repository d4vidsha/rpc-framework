/* =============================================================================
   protocol.c

   Everything related to the protocol.

   References:
   - Elias Gamma Coding: https://en.wikipedia.org/wiki/Elias_gamma_coding
   - While loop with write and read: https://stackoverflow.com/a/15384631
   - Buffer framework: https://stackoverflow.com/a/6002598

   Author: David Sha
============================================================================= */
#include "protocol.h"
#include "config.h"
#include <assert.h>
#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

buffer_t *new_buffer(size_t size) {
    buffer_t *b = (buffer_t *)malloc(sizeof(*b));
    assert(b);
    b->data = calloc(size, sizeof(*b->data));
    assert(b->data);
    b->next = 0;
    b->size = size;
    return b;
}

void buffer_free(buffer_t *b) {
    free(b->data);
    free(b);
}

void reserve_space(buffer_t *b, size_t size) {
    while (b->next + size > b->size) {
        // double the size of the buffer for O(log n) reallocations
        b->size *= 2;
        b->data = realloc(b->data, b->size);
        assert(b->data);
    }
}

int write_bytes(int sockfd, unsigned char *buf, size_t size) {
    debug_print("\nWriting %ld bytes\n", size);
    size_t total_bytes_written = 0;
    while (total_bytes_written != size) {
        assert(total_bytes_written < size);
        size_t bytes_written = write(sockfd, buf + total_bytes_written,
                                     size - total_bytes_written);
        if (bytes_written < 0) {
            if (errno == EPIPE) {
                debug_print("%s", "Connection closed by client\n");
            } else {
                debug_print("%s", "Error writing to socket\n");
            }
            close(sockfd);
            return FAILED;
        } else {
            debug_print("Wrote %ld bytes\n", bytes_written);
            debug_print_bytes(buf + total_bytes_written, bytes_written);
        }
        total_bytes_written += bytes_written;
    }
    assert(total_bytes_written == size);
    return total_bytes_written;
}

int read_bytes(int sockfd, unsigned char *buf, size_t size) {
    debug_print("\nReading %ld bytes\n", size);
    size_t total_bytes_read = 0;
    while (total_bytes_read != size) {
        assert(total_bytes_read < size);
        size_t bytes_read =
            read(sockfd, buf + total_bytes_read, size - total_bytes_read);
        if (bytes_read < 0) {
            debug_print("%s", "Error reading from socket\n");
            close(sockfd);
            return FAILED;
        } else if (bytes_read == 0) {
            debug_print("%s", "Connection closed by client\n");
            close(sockfd);
            return FAILED;
        } else {
            debug_print("Read %ld bytes\n", bytes_read);
            debug_print_bytes(buf + total_bytes_read, bytes_read);
        }
        total_bytes_read += bytes_read;
    }
    assert(total_bytes_read == size);
    return total_bytes_read;
}

void debug_print_bytes(const unsigned char *buf, size_t len) {
    debug_print("Serialised message (%ld bytes):\n", len);
    int box_size = 16;
    for (int i = 0; i < len; i += box_size) {
        for (int j = 0; j < box_size; j++) {
            if (i + j < len)
                debug_print("%02X ", buf[i + j]);
            else
                debug_print("%s", "   ");
        }
        debug_print("%s", "  ");
        for (int j = 0; j < box_size; j++) {
            if (i + j < len) {
                if (isprint(buf[i + j]))
                    debug_print("%c", buf[i + j]);
                else
                    debug_print("%s", ".");
            }
        }
        debug_print("%s", "\n");
    }
}

int send_rpc_message(int sockfd, rpc_message *msg) {
    int result = FAILED;

    // convert message to serialised form
    buffer_t *buf = new_buffer(INITIAL_BUFFER_SIZE);
    serialise_rpc_message(buf, msg);

    // send an integer representing the size of the message
    size_t size = buf->next;
    size_t gamma_size = gamma_code_length(MAX_MESSAGE_BYTE_SIZE);
    buffer_t *size_buf = new_buffer(gamma_size);
    buffer_t *n_buf = new_buffer(gamma_size);
    serialise_size_t(size_buf, size);
    if (write_bytes(sockfd, size_buf->data, size_buf->size) < 0) {
        goto cleanup;
    }

    // read that the number of bytes sent is correct
    if (read_bytes(sockfd, n_buf->data, n_buf->size) <= 0) {
        goto cleanup;
    }
    size_t n = deserialise_size_t(n_buf);
    if (n != size) {
        debug_print("Error: sent %ld bytes but received %ld bytes before "
                    "sending message\n",
                    size, n);
        goto cleanup;
    } else {
        debug_print("%s", "Looks good, sending payload...\n");
    }

    // send the message
    if (write_bytes(sockfd, buf->data, buf->next) < 0) {
        goto cleanup;
    }

    // success if we get here
    result = 0;

cleanup:
    // free buffers
    buffer_free(buf);
    buffer_free(size_buf);
    buffer_free(n_buf);

    return result;
}

rpc_message *receive_rpc_message(int sockfd) {

    // read size of message and send back to confirm
    buffer_t *size_buf = new_buffer(gamma_code_length(MAX_MESSAGE_BYTE_SIZE));
    if (read_bytes(sockfd, size_buf->data, size_buf->size) <= 0) {
        return NULL;
    }
    size_t size = deserialise_size_t(size_buf);
    debug_print("Sending back the expected size of %ld bytes...\n", size);
    if (write_bytes(sockfd, size_buf->data, size_buf->size) < 0) {
        return NULL;
    }

    // read message
    buffer_t *buf = new_buffer(size);

    if (read_bytes(sockfd, buf->data, size) <= 0) {
        return NULL;
    }
    rpc_message *msg = deserialise_rpc_message(buf);
    debug_print_rpc_message(msg);

    // free buffers
    buffer_free(size_buf);
    buffer_free(buf);

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

void serialise_int(buffer_t *b, int value) {
    uint64_t big_endian = htobe64(value);
    reserve_space(b, sizeof(uint64_t));
    memcpy(b->data + b->next, &big_endian, sizeof(uint64_t));
    b->next += sizeof(uint64_t);
}

int deserialise_int(buffer_t *b) {
    assert(b->next + sizeof(uint64_t) <= b->size);
    uint64_t big_endian;
    memcpy(&big_endian, b->data + b->next, sizeof(uint64_t));
    b->next += sizeof(uint64_t);
    return be64toh(big_endian);
}

unsigned int gamma_code_length(size_t x) {
    assert(x > 0);
    return 2 * (unsigned int)log2(x) + 1;
}

void serialise_size_t(buffer_t *b, size_t value) {
    // we cannot take 0 as a valid value for Elias gamma coding so we
    // increment all values by 1 and decrement them when deserialising
    value++;

    unsigned int total_len = gamma_code_length(value);
    reserve_space(b, total_len);
    unsigned char *buf = b->data + b->next;

    unsigned char *ptr = buf;
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

    b->next += total_len;
}

size_t deserialise_size_t(buffer_t *b) {
    unsigned char *buf = b->data + b->next;
    unsigned char *ptr = buf;
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

    b->next += ptr - buf;
    return value - 1;
}

void serialise_string(buffer_t *b, const char *value) {
    size_t len = strlen(value) + 1;
    serialise_size_t(b, len);
    reserve_space(b, len);
    memcpy(b->data + b->next, value, len);
    b->next += len;
}

char *deserialise_string(buffer_t *b) {
    size_t len = deserialise_size_t(b);
    char *value = new_string((char *)(b->data + b->next));
    b->next += len;
    return value;
}

void serialise_rpc_data(buffer_t *b, const rpc_data *data) {
    serialise_int(b, data->data1);
    serialise_size_t(b, data->data2_len);

    // optionally serialise data2
    if (data->data2_len > 0 && data->data2 != NULL) {
        reserve_space(b, data->data2_len);
        memcpy(b->data + b->next, data->data2, data->data2_len);
        b->next += data->data2_len;
    }
}

rpc_data *deserialise_rpc_data(buffer_t *b) {
    int data1 = deserialise_int(b);
    size_t data2_len = deserialise_size_t(b);
    rpc_data *data = new_rpc_data(data1, data2_len, b->data + b->next);
    b->next += data2_len;
    return data;
}

void serialise_rpc_message(buffer_t *b, const rpc_message *message) {
    serialise_int(b, message->request_id);
    serialise_int(b, message->operation);
    serialise_string(b, message->function_name);
    serialise_rpc_data(b, message->data);
}

rpc_message *deserialise_rpc_message(buffer_t *b) {
    int request_id = deserialise_int(b);
    int operation = deserialise_int(b);
    char *function_name = deserialise_string(b);
    rpc_data *data = deserialise_rpc_data(b);
    rpc_message *message =
        new_rpc_message(request_id, operation, function_name, data);
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
