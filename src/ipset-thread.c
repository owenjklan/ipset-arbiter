
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>

#include "arbiterd.h"

void ipset_main(void *args)
{
    struct ipset_thread_args *threadArgs = (struct ipset_thread_args *)args;
    struct request *req;
    char ipsetCmdBuff[2048];

    set_thread_name("ipset", threadArgs->pairNumber);


    struct response resp = {
        .response_type = 1,
        .padd = 0,
        .response_size = 128
    };

    while (1) {
        req = (struct request *)g_async_queue_pop(threadArgs->request_queue);
        resp.request_id = req->request_id;
        char *setname;
        char *requestVerb;
        memset(ipsetCmdBuff, 0x00, 2048);

        printf(" ipset] Request 0x%08X received.\n");

        if ((setname = malloc(req->setname_length + 1)) == NULL) {
            fprintf(stderr, "Failed copying setname! Malloc fail\n");
            continue;
        }
        memset(setname, 0x00, req->setname_length + 1);
        memcpy(setname, req->setname, req->setname_length);

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

        fprintf(stderr, " ipset] Request using setname: %s\n", setname);
        g_async_queue_push(threadArgs->response_queue, (gpointer)&resp);

        free(setname);
    }

    return;
}
