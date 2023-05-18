/* =============================================================================
   socket.c

   Everything related to sockets.

   Author: David Sha
============================================================================= */
#define _POSIX_C_SOURCE 200112L
#include "config.h"
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

int create_listening_socket(char *port) {
    int re, s, sockfd = FAILED;
    struct addrinfo hints, *res = NULL;

    // create address we're going to listen on (with given port number)
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;      // use IPv6
    hints.ai_socktype = SOCK_STREAM; // use TCP
    hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept

    // get address info with above parameters
    if ((s = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        debug_print("getaddrinfo: %s\n", gai_strerror(s));
        goto cleanup;
    }

    // create socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        debug_print("%s", "Error creating socket\n");
        goto cleanup;
    }

    // set socket to be reusable
    re = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof re) < 0) {
        debug_print("%s", "Error setting socket options\n");
        goto cleanup;
    }

    // bind address to socket
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        debug_print("%s", "Error binding socket\n");
        goto cleanup;
    }

cleanup:
    if (res) {
        freeaddrinfo(res);
    }
    return sockfd;
}

int create_connection_socket(char *addr, char *port) {
    int s, sockfd = FAILED;
    struct addrinfo hints, *servinfo = NULL, *rp = NULL;

    // create address we're going to connect to
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;      // use IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP sockets

    // get address info for addr
    if ((s = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
        debug_print("getaddrinfo: %s\n", gai_strerror(s));
        goto cleanup;
    }

    // connect to remote host
    for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == FAILED) {
            continue;
        }
        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != FAILED) {
            break;
        }
        close(sockfd);
    }

    if (rp == NULL) {
        debug_print("%s", "Could not connect to server\n");
        goto cleanup;
    }

cleanup:
    if (servinfo) {
        freeaddrinfo(servinfo);
    }
    return sockfd;
}

int non_blocking_accept(int sockfd, struct sockaddr_in *client_addr,
                        socklen_t *client_addr_size) {
    int new_sockfd = FAILED;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    struct timeval tv = {.tv_sec = 0, .tv_usec = 0};
    int retval = select(sockfd + 1, &readfds, NULL, NULL, &tv);
    if (retval == FAILED) {
        debug_print("%s", "Error in select\n");
    } else if (retval) {
        // connection request received
        new_sockfd =
            accept(sockfd, (struct sockaddr *)client_addr, client_addr_size);
        if (new_sockfd < 0) {
            debug_print("%s", "Error accepting connection\n");
            new_sockfd = FAILED;
        }
    } else {
        // no connection requests received
    }
    return new_sockfd;
}

int is_socket_closed(int sockfd) {
    char buf[1];
    ssize_t n = recv(sockfd, buf, sizeof(buf), MSG_PEEK);
    if (n == 0) {
        // socket is closed
        close(sockfd);
        return TRUE;
    } else if (n == FAILED) {
        perror("recv");
        return FAILED;
    } else {
        // socket is open
        return FALSE;
    }
}
