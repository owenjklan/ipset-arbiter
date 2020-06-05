#ifndef __ARBITERD_H__
#define __ARBITERD_H__

// #define _BSD_SOURCE  // enabled by default
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
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

#define PROG_NAME   "arbiterd"

#define DEFAULT_PORT_NUM	     65432
#define DEFAULT_BIND_ADDR_STR    "127.0.0.1"

#define REQ_TEST_SET    0
#define REQ_ADD_SET     1
#define REQ_LIST_SET    2


/* Data structure definitions */

// For handling command line arguments and their defaults.
struct cli_args {
	uint16_t  port;
	char *bindAddrStr;
	struct in_addr bindAddr;
};

// Thread pair information
struct thread_pair {
    pthread_t  client_thread; // Handles client interactions
    pthread_t  ipset_thread;  // Interacts with ipset
    struct ipset *ipset_handle;  // unique per client thread pair
    int cpuAffinity;  // What CPU have we set affinity for?
    int pairNumber;
    GAsyncQueue *request_queue;  // client -> ipset
    GAsyncQueue *response_queue; // ipset -> client
};

// Arguments passed to each client thread
struct client_thread_args {
    GAsyncQueue *request_queue;
    GAsyncQueue *response_queue;
    uint16_t port;
    struct in_addr *bindAddr;
    int pairNumber;
};

struct ipset_thread_args {
    GAsyncQueue *request_queue;
    GAsyncQueue *response_queue;
    struct ipset *ipset_handle;
    int pairNumber;
};


struct request {
    uint8_t  request_type;
    uint8_t  setname_length;
    uint32_t request_id;
    char *setname;
};

struct response {
    uint8_t response_type;
    uint8_t padd;
    uint16_t response_size;
    uint32_t request_id;
    char *response;
};

// utility.c
extern void set_thread_name(char *prefix, int num);
extern int cpu_count();
extern struct thread_pair *create_thread_pair(
    int number, int cpuAffinity, uint16_t port, struct in_addr *bindAddr);

extern void client_main(void *args);
extern void ipset_main(void *args);

#endif  // __ARBITERD_H__
