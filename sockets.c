/* =============================================================================
   socket.c

   Everything related to sockets.

   Author: David Sha
============================================================================= */
#define _POSIX_C_SOURCE 200112L
#include "sockets.h"
#include "config.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

int create_listening_socket(char *port) {
    int re, s, sockfd;
    struct addrinfo hints, *res;

    // create address we're going to listen on (with given port number)
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;      // use IPv6
    hints.ai_socktype = SOCK_STREAM; // use TCP
    hints.ai_flags = AI_PASSIVE;     // for bind, listen, accept

    // get address info with above parameters
    if ((s = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // create socket
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // set socket to be reusable
    re = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof re) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // bind address to socket
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    return sockfd;
}

int create_connection_socket(char *addr, char *port) {
    int s, sockfd = FAILED;
    struct addrinfo hints, *servinfo, *rp;

    // create address we're going to connect to
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;      // use IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP sockets

    // get address info for addr
    if ((s = getaddrinfo(addr, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
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
        fprintf(stderr, "Could not connect to server\n");
        sockfd = FAILED;
    }

    freeaddrinfo(servinfo);
    return sockfd;
}

int non_blocking_accept(int sockfd, struct sockaddr_in *client_addr,
                        socklen_t *client_addr_size) {
    int new_sockfd;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    struct timeval tv = {.tv_sec = 0, .tv_usec = 0};
    int retval = select(sockfd + 1, &readfds, NULL, NULL, &tv);
    if (retval == FAILED) {
        perror("select");
        exit(EXIT_FAILURE);
    } else if (retval) {
        // connection request received
        new_sockfd = accept(sockfd, (struct sockaddr *)client_addr,
                        client_addr_size);
        if (new_sockfd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        return new_sockfd;
    } else {
        // no connection requests received
        return FAILED;
    }
}

int is_socket_closed(int sockfd) {
    char buf[1];
    ssize_t n = recv(sockfd, buf, sizeof(buf), MSG_PEEK);
    if (n == 0) {
        // socket is closed
        close(sockfd);
        return 1;
    } else if (n == -1) {
        perror("recv");
        return 0;
    } else {
        // socket is open
        return 0;
    }
}
