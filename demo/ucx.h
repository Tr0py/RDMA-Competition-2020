/**
* Copyright (C) Mellanox Technologies Ltd. 2018.  ALL RIGHTS RESERVED.
*
* See file LICENSE for terms.
*/

/*
 * UCP client - server example utility
 * -----------------------------------------------
 *
 * Server side:
 *
 *    ./ucp_client_server
 *
 * Client side:
 *
 *    ./ucp_client_server -a <server-ip>
 *
 * Notes:
 *
 *    - The server will listen to incoming connection requests on INADDR_ANY.
 *    - The client needs to pass the IP address of the server side to connect to
 *      as an argument to the test.
 *    - Currently, the passed IP needs to be an IPoIB or a RoCE address.
 *    - The port which the server side would listen on can be modified with the
 *      '-p' option and should be used on both sides. The default port to use is
 *      13337.
 */

#include <ucp/api/ucp.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>    /* memset */
#include <arpa/inet.h> /* inet_addr */
#include <unistd.h>    /* getopt */
#include <stdlib.h>    /* atoi */

#define DEFAULT_PORT           13337
#define IP_STRING_LEN          50
#define PORT_STRING_LEN        8
#define TAG                    0xCAFE
#define COMM_TYPE_DEFAULT      "STREAM"
#define PRINT_INTERVAL         2000
#define DEFAULT_NUM_ITERATIONS 10
#define CHECKER 0x55

char *test_message           = NULL;
char *recv_message           = NULL;
static uint16_t server_port          = DEFAULT_PORT;
static int num_iterations            = DEFAULT_NUM_ITERATIONS;

int TEST_BUFFER_LEN = 0x1000000;
int EVERY_SEND_LEN = 0;
int RECEIVED_BUFFER_SIZE = 0;
int RECEIVED_THIS = 0;
int EVERY_SEND_LEN_BEGIN = 4, EVERY_SEND_LEN_END = 24;

typedef enum {
    CLIENT_SERVER_SEND_RECV_STREAM  = UCS_BIT(0),
    CLIENT_SERVER_SEND_RECV_TAG     = UCS_BIT(1),
    CLIENT_SERVER_SEND_RECV_DEFAULT = CLIENT_SERVER_SEND_RECV_TAG
} send_recv_type_t;


/**
 * Server's application context to be used in the user's connection request
 * callback.
 * It holds the server's listener and the handle to an incoming connection request.
 */
typedef struct ucx_server_ctx {
    volatile ucp_conn_request_h conn_request;
    ucp_listener_h              listener;
} ucx_server_ctx_t;


/**
 * Stream request context. Holds a value to indicate whether or not the
 * request is completed.
 */
typedef struct test_req {
    int complete;

} test_req_t;


void init_buffer(){
    test_message = (char *)(malloc(sizeof(char) * TEST_BUFFER_LEN));
    recv_message = (char *)(malloc(sizeof(char) * TEST_BUFFER_LEN));
    int i;
    for (i = 0; i < TEST_BUFFER_LEN; ++i){
        test_message[i] = (char)(rand() % 255);
    }
    test_message[0] = CHECKER;
}

void free_buffer(){
    free(test_message);
    free(recv_message);
}

static void tag_recv_cb(void *request, ucs_status_t status,
                        ucp_tag_recv_info_t *info)
{
    test_req_t *req = request;
    RECEIVED_THIS = info->length;
    req->complete = 1;
}

/**
 * The callback on the receiving side, which is invoked upon receiving the
 * stream message.
 */
static void stream_recv_cb(void *request, ucs_status_t status, size_t length)
{
    test_req_t *req = request;
    RECEIVED_THIS = length;
    req->complete = 1;
}

/**
 * The callback on the sending side, which is invoked after finishing sending
 * the message.
 */
static void send_cb(void *request, ucs_status_t status)
{
    test_req_t *req = request;

    req->complete = 1;
}

/**
 * Error handling callback.
 */
static void err_cb(void *arg, ucp_ep_h ep, ucs_status_t status)
{
    printf("error handling callback was invoked with status %d (%s)\n",
           status, ucs_status_string(status));
}

/**
 * Set an address for the server to listen on - INADDR_ANY on a well known port.
 */
void set_listen_addr(const char *address_str, struct sockaddr_in *listen_addr)
{
    /* The server will listen on INADDR_ANY */
    memset(listen_addr, 0, sizeof(struct sockaddr_in));
    listen_addr->sin_family      = AF_INET;
    listen_addr->sin_addr.s_addr = (address_str) ? inet_addr(address_str) : INADDR_ANY;
    listen_addr->sin_port        = htons(server_port);
}

