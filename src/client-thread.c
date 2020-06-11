
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
#include <netinet/in.h>

#include "arbiterd.h"

static int setup_socket(uint16_t port, struct in_addr *bindAddr);

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

    // openlog(PROG_NAME, LOG_NDELAY|LOG_PID, LOG_USER);

    // Setup CPU affinity
    if (threadArgs->enableCpuAffinity)
    {
        cpu_set_t cpuMask;
        CPU_ZERO(&cpuMask);
        CPU_SET(threadArgs->pairNumber, &cpuMask);     
        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuMask) < 0)
        {
            // This is non-fatal. Perhaps make it being fatal
            // a configurable option?
            syslog(LOG_ERR, "Failed setting affinity! %s\n", strerror(errno));
        } else {
            syslog(LOG_INFO, "CPU affinity successfully bound to CPU #%02d",
                threadArgs->pairNumber);
        }
    } else {
        syslog(LOG_INFO, "CPU affinity binding has been disabled!");
    }

    set_thread_name("client", threadArgs->pairNumber);

    srandom(time(NULL) + threadArgs->pairNumber);
    req.request_id = random();

    // Setup server socket
    int sock = setup_socket(threadArgs->port, threadArgs->bindAddr);

    struct request *buffReq = NULL;
    while (arbiterdRunning) {
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
            syslog(LOG_INFO, "client-%02d ] ID %08X : Free'd memory for response",
                threadArgs->pairNumber, resp->request_id);
            free(resp);
        }
    }

    close(sock);
    syslog(LOG_INFO, "client-%02d ] closed listening socket",
                threadArgs->pairNumber);
    if (buffReq) {
        free(buffReq);
    }
    return;
}

static int setup_socket(uint16_t port, struct in_addr *bindAddr)
{
    int sock = -1;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        syslog(LOG_INFO, "Failed setting up listening socket! %s",
            strerror(errno));
        exit(1);
    }

    int sockopt_val = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                    &sockopt_val, sizeof(sockopt_val)) < 0) {
        syslog(LOG_ERR, "Failed configuring port for listening socket! %s",
            strerror(errno));
        close(sock);
        exit(1);
    }

    struct sockaddr_in sockAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(port)
    };
    memcpy(&sockAddr.sin_addr.s_addr, bindAddr, sizeof(struct in_addr));

    if (bind(sock, &sockAddr, sizeof(struct sockaddr_in)) < 0) {
        syslog(LOG_ERR, "Failed binding listening socket! %s",
            strerror(errno));
        close(sock);
        exit(1);
    }

    syslog(LOG_INFO, "Successfully bound listening socket. Waiting for requests...");

    return sock;
}