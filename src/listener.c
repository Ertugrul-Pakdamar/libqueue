#include "queue.h"

static void *worker(void *arg)
{
    t_listener  *l;
    t_node      *node;
    int          result;
    int          effective;

    l = (t_listener *)arg;
    while (osal_atomic_int_load(&l->running))
    {
        node = ring_pop(l->ring);
        if (!node)
            continue ;
        node->retry_count = 0;
        result = run_node(node);
        effective = (node->max_retries >= 0)
                    ? node->max_retries
                    : l->queue->max_retries;
        while (result != 0 && l->queue->policy == POLICY_RETRY
               && node->retry_count < effective)
        {
            node->retry_count++;
            result = run_node(node);
        }
        if (result != 0)
        {
            if (l->queue->on_error)
                l->queue->on_error(node, result);
            if (l->queue->policy == POLICY_STOP)
            {
                node_destroy(l->queue, node);
                osal_atomic_int_store(&l->running, 0);
                return (NULL);
            }
        }
        node_destroy(l->queue, node);
    }
    return (NULL);
}

int     listener_start(t_listener *listener, t_ring *ring, t_queue *queue)
{
    listener->ring  = ring;
    listener->queue = queue;
    osal_atomic_int_init(&listener->running, 1);
    if (osal_task_create(&listener->task, worker, listener) != 0)
    {
        osal_atomic_int_store(&listener->running, 0);
        return (0);
    }
    return (1);
}

void    listener_stop(t_listener *listener)
{
    osal_atomic_int_store(&listener->running, 0);
    osal_task_join(&listener->task);
}
