#include <sys/sysinfo.h>

#include <pthread.h>

#include "arbiterd.h"

int cpu_count()
{
    return get_nprocs();
}

int custom_output_func(struct ipset_session *session,
    void *p, const char *fmt, ...)
{
    char buffer[2048];
    va_list args;
    GAsyncQueue *response_queue = (GAsyncQueue *)p;

    va_start(args, fmt);

    vsnprintf(buffer, 2048, fmt, args);
    va_end(args);

    char *memberStart = strstr(buffer, "Members:\n");
    int memberLen = strlen("Members:\n");

    if (memberStart == NULL) {
        return strlen(buffer);
    } else {
        char *start = memberStart + memberLen;
        int bufferLen = strlen(buffer);
        char *end = buffer + bufferLen;

        printf("BUFFER:\n%s", memberStart + memberLen);
    }

    return strlen(buffer);
}

struct thread_pair *create_thread_pair(
    int number, int cpuAffinity, uint16_t port, struct in_addr *bindAddr)
{
    struct ipset_thread_args *ipset_args;
    struct client_thread_args *client_args;

    struct thread_pair *tpair = (struct thread_pair *)malloc(
        sizeof(struct thread_pair));

    if (tpair == NULL) {
        return NULL;
    }

    // Create queues
    tpair->request_queue = g_async_queue_new();
    tpair->response_queue = g_async_queue_new();
    tpair->pairNumber = number;
    tpair->cpuAffinity = cpuAffinity;

    // Create libipset session handle
    tpair->ipset_handle = ipset_init();
    ipset_custom_printf(tpair->ipset_handle, NULL, NULL, custom_output_func, NULL);

    // Create argument structures for threads
    // ipset_thread
    ipset_args = (struct ipset_thread_args *)malloc(
        sizeof(struct ipset_thread_args));

    if (ipset_args == NULL) {
        fprintf(stderr, "Failed allocating ipset memory\n" );
    }
    ipset_args->request_queue  = tpair->request_queue;
    ipset_args->response_queue = tpair->response_queue;
    ipset_args->ipset_handle   = tpair->ipset_handle;
    ipset_args->pairNumber     = number;

    client_args = (struct client_thread_args *)malloc(
        sizeof(struct client_thread_args));
    if (client_args == NULL) {
        fprintf(stderr, "Failed allocating client memory\n" );
    }
    client_args->request_queue  = tpair->request_queue;
    client_args->response_queue = tpair->response_queue;
    client_args->pairNumber     = number;
    client_args->port           = port;
    client_args->bindAddr       = bindAddr;

    if (pthread_create(&tpair->ipset_thread, NULL, (void *)ipset_main, (void *)ipset_args) != 0)
    {
        printf("Failed creating ipset_thread!\n");
        exit(1);
    }

    if (pthread_create(&tpair->client_thread, NULL, (void *)client_main, (void *)client_args) != 0)
    {
        printf("Failed creating client_thread!\n");
        exit(1);
    }
    return tpair;
}


void set_thread_name(char *prefix, int num)
{
#define MAX_THREAD_NAME  15
    char threadName[MAX_THREAD_NAME + 1];
    memset(threadName, 0x00, MAX_THREAD_NAME + 1);
    snprintf(threadName, MAX_THREAD_NAME, "%s-%02d", prefix, num);      
    pthread_t threadId = pthread_self();
    pthread_setname_np(threadId, threadName);
}