/**
 * Set an address to connect to. A given IP address on a well known port.
 */
void set_connect_addr(const char *address_str, struct sockaddr_in *connect_addr)
{
    memset(connect_addr, 0, sizeof(struct sockaddr_in));
    connect_addr->sin_family      = AF_INET;
    connect_addr->sin_addr.s_addr = inet_addr(address_str);
    connect_addr->sin_port        = htons(server_port);
}

/**
 * Initialize the client side. Create an endpoint from the client side to be
 * connected to the remote server (to the given IP).
 */
static ucs_status_t start_client(ucp_worker_h ucp_worker, const char *ip,
                                 ucp_ep_h *client_ep)
{
    ucp_ep_params_t ep_params;
    struct sockaddr_in connect_addr;
    ucs_status_t status;

    set_connect_addr(ip, &connect_addr);

    /*
     * Endpoint field mask bits:
     * UCP_EP_PARAM_FIELD_FLAGS             - Use the value of the 'flags' field.
     * UCP_EP_PARAM_FIELD_SOCK_ADDR         - Use a remote sockaddr to connect
     *                                        to the remote peer.
     * UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE - Error handling mode - this flag
     *                                        is temporarily required since the
     *                                        endpoint will be closed with
     *                                        UCP_EP_CLOSE_MODE_FORCE which
     *                                        requires this mode.
     *                                        Once UCP_EP_CLOSE_MODE_FORCE is
     *                                        removed, the error handling mode
     *                                        will be removed.
     */
    ep_params.field_mask       = UCP_EP_PARAM_FIELD_FLAGS       |
                                 UCP_EP_PARAM_FIELD_SOCK_ADDR   |
                                 UCP_EP_PARAM_FIELD_ERR_HANDLER |
                                 UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE;
    ep_params.err_mode         = UCP_ERR_HANDLING_MODE_PEER;
    ep_params.err_handler.cb   = err_cb;
    ep_params.err_handler.arg  = NULL;
    ep_params.flags            = UCP_EP_PARAMS_FLAGS_CLIENT_SERVER;
    ep_params.sockaddr.addr    = (struct sockaddr*)&connect_addr;
    ep_params.sockaddr.addrlen = sizeof(connect_addr);

    status = ucp_ep_create(ucp_worker, &ep_params, client_ep);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to connect to %s (%s)\n", ip, ucs_status_string(status));
    }

    return status;
}


/**
 * Progress the request until it completes.
 */
static ucs_status_t request_wait(ucp_worker_h ucp_worker, test_req_t *request)
{
    ucs_status_t status;

    /* if operation was completed immediately */
    if (request == NULL) {
        return UCS_OK;
    }
    
    if (UCS_PTR_IS_ERR(request)) {
        return UCS_PTR_STATUS(request);
    }
    
    while (request->complete == 0) {
        ucp_worker_progress(ucp_worker);
    }
    status = ucp_request_check_status(request);

    /* This request may be reused so initialize it for next time */
    request->complete = 0;
    ucp_request_free(request);

    return status;
}

static int request_finalize(ucp_worker_h ucp_worker, test_req_t *request,
                            int is_server, int iterations)
{
    ucs_status_t status;
    int ret = 0;

    status = request_wait(ucp_worker, request);
    if (status != UCS_OK) {
        fprintf(stderr, "unable to %s UCX message (%s)\n",
                is_server ? "receive": "send", ucs_status_string(status));
        return -1;
    }

    return ret;
}

/**
 * Send and receive a message using the Stream API.
 * The client sends a message to the server and waits until the send it completed.
 * The server receives a message from the client and waits for its completion.
 */
