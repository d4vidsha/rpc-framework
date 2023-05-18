/* =============================================================================
   config.h

   Configuration variables and constants.

   References:
   - Choice of listen backlog: https://stackoverflow.com/a/10002936
   - Debugging macro from: https://stackoverflow.com/a/1644898

   Author: David Sha
============================================================================= */
#ifndef CONFIG_H
#define CONFIG_H

/* #defines ================================================================= */

/*
 * Print debug messages by setting this to TRUE.
 */
#define DEBUG TRUE

/*
 * Print debug messages.
 */
#define debug_print(fmt, ...)                                                  \
    do {                                                                       \
        if (DEBUG)                                                             \
            fprintf(stderr, fmt, __VA_ARGS__);                                 \
    } while (0)

/*
 * Check ptr is not NULL, then free it and set it to NULL.
 */
#define free_and_null(ptr)                                                     \
    do {                                                                       \
        if (ptr) {                                                             \
            free(ptr);                                                         \
            ptr = NULL;                                                        \
        }                                                                      \
    } while (0)

/*
 * Basic constants.
 */
#define TRUE 1
#define FALSE 0
#define FAILED -1

/*
 * Maximum port is 65535 which is 5 digits.
 */
#define MAX_PORT_LENGTH 5

/*
 * Maximum length of a function name.
 */
#define MAX_NAME_LENGTH 1000

/*
 * Size of the hashtable. The larger this is, the less likely there will be
 * collisions. However, the larger this is, the more memory it will take up.
 */
#define HASHTABLE_SIZE 100

/*
 * How many connections are kept in the backlog queue before rejecting new
 * connections using the accept() system call.
 */
#define BACKLOG 128

/*
 * Indicates that this RPC will be non-blocking. Requests will be managed in
 * separate threads allowing for concurrent execution.
 */
#define NONBLOCKING

#endif
