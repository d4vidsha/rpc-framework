/* =============================================================================
   protocol.h

   Everything related to the protocol.

   Author: David Sha
============================================================================= */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "rpc.h"

/*
 * The initial size of a buffer_t when it is created. This is not always
 * the initial size if the buffer is created using new_buffer().
 */
#define INITIAL_BUFFER_SIZE 32

/*
 * The maximum size of a message in bytes which can be sent/received.
 * This is useful for specifying size of messages to the client/server
 * when sending or receiving.
 * 
 * Since we use Elias Gamma Coding for encoding the length of size_t values,
 * if the max byte size is 1 000 000, then the size of the Elias Gamma Code
 * will be 2 * floor(log2(1 000 000)) + 1 = 39 bits.
 * 
 * This is the calculation:
 *   log2(1 000 000) = 19.9...
 *   floor(19.9...) = 19
 *   2 * 19 + 1 = 39
 */
#define MAX_MESSAGE_BYTE_SIZE 1000000

/* structures =============================================================== */

/*
 * A buffer that can be used to read/write bytes to/from a socket.
 */
typedef struct {
    void *data;
    size_t next;
    size_t size;
} buffer_t;

/*
 * The payload for requests/responses.
 */
typedef struct {
    int request_id;
    enum {
        FIND,
        CALL,
        REPLY_SUCCESS,
        REPLY_FAILURE,
    } operation;
    char *function_name;
    rpc_data *data;
} rpc_message;

/* function prototypes ====================================================== */

/*
 * Create a new buffer that will initially be calloc'd to size. Subsequent
 * reallocs will be in O(log n) and not zeroed out.
 *
 * @param size The initial size of the buffer.
 * @return The new buffer.
 */
buffer_t *new_buffer(size_t size);

/*
 * Free a buffer.
 *
 * @param b The buffer to free.
 */
void buffer_free(buffer_t *b);

/*
 * Reserve space in a buffer and use realloc if necessary in O(log n).
 *
 * @param b The buffer to reserve space in.
 * @param size The number of bytes to reserve.
 */
void reserve_space(buffer_t *b, size_t size);

/*
 * Write a string of bytes to a socket.
 *
 * @param sockfd The socket to write to.
 * @param bytes The bytes to write.
 * @param size The number of bytes to write.
 */
int write_bytes(int sockfd, unsigned char *buf, size_t size);

/*
 * Read a string of bytes from a socket.
 *
 * @param sockfd The socket to read from.
 * @param buffer The buffer to read into.
 * @param size The number of bytes to read.
 */
int read_bytes(int sockfd, unsigned char *buf, size_t size);

/*
 * Print bytes.
 *
 * @param buffer: buffer to print
 * @param len: length of buffer
 */
void debug_print_bytes(const unsigned char *buffer, size_t len);

/*
 * Send an rpc_message through a socket.
 *
 * @param sockfd The socket to send the message to.
 * @param msg The message to send.
 * @return 0 if successful, -1 otherwise.
 */
int send_rpc_message(int sockfd, rpc_message *msg);

/*
 * Receive an rpc_message from a socket. 
 *
 * @param sockfd The socket to receive the message from.
 * @return The message received. NULL if there was an error.
 */
rpc_message *receive_rpc_message(int sockfd);

/*
 * Send a message through a socket and receive a response.
 * @param sockfd The socket to send the message to.
 * @param msg The message to send.
 * @return The response from the server. NULL if there was an error.
 * @note We must be wary of serialisation and endianess.
 */
rpc_message *request(int sockfd, rpc_message *msg);

/*
 * Serialise integer value into buffer. We assume that the integer value is
 * no greater than 2^63 - 1.
 *
 * @param b: buffer to serialise into
 * @param value: integer value to serialise
 */
void serialise_int(buffer_t *b, int value);

/*
 * Deserialise integer value from buffer.
 *
 * @param buffer: buffer to deserialise from
 * @return: deserialised integer value
 * @note: buffer pointer is incremented
 */
