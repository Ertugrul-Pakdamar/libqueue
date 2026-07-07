/**
 * @example 04_bounded_execution.c
 * @brief Demonstrates deterministic, bounded execution loops.
 *
 * This example shows how to use the 'max_events' parameter to prevent
 * the dispatcher from hogging the CPU if many events are pending.
 */

#include "libqueue.h"
#include <stdio.h>

#define EVENT_WORK     4
#define POOL_CAPACITY  32

static LQ_STATIC_NODES(node_buffer, POOL_CAPACITY);
static mem_pool_t      pool;
static LQ_STATIC_QUEUE(queue_mem);
static LQ_STATIC_DISPATCHER(disp_mem);

#define event_queue ((lq_queue_t*)queue_mem)
#define dispatcher  ((lq_dispatcher_t*)disp_mem)

int32_t work_handler(lq_node_t *node)
{
    printf("Processing: %s\n", lq_node_get_name(node));
    return 0;
}

int main(void)
{
    pool_init(&pool, node_buffer, LQ_NODE_SIZE, POOL_CAPACITY);
    lq_queue_init(event_queue);
    
    lq_dispatcher_init(dispatcher, NULL);
    lq_dispatcher_register(dispatcher, EVENT_WORK, work_handler);

    /* 1. Flood the queue with many events */
    printf("Main: Pushing 10 events...\n");
    for (int i = 0; i < 10; ++i)
    {
        char name[16];
        sprintf(name, "task-%d", i);
        lq_node_config_t ncfg = {.name = name, .event_type = EVENT_WORK};
        lq_node_t *node = lq_node_new(&pool, &ncfg);
        if (node) lq_queue_push(event_queue, node);
    }

    /* 2. Process in bounded "ticks" */
    printf("\nMain: Starting Bounded Execution (3 events per tick)\n");
    
    int tick = 1;
    while (!lq_queue_is_empty(event_queue))
    {
        printf("--- Tick %d ---\n", tick++);
        /* Only process 3 events at a time */
        lq_dispatcher_run_queue(dispatcher, event_queue, &pool, 3);
        printf("Main Loop: Doing other critical system work...\n");
    }

    printf("\nMain Loop: All events processed deterministically.\n");
    return 0;
}
