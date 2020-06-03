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

// Arguments passed to each client thread
struct thread_args {
    GAsyncQueue *requestQueue;
    uint16_t port;
};

// Main thread's record for keeping track of started threads
struct thread_info {
    pthread_t   handle;  // As returned by pthread_create()
    int cpuAffinity;     // What CPU have we set affinity for?
};


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
    struct ipset_session *ipsSession;
    GAsyncQueue *requestQueue;
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
    if ((ipsetSession = ipset_session_init(NULL)) == NULL) {
        syslog(LOG_ERR, "Failed initialising ipsets!\n");
        exit(1);        
    }

    syslog(LOG_INFO, "ipset session initialised");
    syslog(LOG_INFO, "%d CPUs reported as available\n", cpu_count());
    ipset_fini(ipsetLib);

    return 0;
}