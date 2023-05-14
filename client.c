/* =============================================================================
   client.c

   Example client program for interacting with the RPC server.

   Author: David Sha
============================================================================= */
#include "rpc.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int exit_code = 0;

    rpc_client *state = rpc_init_client("::1", 3000);
    if (state == NULL) {
        exit(EXIT_FAILURE);
    }

    rpc_handle *handle_add2 = rpc_find(state, "add2");
    if (handle_add2 == NULL) {
        fprintf(stderr, "ERROR: Function add2 does not exist\n");
        exit_code = 1;
        goto cleanup;
    }

    for (int i = 0; i < 2; i++) {
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

    printf("Task 2: Remote procedure is called correctly");
    

    printf("We are done!\n");

cleanup:
    if (handle_add2 != NULL) {
        free(handle_add2);
    }

    rpc_close_client(state);
    state = NULL;

    return exit_code;
}
