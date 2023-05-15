/* =============================================================================
   server.c

   Example server program for interacting with the RPC client.

   Author: David Sha
============================================================================= */
#include "rpc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct arguments {
    char *port;
} args_t;

char *read_flag(char *flag, const char *const *valid_args, int argc,
                char *argv[]);
args_t *parse_args(int argc, char *argv[]);
rpc_data *add2_i8(rpc_data *);
rpc_data *sub2_i8(rpc_data *);

int main(int argc, char *argv[]) {
    args_t *args = parse_args(argc, argv);
    if (args->port == NULL) {
        args->port = "3000";
    }
    int port = atoi(args->port);
    free(args);

    printf("Testing RPC\n");
    printf("\nrpc_init_server: %p\n", rpc_init_server);
    rpc_server *state;
    state = rpc_init_server(port);
    if (state != NULL) {
        printf("✅ is initialised\n");
    } 
    // currently cannot check for when state == NULL since malloc


    printf("\nrpc_register: %p\n", rpc_register);
    // test override
    if (rpc_register(state, "op", add2_i8) == -1) {
        printf("❌ failed\n");
    } else {
        printf("✅ is initialised\n");
    }
    if (rpc_register(state, "op", sub2_i8) == -1) {
        printf("❌ failed\n");
    } else {
        printf("✅ successfully overrides\n");
    }

    printf("\nrpc_serve_all: %p\n", rpc_serve_all);

    if (state == NULL) {
        fprintf(stderr, "Failed to init\n");
        exit(EXIT_FAILURE);
    }

    if (rpc_register(state, "add", add2_i8) == -1) {
        fprintf(stderr, "Failed to register add2\n");
        exit(EXIT_FAILURE);
    }

    rpc_serve_all(state);

    return 0;
}

/*
 * Adds 2 signed 8 bit numbers. Uses data1 for left operand and
 * data2 for right operand.
 *
 * @param in The request data
 * @return The response data
 * @note The caller is responsible for freeing the response data
 */
rpc_data *add2_i8(rpc_data *in) {
    /* Check data2 */
    if (in->data2 == NULL || in->data2_len != 1) {
        return NULL;
    }

    /* Parse request */
    char n1 = in->data1;
    char n2 = ((char *)in->data2)[0];

    /* Perform calculation */
    printf("add2: arguments %d and %d\n", n1, n2);
    int res = n1 + n2;

    /* Prepare response */
    rpc_data *out = malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}

rpc_data *sub2_i8(rpc_data *in) {
    /* Check data2 */
    if (in->data2 == NULL || in->data2_len != 1) {
        return NULL;
    }

    /* Parse request */
    char n1 = in->data1;
    char n2 = ((char *)in->data2)[0];

    /* Perform calculation */
    printf("sub: arguments %d and %d\n", n1, n2);
    int res = n1 - n2;

    /* Prepare response */
    rpc_data *out = malloc(sizeof(rpc_data));
    assert(out != NULL);
    out->data1 = res;
    out->data2_len = 0;
    out->data2 = NULL;
    return out;
}

char *read_flag(char *flag, const char *const *valid_args, int argc,
                char *argv[]) {
    /*  Given a flag and a location in the argument list, return the
        argument following the flag provided that it is in the set of
        valid arguments. Otherwise, return NULL.
    */
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], flag) == 0) {

            // if valid_args is NULL, then we don't care about the argument
            if (valid_args == NULL) {
                return argv[i + 1];
            } else {

                // check if the argument is valid
                int j = 0;
                while (valid_args[j] != NULL) {
                    if (strcmp(argv[i + 1], valid_args[j]) == 0) {
                        return argv[i + 1];
                    }
                    j++;
                }

                // if we get here, the argument was not valid
                printf("Invalid argument for flag %s. Must be one of: ", flag);
                j = 0;
                while (valid_args[j] != NULL) {
                    printf("%s ", valid_args[j]);
                    j++;
                }
                printf("\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}

args_t *parse_args(int argc, char *argv[]) {
    /*  Given a list of arguments, parse them and return all flags and
        arguments in a struct.
    */
    args_t *args;
    args = (args_t *)malloc(sizeof(*args));
    assert(args);
    args->port = read_flag("-p", NULL, argc, argv);
    return args;
}