static int send_recv_stream(ucp_worker_h ucp_worker, ucp_ep_h ep, int is_server,
                            int iterations)
{
    test_req_t *request;
    size_t length;

    int need_length = 0;
    int err = 0;
    int iter = 0;

    if (!is_server) {
        double Gbps = 0;
        for (iter = 0; iter < iterations; ++iter){
            int i;
            struct timeval tv_begin, tv_end;
            need_length = EVERY_SEND_LEN;
            /* Warm Up */
            for (i = 0; i < TEST_BUFFER_LEN / EVERY_SEND_LEN; ++i){
                /* Client sends a message to the server using the stream API */
                request = ucp_stream_send_nb(ep, test_message + i * EVERY_SEND_LEN, 1,
                                            ucp_dt_make_contig(need_length),
                                            send_cb, 0);
                err = request_finalize(ucp_worker, request, is_server, 
                                iterations);
                if (err) return err;
            }
            /* Get RTT */
            gettimeofday(&tv_begin, NULL);
            request = ucp_stream_send_nb(ep, test_message, 1,
                    ucp_dt_make_contig(1),
                    send_cb, 0);
            err = request_finalize(ucp_worker, request, is_server, 
                    iterations);
    
            request = ucp_stream_recv_nb(ep, recv_message, 1,
                                            ucp_dt_make_contig(1),
                                            stream_recv_cb, &length,
                                            UCP_STREAM_RECV_FLAG_WAITALL);
            err = request_finalize(ucp_worker, request, !is_server, 
                                iterations);

            gettimeofday(&tv_end, NULL);
            long RTT = (tv_end.tv_sec - tv_begin.tv_sec) * 1000000 + (tv_end.tv_usec - tv_begin.tv_usec);

            /* Begin */
            gettimeofday(&tv_begin, NULL);
            for (i = 0; i < TEST_BUFFER_LEN / EVERY_SEND_LEN; ++i){
                /* Client sends a message to the server using the stream API */
                request = ucp_stream_send_nb(ep, test_message + i * EVERY_SEND_LEN, 1,
                                            ucp_dt_make_contig(need_length),
                                            send_cb, 0);
                err = request_finalize(ucp_worker, request, is_server, 
                                iterations);
                if (err) return err;
            }
            request = ucp_stream_recv_nb(ep, recv_message, 1,
                                            ucp_dt_make_contig(1),
                                            stream_recv_cb, &length,
                                            UCP_STREAM_RECV_FLAG_WAITALL);
            err = request_finalize(ucp_worker, request, !is_server, 
                                iterations);
            gettimeofday(&tv_end, NULL);
            if (err) return err;
            if (recv_message[0] != CHECKER) return -1;
            long usec = (tv_end.tv_sec - tv_begin.tv_sec) * 1000000 + (tv_end.tv_usec - tv_begin.tv_usec) - RTT / 2;
            Gbps += (TEST_BUFFER_LEN / 1000. / 1000. / 1000. * 8) / (usec / 1000. / 1000.);
        }
        printf("%lf Gbps", Gbps / iterations);
        return 0;
    } else {
        for (iter = 0; iter < iterations; ++iter){
            /* Warm Up */
            RECEIVED_BUFFER_SIZE = 0;
            while (RECEIVED_BUFFER_SIZE < TEST_BUFFER_LEN){
                need_length = TEST_BUFFER_LEN - RECEIVED_BUFFER_SIZE;
                /* Server receives a message from the client using the stream API */
                request = ucp_stream_recv_nb(ep, recv_message + RECEIVED_BUFFER_SIZE, 1,
                                            ucp_dt_make_contig(need_length),
                                            stream_recv_cb, &length,
                                            UCP_STREAM_RECV_FLAG_WAITALL);
                err = request_finalize(ucp_worker, request, is_server, 
                                iterations);
                if (err) return err;
                RECEIVED_BUFFER_SIZE += RECEIVED_THIS;
                RECEIVED_THIS = 0;
            }
            if (recv_message[0] != CHECKER) return -1;
            /* Get RTT */
            request = ucp_stream_recv_nb(ep, recv_message + RECEIVED_BUFFER_SIZE, 1,
                    ucp_dt_make_contig(1),
                    stream_recv_cb, &length,
                    UCP_STREAM_RECV_FLAG_WAITALL);
            err = request_finalize(ucp_worker, request, is_server, 
                    iterations);

            request = ucp_stream_send_nb(ep, test_message, 1,
                                            ucp_dt_make_contig(1),
                                            send_cb, 0);
            err = request_finalize(ucp_worker, request, !is_server, 
                                iterations);

    
            if (err) return err;

            /* Begin */
            RECEIVED_BUFFER_SIZE = 0;
            while (RECEIVED_BUFFER_SIZE < TEST_BUFFER_LEN){
                need_length = TEST_BUFFER_LEN - RECEIVED_BUFFER_SIZE;
                /* Server receives a message from the client using the stream API */
                request = ucp_stream_recv_nb(ep, recv_message + RECEIVED_BUFFER_SIZE, 1,
                                            ucp_dt_make_contig(need_length),
                                            stream_recv_cb, &length,
                                            UCP_STREAM_RECV_FLAG_WAITALL);
                err = request_finalize(ucp_worker, request, is_server, 
                                iterations);
                if (err) return err;
                RECEIVED_BUFFER_SIZE += RECEIVED_THIS;
                RECEIVED_THIS = 0;
            }
            if (recv_message[0] != CHECKER) return -1;
            request = ucp_stream_send_nb(ep, test_message, 1,
                                            ucp_dt_make_contig(1),
                                            send_cb, 0);
            err = request_finalize(ucp_worker, request, !is_server, 
                            iterations);
            if (err) return err;
        }
        return 0;
    }
}

