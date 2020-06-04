
#include <stdio.h>
#include <unistd.h>

#include "arbiterd.h"

void client_main(void *args)
{
    struct thread_args *threadArgs = (struct thread_args *)args;

    struct request req = {
        .request_type = 1,
        .setname_length = 7,
        .request_id = 0xDEADBEEF,
        .setname = "testset"
    };

    while (1) {
        sleep(5);
        g_async_queue_push(threadArgs->requestQueue, (gpointer)&req);
    }

    return;
}
