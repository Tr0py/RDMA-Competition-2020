#include "ucx.h"

int main(int argc, char **argv)
{
    send_recv_type_t send_recv_type = CLIENT_SERVER_SEND_RECV_DEFAULT;
    char *server_addr = NULL;
    char *listen_addr = NULL;
    int ret;
    srand(time(NULL));

    /* UCP objects */
    ucp_context_h ucp_context;
    ucp_worker_h ucp_worker;

    if (ret != 0)
    {
        goto err;
    }

    init_buffer();

    /* Initialize the UCX required objects */
    ret = init_context(&ucp_context, &ucp_worker);
    if (ret != 0)
    {
        goto err;
    }

    /* Client-Server initialization */
    if (argc == 1)
    {
        /* Server side */
        //ret = run_server(ucp_context, ucp_worker, listen_addr, send_recv_type);
        ucp_worker_h ucp_data_worker;
        ucx_server_ctx_t context;
        ucp_ep_h server_ep;

        ret = ucp_listen(ucp_context, ucp_worker, listen_addr, &ucp_data_worker, &context, &server_ep);
        ucp_accept(ucp_data_worker, ucp_worker, &server_ep, &context);

        EVERY_SEND_LEN = 0;

        printf("ALL SIZE: %d\n", TEST_BUFFER_LEN);
        for (EVERY_SEND_LEN = 1 << EVERY_SEND_LEN_BEGIN; EVERY_SEND_LEN <= 1 << EVERY_SEND_LEN_END; EVERY_SEND_LEN <<= 1){
            printf("EVERY SIZE: %d\n", EVERY_SEND_LEN);
            ret = client_server_do_work(ucp_data_worker, server_ep, send_recv_type, 1, num_iterations);
            if (ret != 0) {
                printf("error transport\n");
            }
        }           
        
        ep_close(ucp_worker, server_ep);
    }
    else
    {
        /* Client side */
        //ret = run_client(ucp_worker, server_addr, send_recv_type);
        ucp_ep_h client_ep;
        ret = ucp_connect(ucp_worker, argv[1], &client_ep);
        EVERY_SEND_LEN = 0;

        printf("ALL SIZE: %d\n", TEST_BUFFER_LEN);
        for (EVERY_SEND_LEN = 1 << EVERY_SEND_LEN_BEGIN; EVERY_SEND_LEN <= 1 << EVERY_SEND_LEN_END; EVERY_SEND_LEN <<= 1){
            printf("%d ", EVERY_SEND_LEN);
            ret = client_server_do_work(ucp_worker, client_ep, send_recv_type, 0, num_iterations);
            printf("\n");
        }
        ep_close(ucp_worker, client_ep);
    }

    ucp_worker_destroy(ucp_worker);
    ucp_cleanup(ucp_context);
    free_buffer();
err:
    return ret;
}
