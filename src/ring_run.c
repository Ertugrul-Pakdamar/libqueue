#include "libqueue.h"

void    ring_run_sync(t_ring *ring, t_queue *queue)
{
    t_node  *current;
    int     result;
    int     effective;

    if (!ring || !queue)
        return ;

    while ((current = ring_pop(ring)) != NULL)
    {
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
                
                /* Since it's a ring buffer pop, we can't easily "continue" and leave it 
                   at the front like a queue. The item was already popped. 
                   We must execute it again synchronously here. */
                while (result != 0 && current->retry_count < effective)
                {
                    result = node_run(queue, current);
                    if (result != 0)
                        current->retry_count++;
                }
            }
            if (result != 0)
            {
                if (queue->on_error)
                    queue->on_error(current, result);
                if (queue->policy == POLICY_STOP)
                {
                    node_destroy(queue, current);
                    return ;
                }
            }
        }
        node_destroy(queue, current);
    }
}