/**
 * Send and receive a message using the Tag-Matching API.
 * The client sends a message to the server and waits until the send it completed.
 * The server receives a message from the client and waits for its completion.
 */
static int send_recv_tag(ucp_worker_h ucp_worker, ucp_ep_h ep, int is_server,
                         int iterations)
{
    //printf("in tag\n");
    test_req_t *request;
    size_t length;

    int need_length = 0;
    int err = 0;
    int iter = 0;

    if (!is_server) {
        double Gbps = 0;
        for (iter = 0; iter < iterations; ++iter){
            int i;
            struct timeval tv_begin, tv_end;
            need_length = EVERY_SEND_LEN;
            /* Warm Up */
            for (i = 0; i < TEST_BUFFER_LEN / EVERY_SEND_LEN; ++i){
                /* Client sends a message to the server using the stream API */
                request = ucp_tag_send_nb(ep, test_message + i * EVERY_SEND_LEN, 1,
                                            ucp_dt_make_contig(need_length), TAG,
                                            send_cb);
                err = request_finalize(ucp_worker, request, is_server, 
                                iterations);
                if (err) return err;
            }
            /* Get RTT */
            gettimeofday(&tv_begin, NULL);
            request = ucp_tag_send_nb(ep, test_message, 1,
                    ucp_dt_make_contig(1), TAG, 
                    send_cb);
            err = request_finalize(ucp_worker, request, is_server, 
                    iterations);
    
            request = ucp_tag_recv_nb(ucp_worker, recv_message, 1,
                                            ucp_dt_make_contig(1), TAG, 0,
                                            tag_recv_cb);
            err = request_finalize(ucp_worker, request, !is_server, 
                                iterations);

            gettimeofday(&tv_end, NULL);
            long RTT = (tv_end.tv_sec - tv_begin.tv_sec) * 1000000 + (tv_end.tv_usec - tv_begin.tv_usec);

            /* Begin */
            gettimeofday(&tv_begin, NULL);
            for (i = 0; i < TEST_BUFFER_LEN / EVERY_SEND_LEN; ++i){
                /* Client sends a message to the server using the stream API */
                request = ucp_tag_send_nb(ep, test_message + i * EVERY_SEND_LEN, 1,
                                            ucp_dt_make_contig(need_length), TAG,
                                            send_cb);
                err = request_finalize(ucp_worker, request, is_server, 
                                iterations);
                if (err) return err;
            }
            request = ucp_tag_recv_nb(ucp_worker, recv_message, 1,
                                            ucp_dt_make_contig(1), TAG, 0, tag_recv_cb);
            err = request_finalize(ucp_worker, request, !is_server, 
                                iterations);
            gettimeofday(&tv_end, NULL);
            if (err) return err;
            if (recv_message[0] != CHECKER) return -1;
            long usec = (tv_end.tv_sec - tv_begin.tv_sec) * 1000000 + (tv_end.tv_usec - tv_begin.tv_usec) - RTT / 2;
            Gbps += (TEST_BUFFER_LEN / 1000. / 1000. / 1000. * 8) / (usec / 1000. / 1000.);
        }
        printf("%lf Gbps", Gbps / iterations);
        return 0;
    } else {
        for (iter = 0; iter < iterations; ++iter){
            /* Warm Up */
            RECEIVED_BUFFER_SIZE = 0;
            while (RECEIVED_BUFFER_SIZE < TEST_BUFFER_LEN){
                need_length = TEST_BUFFER_LEN - RECEIVED_BUFFER_SIZE;
                /* Server receives a message from the client using the stream API */
                request = ucp_tag_recv_nb(ucp_worker, recv_message + RECEIVED_BUFFER_SIZE, 1,
                                            ucp_dt_make_contig(need_length), TAG, 0,
                                            tag_recv_cb);
                err = request_finalize(ucp_worker, request, is_server, 
                                iterations);
                if (err) return err;
                RECEIVED_BUFFER_SIZE += RECEIVED_THIS;
                RECEIVED_THIS = 0;
            }
            if (recv_message[0] != CHECKER) return -1;
            /* Get RTT */
            request = ucp_tag_recv_nb(ucp_worker, recv_message + RECEIVED_BUFFER_SIZE, 1,
                    ucp_dt_make_contig(1), TAG, 0, tag_recv_cb);
            err = request_finalize(ucp_worker, request, is_server, 
                    iterations);

            request = ucp_tag_send_nb(ep, test_message, 1,
                                            ucp_dt_make_contig(1), TAG,
                                            send_cb);
            err = request_finalize(ucp_worker, request, !is_server, 
                                iterations);

    
            if (err) return err;

            /* Begin */
            RECEIVED_BUFFER_SIZE = 0;
            while (RECEIVED_BUFFER_SIZE < TEST_BUFFER_LEN){
                need_length = TEST_BUFFER_LEN - RECEIVED_BUFFER_SIZE;
                /* Server receives a message from the client using the stream API */
                request = ucp_tag_recv_nb(ucp_worker, recv_message + RECEIVED_BUFFER_SIZE, 1,
                                            ucp_dt_make_contig(need_length), TAG, 0,
                                            tag_recv_cb);
                err = request_finalize(ucp_worker, request, is_server, 
                                iterations);
                if (err) return err;
                RECEIVED_BUFFER_SIZE += RECEIVED_THIS;
                RECEIVED_THIS = 0;
            }
            if (recv_message[0] != CHECKER) return -1;
            request = ucp_tag_send_nb(ep, test_message, 1,
                                            ucp_dt_make_contig(1), TAG,
                                            send_cb);
            err = request_finalize(ucp_worker, request, !is_server, 
                            iterations);
            if (err) return err;
        }
        return 0;
    }
}

