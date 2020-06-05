#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <netinet/in.h>  // For IPPROTO_TCP

// #define _BSD_SOURCE
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
    int numCpus = cpu_count();
    struct thread_pair *threads[numCpus];
    struct cli_args cliArgs = {
        .port        = DEFAULT_PORT_NUM,
        .bindAddrStr = DEFAULT_BIND_ADDR_STR
    };
    parse_args(argc, argv, &cliArgs);

    syslog(LOG_INFO, "Will bind to address: %s:%d\n",
        cliArgs.bindAddrStr, cliArgs.port);

    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(PROG_NAME, LOG_NDELAY|LOG_PID, LOG_USER);

    ipset_load_types();
    syslog(LOG_INFO, "ipset types loaded\n");
    syslog(LOG_INFO, "%d CPUs reported as available\n", numCpus);

    // Start threads

    for (int threadIndex = 0; threadIndex < numCpus; threadIndex++) {
        struct thread_pair *pair;
        pair = create_thread_pair(threadIndex, threadIndex, cliArgs.port,
            &cliArgs.bindAddr);
        threads[threadIndex] = pair;
    }

    syslog(LOG_INFO, "All thread pairs created. Main thread awaiting child threads.");
    // Main thread awaiting on all child threads
    for (int threadIndex = 0; threadIndex < numCpus; threadIndex++)
    {
        struct thread_pair *pair = threads[threadIndex];
        pthread_join(pair->ipset_thread, NULL);
        pthread_join(pair->client_thread, NULL);
    }

    return 0;
}
