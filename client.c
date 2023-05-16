/* =============================================================================
   client.c

   Example client program for interacting with the RPC server.

   Author: David Sha
============================================================================= */
#include "rpc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct arguments {
    char *ip;
    char *port;
} args_t;

char *read_flag(char *flag, const char *const *valid_args, int argc,
                char *argv[]);
args_t *parse_args(int argc, char *argv[]);

int main(int argc, char *argv[]) {

    args_t *args = parse_args(argc, argv);
    if (args->ip == NULL) {
        args->ip = "::1";
    }
    if (args->port == NULL) {
        args->port = "3000";
    }

    // convert port to int
    int port = atoi(args->port);

    int exit_code = 0;

    rpc_client *state = rpc_init_client(args->ip, port);
    free(args);
    if (state == NULL) {
        exit(EXIT_FAILURE);
    }

    rpc_handle *handle_add2 = rpc_find(state, "add2");
    if (handle_add2 == NULL) {
        fprintf(stderr, "ERROR: Function add2 does not exist\n");
        exit_code = 1;
        goto cleanup;
    }

    for (int i = 0; i < 5; i++) {
        // sleep for half a second
        usleep(500000);
        /* Prepare request */
        char left_operand = i;
        char right_operand = 100;
        rpc_data request_data = {
            .data1 = left_operand, .data2_len = 1, .data2 = &right_operand};

        /* Call and receive response */
        printf("Calling add2 with %d and %d\n", left_operand, right_operand);
        rpc_data *response_data = rpc_call(state, handle_add2, &request_data);
        if (response_data == NULL) {
            fprintf(stderr, "Function call of add2 failed\n");
            exit_code = 1;
            goto cleanup;
        }

        /* Interpret response */
        assert(response_data->data2_len == 0);
        assert(response_data->data2 == NULL);
        printf("Result of adding %d and %d: %d\n", left_operand, right_operand,
               response_data->data1);
        rpc_data_free(response_data);
    }

    printf("Task 1: Client correctly finds module on server\n");
    printf("Attempting to find a function not registered on the server...\n");
    rpc_handle *handle_sub2 = rpc_find(state, "sub2");
    if (handle_sub2 != NULL) {
        fprintf(stderr, "Function sub2 exists on server\n");
        exit_code = 1;
        goto cleanup;
    } else {
        printf("✅ Function sub2 does not exist on server\n");
    }

    printf("Task 2: Remote procedure is called correctly\n");

    printf("We are done!\n");

cleanup:
    if (handle_add2 != NULL) {
        free(handle_add2);
    }



    rpc_close_client(state);
    state = NULL;

    return exit_code;
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
    args->ip = read_flag("-i", NULL, argc, argv);
    args->port = read_flag("-p", NULL, argc, argv);
    return args;
}