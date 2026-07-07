/**
 * @example 03_fail_policy.c
 * @brief Demonstrates failure handling and retry policies.
 *
 * This example shows how the dispatcher reacts when a handler returns an error,
 * specifically using the LQ_POLICY_RETRY policy.
 */

#include "libqueue.h"
#include <stdio.h>

#define EVENT_CRITICAL 3
#define POOL_CAPACITY  16

static LQ_STATIC_NODES(node_buffer, POOL_CAPACITY);
static mem_pool_t      pool;
static LQ_STATIC_QUEUE(queue_mem);
static LQ_STATIC_DISPATCHER(disp_mem);

#define event_queue ((lq_queue_t*)queue_mem)
#define dispatcher  ((lq_dispatcher_t*)disp_mem)

/* A handler that fails twice and succeeds on the third attempt */
int32_t flaky_handler(lq_node_t *node)
{
    static int attempts = 0;
    attempts++;

    printf("Handler: Attempt %d for node '%s'...\n", attempts, lq_node_get_name(node));
    
    if (attempts < 3)
    {
        printf("Handler: Failed! (Will retry if policy allows)\n");
        return -1; /* Error code triggers retry */
    }

    printf("Handler: Success on attempt 3!\n");
    return 0;
}

void error_callback(lq_node_t *node, int32_t result)
{
    printf("[ALERT]: Node '%s' failed permanently with code %d\n", lq_node_get_name(node), result);
}

int main(void)
{
    pool_init(&pool, node_buffer, LQ_NODE_SIZE, POOL_CAPACITY);
    lq_queue_init(event_queue);
    
    /* 1. Configure dispatcher for RETRY */
    const lq_dispatcher_config_t cfg = {
        .policy = LQ_POLICY_RETRY,
        .max_retries = 5,
        .on_error = error_callback
    };
    lq_dispatcher_init(dispatcher, &cfg);
    lq_dispatcher_register(dispatcher, EVENT_CRITICAL, flaky_handler);

    /* 2. Push a node */
    lq_node_config_t ncfg = {
        .name = "Critical-Task",
        .event_type = EVENT_CRITICAL,
        .max_retries = -1 /* Use dispatcher default */
    };
    lq_node_t *node = lq_node_new(&pool, &ncfg);
    if (node) lq_queue_push(event_queue, node);

    /* 3. Run */
    printf("Main: Starting execution with RETRY policy.\n");
    lq_dispatcher_run_queue(dispatcher, event_queue, &pool, 0);

    return 0;
}
