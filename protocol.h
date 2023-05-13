/* =============================================================================
   protocol.h

   Everything related to the protocol.

   Author: David Sha
============================================================================= */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "rpc.h"

/* structures =============================================================== */

/*
 * The payload for requests/responses.
 */
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

/* function prototypes ====================================================== */

/*
 * Write a string of bytes to a socket.
 *
 * @param sockfd The socket to write to.
 * @param bytes The bytes to write.
 * @param size The number of bytes to write.
 */
int write_bytes(int sockfd, const unsigned char *bytes, int size);

/*
 * Read a string of bytes from a socket.
 *
 * @param sockfd The socket to read from.
 * @param buffer The buffer to read into.
 * @param size The number of bytes to read.
 */
int read_bytes(int sockfd, unsigned char *buffer, int size);

/*
 * Print bytes.
 * 
 * @param buffer: buffer to print
 * @param len: length of buffer
 */
void print_bytes(const unsigned char *buffer, size_t len);

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
 * Serialise integer value into buffer.
 * 
 * @param buffer: buffer to serialise into
 * @param value: integer value to serialise
 * @return: pointer to next byte in buffer
 */
unsigned char *serialise_int(unsigned char *buffer, int value);

/*
 * Deserialise integer value from buffer.
 * 
 * @param buffer: buffer to deserialise from
 * @return: deserialised integer value
 * @note: buffer pointer is incremented
 */
int deserialise_int(unsigned char **buffer_ptr);

/*
 * Serialise size_t value into buffer.
 * 
 * @param buffer: buffer to serialise into
 * @param value: size_t value to serialise
 * @return: pointer to next byte in buffer
 */
unsigned char *serialise_size_t(unsigned char *buffer, size_t value);

/*
 * Deserialise size_t value from buffer.
 * 
 * @param buffer: buffer to deserialise from
 * @return: deserialised size_t value
 * @note: buffer pointer is incremented
 */
size_t deserialise_size_t(unsigned char **buffer_ptr);

/*
 * Serialise string value into buffer.
 * 
 * @param buffer: buffer to serialise into
 * @param value: string value to serialise
 * @return: pointer to next byte in buffer
 */
unsigned char *serialise_string(unsigned char *buffer, const char *value);

/*
 * Deserialise string value from buffer.
 * 
 * @param buffer: buffer to deserialise from
 * @return: deserialised string value
 * @note: buffer pointer is incremented
 */
char *deserialise_string(unsigned char **buffer_ptr);

/*
 * Serialise rpc_data value into buffer.
 * 
 * @param buffer: buffer to serialise into
 * @param data: rpc_data value to serialise
 * @return: pointer to next byte in buffer
 */
unsigned char *serialise_rpc_data(unsigned char *buffer, const rpc_data *data);

/*
 * Deserialise rpc_data value from buffer.
 * 
 * @param buffer: buffer to deserialise from
 * @return: deserialised rpc_data value
 * @note: buffer pointer is incremented
 */ 
rpc_data *deserialise_rpc_data(unsigned char **buffer_ptr);

/*
 * Serialise rpc_message value into buffer.
 * 
 * @param buffer: buffer to serialise into
 * @param msg: rpc_message value to serialise
 * @return: pointer to next byte in buffer
 */
unsigned char *serialise_rpc_message(unsigned char *buffer, const rpc_message *message);

/*
 * Deserialise rpc_message value from buffer.
 * 
 * @param buffer: buffer to deserialise from
 * @return: deserialised rpc_message value
 * @note: buffer pointer is incremented
 */
rpc_message *deserialise_rpc_message(unsigned char **buffer_ptr);

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
rpc_message *new_rpc_message(int request_id, int operation, char *function_name, rpc_data *data);

/*
 * Free an RPC message.
 *
 * @param message The RPC message to free.
 */
void free_rpc_message(rpc_message *message, void (*free_data)(rpc_data *));

#endif
