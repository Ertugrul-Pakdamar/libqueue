#include "libqueue.h"

static int dispatch_event(t_queue *queue, t_node *node)
{
    if (!queue || !node)
        return (0);
    if (node->event_type <= EVENT_TYPE_NONE || node->event_type >= EVENT_TYPE_MAX)
        return (1);
    if (!queue->handlers[node->event_type])
        return (1);
    return queue->handlers[node->event_type](node);
}

int     queue_register_handler(t_queue *queue, t_event_type type,
                                t_event_handler handler)
{
    if (!queue || type <= EVENT_TYPE_NONE || type >= EVENT_TYPE_MAX)
        return (0);
    queue->handlers[type] = handler;
    return (1);
}

int     run_node(t_queue *queue, t_node *node)
{
    return dispatch_event(queue, node);
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
        result = run_node(queue, current);
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
