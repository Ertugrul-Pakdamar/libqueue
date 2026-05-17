#include "queue.h"

int     queue_init(t_queue *queue, size_t capacity, const t_queue_config *config)
{
    void    *buffer;

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
    return (1);
}

void    queue_destroy(t_queue *queue)
{
    clear_queue(queue);
    osal_mutex_destroy(&queue->pool_lock);
    free(queue->node_pool.start_addr);
    queue->node_pool.start_addr = NULL;
}

t_node  *new_node(t_queue *queue, const t_node_config *config)
{
    t_node  *node;

    osal_mutex_lock(&queue->pool_lock);
    node = (t_node *)pool_alloc(&queue->node_pool);
    osal_mutex_unlock(&queue->pool_lock);
    if (!node)
        return (NULL);
    strncpy(node->name, config->name, NODE_NAME_MAX - 1);
    node->name[NODE_NAME_MAX - 1] = '\0';
    node->process = config->process;
    node->args = config->args;
    node->del_for_args = config->del_for_args;
    node->retry_count = 0;
    node->max_retries = config->max_retries;
    node->next = NULL;
    return (node);
}
