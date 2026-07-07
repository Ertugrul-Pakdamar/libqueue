#include "internal/lq_internal.h"
#include <string.h>

lq_node_t *lq_node_new(mem_pool_t *pool, const lq_node_config_t *config)
{
    lq_node_t *node;

    if (!pool || !config)
        return NULL;

    node = (lq_node_t *)pool_alloc(pool);
    if (!node)
        return NULL;

    /* Initialize node */
    strncpy(node->name, config->name, LQ_NODE_NAME_MAX - 1);
    node->name[LQ_NODE_NAME_MAX - 1] = '\0';
    node->event_type = config->event_type;
    node->args = config->args;
    node->del_for_args = config->del_for_args;
    node->retry_count = 0;
    node->max_retries = config->max_retries;
    
    /* Portable atomic init */
    osal_atomic_ptr_init(&node->next, NULL);

    return node;
}

void lq_node_destroy(mem_pool_t *pool, lq_node_t *node)
{
    if (!pool || !node)
        return;

    if (node->args && node->del_for_args)
        (node->del_for_args)(node->args);

    pool_free(pool, node);
}

void *lq_node_get_args(lq_node_t *node)
{
    if (!node)
        return NULL;
    return node->args;
}

const char *lq_node_get_name(lq_node_t *node)
{
    if (!node)
        return "NULL";
    return node->name;
}