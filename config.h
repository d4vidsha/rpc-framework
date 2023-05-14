/* =============================================================================
   config.h

   Configuration variables and constants.

   Author: David Sha
============================================================================= */
#ifndef CONFIG_H
#define CONFIG_H

/* #defines ================================================================= */
#define DEBUG FALSE

// Debugging macro from: https://stackoverflow.com/a/1644898
#define debug_print(fmt, ...)                                                  \
    do {                                                                       \
        if (DEBUG)                                                             \
            fprintf(stderr, fmt, __VA_ARGS__);                                 \
    } while (0)

#define TRUE 1
#define FALSE 0
#define FAILED -1

#define MAX_PORT_LENGTH 5
#define MAX_NAME_LENGTH 1000

#define HASHTABLE_SIZE 100

#define BACKLOG 100

#endif
