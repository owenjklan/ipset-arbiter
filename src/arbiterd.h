#ifndef __ARBITERD_H__
#define __ARBITERD_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PROG_NAME   "arbiterd"

#define DEFAULT_PORT_NUM	     65432
#define DEFAULT_BIND_ADDR_STR    "127.0.0.1"

/* Data structure definitions */
struct cli_args {
	uint16_t  port;
	char *bindAddrStr;
	struct in_addr bindAddr;
};

extern int cpu_count();  // utility.c

extern void client_main(void *args);

#endif  // __ARBITERD_H__