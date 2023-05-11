/* =============================================================================
   socket.h

   Everything related to sockets.

   Author: David Sha
============================================================================= */
#ifndef SOCKETS_H
#define SOCKETS_H

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

#endif