int deserialise_int(buffer_t *b);

/*
 * The length of the gamma code for a given integer value (x)
 * greater than 0.
 *
 * Formula: 2 * floor(log2(x)) + 1
 *
 * @param x The integer value.
 * @return The length of the gamma code.
 */
unsigned int gamma_code_length(size_t x);

/*
 * Serialise size_t value into buffer.
 *
 * @param buffer: buffer to serialise into
 * @param value: size_t value to serialise
 * @return: pointer to next byte in buffer
 */
void serialise_size_t(buffer_t *b, size_t value);

/*
 * Deserialise size_t value from buffer.
 *
 * @param buffer: buffer to deserialise from
 * @return: deserialised size_t value
 * @note: buffer pointer is incremented
 */
size_t deserialise_size_t(buffer_t *b);

/*
 * Serialise string value into buffer.
 *
 * @param buffer: buffer to serialise into
 * @param value: string value to serialise
 * @return: pointer to next byte in buffer
 */
void serialise_string(buffer_t *b, const char *value);

/*
 * Deserialise string value from buffer.
 *
 * @param buffer: buffer to deserialise from
 * @return: deserialised string value
 * @note: buffer pointer is incremented
 */
char *deserialise_string(buffer_t *b);

/*
 * Serialise rpc_data value into buffer.
 *
 * @param buffer: buffer to serialise into
 * @param data: rpc_data value to serialise
 * @return: pointer to next byte in buffer
 */
void serialise_rpc_data(buffer_t *b, const rpc_data *data);

/*
 * Deserialise rpc_data value from buffer. If rpc_data is inconsistent,
 * then we try to give as much information as possible and return a 
 * partially deserialised rpc_data value.
 *
 * @param buffer: buffer to deserialise from
 * @return: deserialised rpc_data value
 * @note: buffer pointer is incremented
 */
rpc_data *deserialise_rpc_data(buffer_t *b);

/*
 * Serialise rpc_message value into buffer.
 *
 * @param buffer: buffer to serialise into
 * @param msg: rpc_message value to serialise
 * @return: pointer to next byte in buffer
 */
void serialise_rpc_message(buffer_t *b, const rpc_message *message);

/*
 * Deserialise rpc_message value from buffer. If rpc_data is inconsistent,
 * then we return NULL.
 *
 * @param buffer: buffer to deserialise from
 * @return: deserialised rpc_message value
 * @note: buffer pointer is incremented
 */
rpc_message *deserialise_rpc_message(buffer_t *b);

/*
 * Create a new string.
 *
 * @param value: string value
 * @return: new string
 */
char *new_string(const char *value);

/*
 * Create a new RPC data.
 *
 * @param data1 The first data value.
 * @param data2 The second data value.
 * @param data2_len The length of the second data value.
 * @return The new RPC data.
 */
rpc_data *new_rpc_data(int data1, size_t data2_len, void *data2);

/*
 * Create a new RPC message.
 *
 * @param request_id The request ID.
 * @param operation The operation to perform.
 * @param handle The handle to perform the operation on.
 * @param data The data to send.
 * @return The new RPC message.
 */
rpc_message *new_rpc_message(int request_id, int operation, char *function_name,
                             rpc_data *data);

/*
 * Free an RPC message.
 *
 * @param message The RPC message to free.
 */
void rpc_message_free(rpc_message *message, void (*free_data)(rpc_data *));

/*
 * Create a default failure message so that we can send it to the client.
 * from the server.
 * 
 * @return The failure message.
 */
rpc_message *create_failure_message();

/*
 * Print an RPC data.
 *
 * @param data The RPC data to print.
 */
void debug_print_rpc_data(rpc_data *data);

/*
 * Print an RPC message.
 *
 * @param message The RPC message to print.
 */
void debug_print_rpc_message(rpc_message *message);

#endif