/**
 * Close the given endpoint.
 * Currently closing the endpoint with UCP_EP_CLOSE_MODE_FORCE since we currently
 * cannot rely on the client side to be present during the server's endpoint
 * closing process.
 */
static void ep_close(ucp_worker_h ucp_worker, ucp_ep_h ep)
{
    ucs_status_t status;
    void *close_req;

    close_req = ucp_ep_close_nb(ep, UCP_EP_CLOSE_MODE_FORCE);
    if (UCS_PTR_IS_PTR(close_req)) {
        do {
            ucp_worker_progress(ucp_worker);
            status = ucp_request_check_status(close_req);
        } while (status == UCS_INPROGRESS);

        ucp_request_free(close_req);
    } else if (UCS_PTR_STATUS(close_req) != UCS_OK) {
        fprintf(stderr, "failed to close ep %p\n", (void*)ep);
    }
}

/**
 * A callback to be invoked by UCX in order to initialize the user's request.
 */
static void request_init(void *request)
{
    test_req_t *req = request;
    req->complete = 0;
}

/**
 * Print this application's usage help message.
 */
static void usage()
{
    fprintf(stderr, "Usage: ucp_client_server [parameters]\n");
    fprintf(stderr, "UCP client-server example utility\n");
    fprintf(stderr, "\nParameters are:\n");
    fprintf(stderr, " -a Set IP address of the server "
                    "(required for client and should not be specified "
                    "for the server)\n");
    fprintf(stderr, " -l Set IP address where server listens "
                    "(If not specified, server uses INADDR_ANY; "
                    "Irrelevant at client)\n");
    fprintf(stderr, " -p Port number to listen/connect to (default = %d). "
                    "0 on the server side means select a random port and print it\n",
                    DEFAULT_PORT);
    fprintf(stderr, " -c Communication type for the client and server. "
                    " Valid values are:\n"
                    "     'stream' : Stream API\n"
                    "     'tag'    : Tag API\n"
                    "    If not specified, %s API will be used.\n", COMM_TYPE_DEFAULT);
    fprintf(stderr, " -i Number of iterations to run. Client and server must "
                    "have the same value. (default = %d).\n",
                    num_iterations);
    fprintf(stderr, "\n");
}

/**
 * Parse the command line arguments.
 */
static int parse_cmd(int argc, char *const argv[], char **server_addr,
                     char **listen_addr, send_recv_type_t *send_recv_type)
{
    int c = 0;
    int port;

    opterr = 0;

    while ((c = getopt(argc, argv, "a:")) != -1) {
        switch (c) {
        case 'a':
            *server_addr = optarg;
            break;
        default:
            usage();
            return -1;
        }
    }

    return 0;
}

