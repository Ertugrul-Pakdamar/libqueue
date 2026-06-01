/*
** MISRA C:2012 Rule 21.3 deviation:
**   malloc() is called once in queue_init() to allocate the node pool backing
**   buffer, and free() is called once in queue_destroy() to release it.
**   No dynamic allocation occurs on any code path after initialisation.
**   Rationale: a fixed-size pool backed by a single malloc block is more
**   deterministic than repeated individual allocations; all subsequent node
**   allocations are served by the pool (O(1), no system call).
*/
#include "libqueue.h"
#include <stdlib.h>

int32_t     queue_init(t_queue *queue, size_t capacity, const t_queue_config *config)
{
    void    *buffer;
    size_t  i;

    buffer = malloc(sizeof(t_node) * capacity);
    if (!buffer)
        return (0);
    pool_init(&queue->node_pool, buffer, sizeof(t_node), capacity);
    osal_mutex_init(&queue->pool_lock);
    queue->head = NULL;
    if (config)
    {
        queue->policy = config->policy;
        queue->max_retries = (config->max_retries > 0) ? config->max_retries : 3;
        queue->on_error = config->on_error;
    }
    else
    {
        queue->policy = POLICY_CONTINUE;
        queue->max_retries = 3;
        queue->on_error = NULL;
    }
    for (i = 0; i < EVENT_TYPE_MAX; ++i)
        queue->handlers[i] = NULL;
    return (1);
}

void    queue_destroy(t_queue *queue)
{
    queue_clear(queue);
    osal_mutex_destroy(&queue->pool_lock);
    free(queue->node_pool.start_addr);
    queue->node_pool.start_addr = NULL;
}

int32_t     queue_register_handler(t_queue *queue, t_event_type type, t_event_handler handler)
{
    if (!queue || type <= EVENT_TYPE_NONE || type >= EVENT_TYPE_MAX)
        return (0);
    queue->handlers[type] = handler;
    return (1);
}
