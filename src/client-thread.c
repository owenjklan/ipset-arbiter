
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <errno.h>

#include <syslog.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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

    // Setup CPU affinity
    cpu_set_t cpuMask;
    CPU_ZERO(&cpuMask);
    CPU_SET(threadArgs->pairNumber, &cpuMask);     
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuMask) < 0)
    {
        // This is non-fatal. Perhaps make it being fatal
        // a configurable option?
        syslog(LOG_ERR, "Failed setting affinity! %s\n", strerror(errno));
    }

    set_thread_name("client", threadArgs->pairNumber);

    srandom(time(NULL) + threadArgs->pairNumber);
    req.request_id = random();

    // Setup server socket
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct request *buffReq = NULL;
    while (1) {
        // We need to send a dynamic pointer "across the queue", so that
        // the reference can be free'd
        buffReq = malloc(sizeof(struct request));
        memcpy(buffReq, &req, sizeof(struct request));

        syslog(LOG_INFO, "client-%02d ] ID %08X : Request sent\n",
            threadArgs->pairNumber, buffReq->request_id);
        g_async_queue_push(threadArgs->request_queue, (gpointer)buffReq);

        resp = (struct response *)g_async_queue_pop(threadArgs->response_queue);
        syslog(LOG_INFO, "client-%02d ] ID %08X : Response received\n",
            threadArgs->pairNumber, resp->request_id);
        req.request_id++;

        if (resp) {
            syslog(LOG_INFO, "client-%02d ] ID %08X : Free'd memory for response");
            free(resp);
        }
    }

    free(buffReq);
    return;
}