static char* sockaddr_get_ip_str(const struct sockaddr_storage *sock_addr,
                                 char *ip_str, size_t max_size)
{
    struct sockaddr_in  addr_in;
    struct sockaddr_in6 addr_in6;

    switch (sock_addr->ss_family) {
    case AF_INET:
        memcpy(&addr_in, sock_addr, sizeof(struct sockaddr_in));
        inet_ntop(AF_INET, &addr_in.sin_addr, ip_str, max_size);
        return ip_str;
    case AF_INET6:
        memcpy(&addr_in6, sock_addr, sizeof(struct sockaddr_in6));
        inet_ntop(AF_INET6, &addr_in6.sin6_addr, ip_str, max_size);
        return ip_str;
    default:
        return "Invalid address family";
    }
}

static char* sockaddr_get_port_str(const struct sockaddr_storage *sock_addr,
                                   char *port_str, size_t max_size)
{
    struct sockaddr_in  addr_in;
    struct sockaddr_in6 addr_in6;

    switch (sock_addr->ss_family) {
    case AF_INET:
        memcpy(&addr_in, sock_addr, sizeof(struct sockaddr_in));
        snprintf(port_str, max_size, "%d", ntohs(addr_in.sin_port));
        return port_str;
    case AF_INET6:
        memcpy(&addr_in6, sock_addr, sizeof(struct sockaddr_in6));
        snprintf(port_str, max_size, "%d", ntohs(addr_in6.sin6_port));
        return port_str;
    default:
        return "Invalid address family";
    }
}

static int client_server_communication(ucp_worker_h worker, ucp_ep_h ep,
                                       send_recv_type_t send_recv_type,
                                       int is_server, int iterations)
{
    int ret;

    /* Client-Server communication via Stream API */
    ret = send_recv_stream(worker, ep, is_server, iterations);

    return ret;
}

/**
 * Create a ucp worker on the given ucp context.
 */
static int init_worker(ucp_context_h ucp_context, ucp_worker_h *ucp_worker)
{
    ucp_worker_params_t worker_params;
    ucs_status_t status;
    int ret = 0;

    memset(&worker_params, 0, sizeof(worker_params));

    worker_params.field_mask  = UCP_WORKER_PARAM_FIELD_THREAD_MODE;
    worker_params.thread_mode = UCS_THREAD_MODE_SINGLE;

    status = ucp_worker_create(ucp_context, &worker_params, ucp_worker);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to ucp_worker_create (%s)\n", ucs_status_string(status));
        ret = -1;
    }

    return ret;
}

/**
 * The callback on the server side which is invoked upon receiving a connection
 * request from the client.
 */
static void server_conn_handle_cb(ucp_conn_request_h conn_request, void *arg)
{
    ucx_server_ctx_t *context = arg;
    ucs_status_t status;

    if (context->conn_request == NULL) {
        context->conn_request = conn_request;
    } else {
        /* The server is already handling a connection request from a client,
         * reject this new one */
        printf("Rejecting a connection request. "
               "Only one client at a time is supported.\n");
        status = ucp_listener_reject(context->listener, conn_request);
        if (status != UCS_OK) {
            fprintf(stderr, "server failed to reject a connection request: (%s)\n",
                    ucs_status_string(status));
        }
    }
}

static ucs_status_t server_create_ep(ucp_worker_h data_worker,
                                     ucp_conn_request_h conn_request,
                                     ucp_ep_h *server_ep)
{
    ucp_ep_params_t ep_params;
    ucs_status_t    status;

    /* Server creates an ep to the client on the data worker.
     * This is not the worker the listener was created on.
     * The client side should have initiated the connection, leading
     * to this ep's creation */
    ep_params.field_mask      = UCP_EP_PARAM_FIELD_ERR_HANDLER |
                                UCP_EP_PARAM_FIELD_CONN_REQUEST;
    ep_params.conn_request    = conn_request;
    ep_params.err_handler.cb  = err_cb;
    ep_params.err_handler.arg = NULL;

    status = ucp_ep_create(data_worker, &ep_params, server_ep);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to create an endpoint on the server: (%s)\n",
                ucs_status_string(status));
    }

    return status;
}

/**
 * Initialize the server side. The server starts listening on the set address.
 */
