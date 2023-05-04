#define _POSIX_C_SOURCE 200112L
#include "rpc.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct rpc_server {
    int port;
    int sockfd;
};

/*
 * Initialises a rpc_server struct and binds to the given port.
 *
 * @param port The port to bind to.
 * @return A pointer to a rpc_server struct, or NULL on failure.
 * @note This function is called before rpc_register.
 */
rpc_server *rpc_init_server(int port) {



    return NULL;
}

/*
 * Register a handler for a given name. At the server, let the
 * subsystem know what function to call when an incoming request
 * with the given name is received.
 *
 * @param srv The server to register the handler with.
 * @param name The name of the function.
 * @param handler The function to call when a request with the 
 * given name is received.
 * @return 0 on success, -1 on failure. If any of the parameters
 * are NULL, return -1.
 * @note TODO: Possibly use ID for the handler as the return value
 * instead of 0.
 */
int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    
    
    
    return -1;
}

/*
 * Server function to handle incoming requests. This function will wait
 * for incoming requests for any registered functions, or rpc_find, on
 * the port specified in rpc_init_server of any interface. If it is a
 * function call request, it will call the requested function, send a
 * reply to the caller, and resume waiting for new requests. If it is 
 * rpc_find, it will reply to the caller saying whether the name was
 * found or not, or possibly an error code.
 * 
 * @param srv The server to serve requests for.
 * @note This function should only return if srv is NULL or handling
 * SIGINT.
 */
void rpc_serve_all(rpc_server *srv) {



    return;
}

struct rpc_client {
};

struct rpc_handle {

};

/*
 * Initialises a client state.
 *
 * @param addr The address of the server to connect to.
 * @param port The port of the server to connect to.
 * @return rpc_client*, or NULL on failure.
 * @note This function is called before rpc_find or rpc_call.
 */
rpc_client *rpc_init_client(char *addr, int port) {



    return NULL;
}

/*
 * Tell the subsystem what details are required to place a call.
 * 
 * @param cl The client to use.
 * @param name The name of the function to call.
 * @return A handle for the remote procedure, or NULL if the name
 * is not registered or if any of the parameters are NULL, or if
 * the find request fails.
 */
rpc_handle *rpc_find(rpc_client *cl, char *name) {
    


    return NULL;
}

/*
 * Call a remote procedure. This function causes the subsystem to
 * run the remote procedure, and returns the value.
 * 
 * @param cl The client to use.
 * @param h The handle for the remote procedure to call.
 * @param payload The data to send to the remote procedure.
 * @return The data returned by the remote procedure, or NULL if
 * the call fails, or if any of the parameters are NULL.
 * @note The returned data should be freed by the caller using
 * rpc_data_free.
 */
rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {



    return NULL;
}

/*
 * Clean up the client state and close the connection. If an already
 * closed client is passed, do nothing.
 *
 * @param cl The client to close.
 * @note This function is called after the final rpc_call or rpc_find.
 */
void rpc_close_client(rpc_client *cl) {



    return;
}

/*
 * Free the memory allocated to rpc_data struct.
 *
 * @param data The struct to free.
 */
void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}
