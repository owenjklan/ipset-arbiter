#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <netinet/in.h>  // For IPPROTO_TCP
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>

#include <libipset/ipset.h>
#include <libipset/data.h>
#include <libipset/mnl.h>
#include <libipset/parse.h>
#include <libipset/session.h>
#include <libipset/types.h>
#include <libipset/linux_ip_set.h>

#include <glib.h>

#include <pthread.h>

#include "arbiterd.h"

void usage(char *progname)
{
    fprintf(stderr,
        "USAGE:\n");
    fprintf(stderr,
        "  %s [-h] [-p port] [-b bind_address]\n\n",
        progname);
    fprintf(stderr,
        "Options:\n");
    fprintf(stderr,
        "  -h           Display usage information / help.\n\n");
    fprintf(stderr,
        "  -p port      Port number to listen on. Default: %d\n\n",
        DEFAULT_PORT_NUM);
    fprintf(stderr,
        "  -b bind_addr Specify specific IP address to listen on. Can specify '*' wildcard. Default: %s\n\n",
        DEFAULT_BIND_ADDR_STR);
}

void parse_args(int argc, char *argv[], struct cli_args *args)
{
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "hb:p:")) != EOF) {
        switch (c) {
            case 'b':  // Set bind address
            {
                if (strncmp(optarg, "*", 1) == 0) {
                    args->bindAddrStr = "0.0.0.0";
                } else {
                    args->bindAddrStr = optarg;
                }
                if (!inet_aton(args->bindAddrStr, &args->bindAddr)) {
                    fprintf(stderr,
                        "The supplied bind address is not valid IP address!\n");
                    exit(1);
                }
                break;
            }
            case 'p':  // set listening port
            {
                args->port = atoi(optarg);
                break;
            }
            case 'h':
                usage(argv[0]);
                exit(0);
            default:
                fprintf(stderr, "Unknown option! %c\n", c);
        }
    }
}

int main(int argc, char *argv[])
{
//    struct ipset_session *ipsSession;
    GAsyncQueue *requestQueue;
    int numCpus = cpu_count();
    struct thread_info threads[numCpus];
    struct cli_args cliArgs = {
        .port        = DEFAULT_PORT_NUM,
        .bindAddrStr = DEFAULT_BIND_ADDR_STR
    };
    parse_args(argc, argv, &cliArgs);

    syslog(LOG_INFO, "Will bind to address: %s:%d\n",
        cliArgs.bindAddrStr, cliArgs.port);

    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(PROG_NAME, LOG_NDELAY|LOG_PID, LOG_USER);

    if ((requestQueue = g_async_queue_new()) == NULL) {
        syslog(LOG_ERR, "Failed initialising request queue!\n");
        exit(1);
    }

    // setup ipset library and session
    struct ipset *ipsetLib = ipset_init();
    struct ipset_session *ipsetSession = NULL;

    ipset_load_types();
    if ((ipsetSession = ipset_session(ipsetLib)) == NULL) {
        syslog(LOG_ERR, "Failed initialising ipsets!\n");
        exit(1);        
    }

    syslog(LOG_INFO, "ipset session initialised");
    syslog(LOG_INFO, "%d CPUs reported as available\n", numCpus);

    struct thread_args threadArgs = {
        .requestQueue = requestQueue,
        .port = cliArgs.port
    };

    // Start threads
    for (int threadIndex = 0; threadIndex < numCpus; threadIndex++) {
        if (pthread_create(&threads[threadIndex].handle, NULL, &client_main,
                            (void *)&threadArgs) != 0) {
            syslog(LOG_ERR, "Failed starting client thread #%02d!\n",
                    threadIndex);
            exit(1); // CLEANUP BETTER
        }
    }

    // wait for requests
    struct request *req;
    char ipsetCmdBuff[2048];
    while (1) {
        req = (struct request *)g_async_queue_pop(requestQueue);
        char *setname;

        if ((setname = malloc(req->setname_length + 1)) == NULL) {
            fprintf(stderr, "Failed copying setname! Malloc fail\n");
            continue;
        }
        memset(setname, 0x00, req->setname_length + 1);
        memcpy(setname, req->setname, req->setname_length);

        fprintf(stderr, "Request using setname: %s\n", setname);

        free(setname);
    }
 

    ipset_fini(ipsetLib);

    return 0;
}