static ucs_status_t start_server(ucp_worker_h ucp_worker,
                                 ucx_server_ctx_t *context,
                                 ucp_listener_h *listener_p, const char *ip)
{
    struct sockaddr_in listen_addr;
    ucp_listener_params_t params;
    ucp_listener_attr_t attr;
    ucs_status_t status;
    char ip_str[IP_STRING_LEN];
    char port_str[PORT_STRING_LEN];

    set_listen_addr(ip, &listen_addr);

    params.field_mask         = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR |
                                UCP_LISTENER_PARAM_FIELD_CONN_HANDLER;
    params.sockaddr.addr      = (const struct sockaddr*)&listen_addr;
    params.sockaddr.addrlen   = sizeof(listen_addr);
    params.conn_handler.cb    = server_conn_handle_cb;
    params.conn_handler.arg   = context;

    /* Create a listener on the server side to listen on the given address.*/
    status = ucp_listener_create(ucp_worker, &params, listener_p);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to listen (%s)\n", ucs_status_string(status));
        goto out;
    }

    /* Query the created listener to get the port it is listening on. */
    attr.field_mask = UCP_LISTENER_ATTR_FIELD_SOCKADDR;
    status = ucp_listener_query(*listener_p, &attr);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to query the listener (%s)\n",
                ucs_status_string(status));
        ucp_listener_destroy(*listener_p);
        goto out;
    }

    fprintf(stderr, "server is listening on IP %s port %s\n",
            sockaddr_get_ip_str(&attr.sockaddr, ip_str, IP_STRING_LEN),
            sockaddr_get_port_str(&attr.sockaddr, port_str, PORT_STRING_LEN));

    printf("Waiting for connection...\n");

out:
    return status;
}

static int client_server_do_work(ucp_worker_h ucp_worker, ucp_ep_h ep,
                                 send_recv_type_t send_recv_type, int is_server, int iterations)
{
    int ret = client_server_communication(ucp_worker, ep, send_recv_type,
                                            is_server, iterations);
    if (ret != 0) {
        fprintf(stderr, "%s failed on iteration #%d\n",
                (is_server ? "server": "client"), iterations);
        goto out;
    }

out:
    return ret;
}


static inline int ucp_listen(ucp_context_h ucp_context, ucp_worker_h ucp_worker, char *listen_addr, ucp_worker_h* ucp_data_worker, ucx_server_ctx_t *context, ucp_ep_h *server_ep)
{
    ucs_status_t     status;
    int              ret;

    /* Create a data worker (to be used for data exchange between the server
     * and the client after the connection between them was established) */
    ret = init_worker(ucp_context, ucp_data_worker);
    if (ret != 0) {
        return ret; 
    }

    /* Initialize the server's context. */
    context->conn_request = NULL;

    /* Create a listener on the worker created at first. The 'connection
     * worker' - used for connection establishment between client and server.
     * This listener will stay open for listening to incoming connection
     * requests from the client */
    status = start_server(ucp_worker, context, &context->listener, listen_addr);
    if (status != UCS_OK) {
        ret = -1;
    } 
    return ret; 
}

static inline int ucp_accept(ucp_worker_h ucp_data_worker, ucp_worker_h ucp_worker, ucp_ep_h *server_ep, ucx_server_ctx_t *context)
{
    int ret, status;
    while (context->conn_request == NULL) {
        ucp_worker_progress(ucp_worker);
    }

    /* Server creates an ep to the client on the data worker.
    * This is not the worker the listener was created on.
    * The client side should have initiated the connection, leading
    * to this ep's creation */
    status = server_create_ep(ucp_data_worker, context->conn_request,
                            server_ep);
    if (status != UCS_OK) {
        ret = -1;
    }

    return ret;
}

