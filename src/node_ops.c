#include "libqueue.h"
#include <string.h>

t_node  *node_new(t_queue *queue, const t_node_config *config)
{
    t_node  *node;

    osal_mutex_lock(&queue->pool_lock);
    node = (t_node *)pool_alloc(&queue->node_pool);
    osal_mutex_unlock(&queue->pool_lock);
    if (!node)
        return (NULL);
    strncpy(node->name, config->name, NODE_NAME_MAX - 1);
    node->name[NODE_NAME_MAX - 1] = '\0';
    node->event_type = config->event_type;
    node->args = config->args;
    node->del_for_args = config->del_for_args;
    node->retry_count = 0;
    node->max_retries = config->max_retries;
    node->next = NULL;
    return (node);
}

void    node_destroy(t_queue *queue, t_node *node)
{
    if (!node)
        return ;
    if (node->args && node->del_for_args)
        (node->del_for_args)(node->args);
    osal_mutex_lock(&queue->pool_lock);
    pool_free(&queue->node_pool, node);
    osal_mutex_unlock(&queue->pool_lock);
}

static int32_t dispatch_event(t_queue *queue, t_node *node)
{
    if (!queue || !node)
        return (0);
    if (node->event_type <= EVENT_TYPE_NONE || node->event_type >= EVENT_TYPE_MAX)
        return (1);
    if (!queue->handlers[node->event_type])
        return (1);
    return queue->handlers[node->event_type](node);
}

int32_t     node_run(t_queue *queue, t_node *node)
{
    return dispatch_event(queue, node);
}
