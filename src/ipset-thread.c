
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include "arbiterd.h"

void ipset_main(void *args)
{
    struct ipset_thread_args *threadArgs = (struct ipset_thread_args *)args;
    struct request *req;
    char ipsetCmdBuff[2048];

    cpu_set_t cpuMask;

    CPU_ZERO(&cpuMask);
    CPU_SET(threadArgs->pairNumber, &cpuMask);     
    
    // This isn't considered fatal.
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuMask) < 0)
    {
        syslog(LOG_ERR, "Failed setting affinity! %s\n", strerror(errno));
    }

    set_thread_name("ipset", threadArgs->pairNumber);

    struct response resp = {
        .response_type = 1,
        .padd = 0,
        .response_size = 128
    };
    struct response *buffResp = NULL;

    while (1) {
        req = (struct request *)g_async_queue_pop(threadArgs->request_queue);
        resp.request_id = req->request_id;
        char *setname;
        char *requestVerb;
        memset(ipsetCmdBuff, 0x00, 2048);

        // We need to send a dynamic pointer "across the queue", so that
        // the reference can be free'd
        buffResp = malloc(sizeof(struct response));
        memcpy(buffResp, &resp, sizeof(struct response));

        if ((setname = malloc(req->setname_length + 1)) == NULL) {
            syslog(LOG_ERR, "Failed copying setname! Malloc fail\n");
            if (req) { free(req); }
            continue;
        }
        memset(setname, 0x00, req->setname_length + 1);
        memcpy(setname, req->setname, req->setname_length);
        syslog(LOG_INFO, " ipset-%02d ] ID %08X: Request received. setname: %s\n",
            threadArgs->pairNumber, req->request_id, setname);

        switch (req->request_type) {
            case REQ_TEST_SET:
                requestVerb = "test";
                break;
            case REQ_ADD_SET:
                requestVerb = "add";
                break;
            case REQ_LIST_SET:
            {
                requestVerb = "list";
                snprintf(ipsetCmdBuff, 2048, "-! %s %s", requestVerb, setname);
                if (ipset_parse_line(threadArgs->ipset_handle, ipsetCmdBuff) < 0) {
                    syslog(LOG_ERR, "Command fail: %s\n", ipsetCmdBuff);
                    break;
                }
                break;
            }
            default:
                syslog(LOG_INFO, "Unknown request code: 0x%02X\n", req->request_type);
        }

        g_async_queue_push(threadArgs->response_queue, (gpointer)buffResp);

        free(setname);
        if (req) { free(req); }
    }
    // if (buffResp) { free(buffResp); }

    return;
}
