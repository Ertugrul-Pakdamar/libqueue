/**
 * @example 01_queue_mpsc.c
 * @brief Demonstrates a Multi-Producer Single-Consumer (MPSC) Lock-Free Queue.
 *
 * In this example, multiple tasks/ISRs (simulated) push events to a single
 * queue. The main loop (consumer) dispatches them.
 */

#include "libqueue.h"
#include <stdio.h>

#define EVENT_LOG      1
#define POOL_CAPACITY  32

/* 1. Static memory allocation */
static LQ_STATIC_NODES(node_buffer, POOL_CAPACITY);
static mem_pool_t      pool;
static LQ_STATIC_QUEUE(queue_mem);
static LQ_STATIC_DISPATCHER(disp_mem);

#define event_queue ((lq_queue_t*)queue_mem)
#define dispatcher  ((lq_dispatcher_t*)disp_mem)

/* 2. Handler for log events */
int32_t log_handler(lq_node_t *node)
{
    const char *msg = (const char *)lq_node_get_args(node);
    printf("[LOG]: %s\n", msg);
    return 0;
}

int main(void)
{
    /* 3. Initialization */
    pool_init(&pool, node_buffer, LQ_NODE_SIZE, POOL_CAPACITY);
    lq_queue_init(event_queue);
    
    lq_dispatcher_init(dispatcher, NULL);
    lq_dispatcher_register(dispatcher, EVENT_LOG, log_handler);

    /* 4. Simulating Multiple Producers (e.g. various ISRs or Tasks) */
    const char *messages[] = {"System starting", "Sensor A active", "Sensor B active", "Ready."};
    for (int i = 0; i < 4; ++i)
    {
        lq_node_config_t ncfg = {
            .name = "log-node",
            .event_type = EVENT_LOG,
            .args = (void *)messages[i]
        };
        
        lq_node_t *node = lq_node_new(&pool, &ncfg);
        if (node)
        {
            lq_queue_push(event_queue, node);
            printf("Producer: Pushed '%s'\n", messages[i]);
        }
    }

    /* 5. Execution Phase (Consumer) */
    printf("\nMain Loop: Processing Queue...\n");
    lq_dispatcher_run_queue(dispatcher, event_queue, &pool, 0);

    printf("Main Loop: Done.\n");
    return 0;
}
