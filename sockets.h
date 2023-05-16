/* =============================================================================
   socket.h

   Everything related to sockets.

   Author: David Sha
============================================================================= */
#ifndef SOCKETS_H
#define SOCKETS_H

#include <arpa/inet.h>

/* function prototypes ====================================================== */

/*
 * Create a socket given a port number.
 *
 * @param port The port number to bind to.
 * @return A file descriptor for the socket.
 * @note This function was adapted from week 9 tute.
 */
int create_listening_socket(char *port);

/*
 * Create a socket given an address and port number.
 *
 * @param addr The address to connect to.
 * @param port The port number to connect to.
 * @return A file descriptor for the socket.
 * @note This function was adapted from week 9 tute.
 */
int create_connection_socket(char *addr, char *port);

/*
 * Accept a connection from a client in a non-blocking manner. This assumes
 * this function is run within a loop.
 *
 * @param sockfd The socket file descriptor.
 * @param client_addr The client's address that will be populated by this
 * function call.
 * @param client_addr_size The size of the client's address that is populated
 * during this function call.
 * @return The client's socket file descriptor, or -1 if no client is connected.
 */
int non_blocking_accept(int sockfd, struct sockaddr_in *client_addr,
                        socklen_t *client_addr_size);

/*
 * Checks if a socket is closed.
 *
 * @param sockfd The socket file descriptor.
 * @return 1 if the socket is closed, 0 otherwise.
 */
int is_socket_closed(int sockfd);

#endif
