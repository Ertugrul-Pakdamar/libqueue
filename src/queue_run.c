#include "libqueue.h"

void    queue_run_sync(t_queue *queue)
{
    t_node  *current;
    t_node  *next;
    int32_t     result;
    int32_t     effective;

    if (!queue || !queue->head)
        return ;
    current = queue->head;
    queue->head = NULL;
    while (current)
    {
        next = current->next;
        result = node_run(queue, current);
        if (result != 0)
        {
            effective = (current->max_retries >= 0)
                            ? current->max_retries
                            : queue->max_retries;
            if (queue->policy == POLICY_RETRY
                && current->retry_count < effective)
            {
                current->retry_count++;
                continue ;
            }
            if (queue->on_error)
                queue->on_error(current, result);
            if (queue->policy == POLICY_STOP)
            {
                node_destroy(queue, current);
                queue->head = next;
                return ;
            }
        }
        node_destroy(queue, current);
        current = next;
    }
}
