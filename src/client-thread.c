
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

#include "arbiterd.h"

void client_main(void *args)
{
    struct client_thread_args *threadArgs = (struct client_thread_args *)args;
    struct response *resp;
    struct request req = {
        .request_type = REQ_LIST_SET,
        .setname_length = 7,
        .request_id = 0xDEADBEEF,
        .setname = "testset"
    };

    set_thread_name("client", threadArgs->pairNumber);

    srandom(time(NULL));
    req.request_id = random();

    while (1) {
        sleep(3);
        g_async_queue_push(threadArgs->request_queue, (gpointer)&req);

        resp = (struct response *)g_async_queue_pop(threadArgs->response_queue);
        printf("client %02d] ID %08X : response received\n",
            threadArgs->pairNumber, resp->request_id);
        req.request_id++;
    }

    return;
}