static int run_server(ucp_context_h ucp_context, ucp_worker_h ucp_worker,
                      char *listen_addr, send_recv_type_t send_recv_type)
{
    ucx_server_ctx_t context;
    ucp_worker_h     ucp_data_worker;
    ucp_ep_h         server_ep;
    ucs_status_t     status;
    int              ret;

    /* Create a data worker (to be used for data exchange between the server
     * and the client after the connection between them was established) */
    ret = init_worker(ucp_context, &ucp_data_worker);
    if (ret != 0) {
        goto err;
    }

    /* Initialize the server's context. */
    context.conn_request = NULL;

    /* Create a listener on the worker created at first. The 'connection
     * worker' - used for connection establishment between client and server.
     * This listener will stay open for listening to incoming connection
     * requests from the client */
    status = start_server(ucp_worker, &context, &context.listener, listen_addr);
    if (status != UCS_OK) {
        ret = -1;
        goto err_worker;
    }            
    /* Wait for the server to receive a connection request from the client.
    * If there are multiple clients for which the server's connection request
    * callback is invoked, i.e. several clients are trying to connect in
    * parallel, the server will handle only the first one and reject the rest */
    while (context.conn_request == NULL) {
        ucp_worker_progress(ucp_worker);
    }

    /* Server creates an ep to the client on the data worker.
    * This is not the worker the listener was created on.
    * The client side should have initiated the connection, leading
    * to this ep's creation */
    status = server_create_ep(ucp_data_worker, context.conn_request,
                            &server_ep);
    if (status != UCS_OK) {
        ret = -1;
        goto err_listener;
    }

    ret = 0;
    EVERY_SEND_LEN = 0;

    printf("ALL SIZE: %d\n", TEST_BUFFER_LEN);
    for (EVERY_SEND_LEN = 1 << EVERY_SEND_LEN_BEGIN; EVERY_SEND_LEN <= 1 << EVERY_SEND_LEN_END; EVERY_SEND_LEN <<= 1){
        printf("EVERY SIZE: %d\n", EVERY_SEND_LEN);
        ret = client_server_do_work(ucp_data_worker, server_ep, send_recv_type, 1, num_iterations);
        if (ret != 0) {
            goto err_ep;
        }
    }           

    ep_close(ucp_data_worker, server_ep);

    /* Reinitialize the server's context to be used for the next client */
    context.conn_request = NULL;
    exit(0);

err_ep:
    ep_close(ucp_data_worker, server_ep);
err_listener:
    ucp_listener_destroy(context.listener);
err_worker:
    ucp_worker_destroy(ucp_data_worker);
err:
    return ret;
}

static inline int ucp_connect(ucp_worker_h ucp_worker, char *server_addr, ucp_ep_h *client_ep)
{
    ucs_status_t status;
    int          ret;

    printf("server addr %s\n", server_addr);
    status = start_client(ucp_worker, server_addr, client_ep);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to start client (%s)\n", ucs_status_string(status));
        ret = -1;
    }
    return ret;
}

static int run_client(ucp_worker_h ucp_worker, char *server_addr,
        send_recv_type_t send_recv_type)
{
    ucp_ep_h     client_ep;
    ucs_status_t status;
    int          ret;

    printf("server addr %s\n", server_addr);
    status = start_client(ucp_worker, server_addr, &client_ep);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to start client (%s)\n", ucs_status_string(status));
        ret = -1;
        goto out;
    }

    ret = 0;
    /*EVERY_SEND_LEN = 0;

      printf("ALL SIZE: %d\n", TEST_BUFFER_LEN);
      for (EVERY_SEND_LEN = 1 << EVERY_SEND_LEN_BEGIN; EVERY_SEND_LEN <= 1 << EVERY_SEND_LEN_END; EVERY_SEND_LEN <<= 1){
      printf("%d ", EVERY_SEND_LEN);
      ret = client_server_do_work(ucp_worker, client_ep, send_recv_type, 0, num_iterations);
      printf("\n");
      }*/

    /* Close the endpoint to the server */
    ep_close(ucp_worker, client_ep);

out:
    return ret;
}

/**
 * Initialize the UCP context and worker.
 */
static int init_context(ucp_context_h *ucp_context, ucp_worker_h *ucp_worker)
{
    /* UCP objects */
    ucp_params_t ucp_params;
    ucs_status_t status;
    int ret = 0;

    memset(&ucp_params, 0, sizeof(ucp_params));

    /* UCP initialization */
    ucp_params.field_mask   = UCP_PARAM_FIELD_FEATURES     |
        UCP_PARAM_FIELD_REQUEST_SIZE |
        UCP_PARAM_FIELD_REQUEST_INIT;
    ucp_params.features     = UCP_FEATURE_STREAM | UCP_FEATURE_TAG;
    ucp_params.request_size = sizeof(test_req_t);
    ucp_params.request_init = request_init;

    status = ucp_init(&ucp_params, NULL, ucp_context);
    if (status != UCS_OK) {
        fprintf(stderr, "failed to ucp_init (%s)\n", ucs_status_string(status));
        ret = -1;
        goto err;
    }

    ret = init_worker(*ucp_context, ucp_worker);
    if (ret != 0) {
        goto err_cleanup;
    }

    return ret;

err_cleanup:
    ucp_cleanup(*ucp_context);
err:
    return ret;
}


