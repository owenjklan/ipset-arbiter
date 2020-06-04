#ifndef __ARBITERD_H__
#define __ARBITERD_H__

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

/* Data structure definitions */
struct cli_args {
	uint16_t  port;
	char *bindAddrStr;
	struct in_addr bindAddr;
};
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
};
                                             
extern int cpu_count();  // utility.c

extern void client_main(void *args);

#endif  // __ARBITERD_H__
