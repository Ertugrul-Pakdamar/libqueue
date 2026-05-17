#include "queue.h"

int     run_node(t_node *node)
{
    if (node == NULL || node->process == NULL)
        return (0);
    return (node->process(node));
}

void    run_queue_synchronous(t_queue *queue)
{
    t_node  *current;
    t_node  *next;
    int     result;

    if (!queue || !queue->head)
        return ;
    current = queue->head;
    queue->head = NULL;
    while (current)
    {
        next = current->next;
        result = run_node(current);
        if (result != 0)
        {
            int effective = (current->max_retries >= 0)
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
