#include "internal/lq_internal.h"

lq_status_t lq_queue_init(lq_queue_t *queue)
{
    if (!queue)
        return LQ_ERR_INVALID_PARAM;

    osal_atomic_ptr_init(&queue->head, NULL);
    queue->tail = NULL;

    return LQ_SUCCESS;
}

lq_status_t lq_queue_push(lq_queue_t *queue, lq_node_t *node)
{
    lq_node_t *prev_head;

    if (!queue || !node)
        return LQ_ERR_INVALID_PARAM;

    /* Push is a single atomic exchange. 
       We make the new node point to whatever was previously the head. */
    prev_head = (lq_node_t *)osal_atomic_ptr_exchange(&queue->head, node);
    osal_atomic_ptr_store(&node->next, prev_head);

    return LQ_SUCCESS;
}

lq_node_t *lq_queue_pop(lq_queue_t *queue)
{
    lq_node_t *head;
    lq_node_t *curr;
    lq_node_t *prev;
    lq_node_t *next;

    if (!queue)
        return NULL;

    /* If we have nodes in the consumer-owned tail list, pop from there */
    if (queue->tail != NULL)
    {
        lq_node_t *node = queue->tail;
        queue->tail = (lq_node_t *)osal_atomic_ptr_load(&node->next);
        return node;
    }

    /* Tail is empty. Grab the entire producer list via atomic exchange */
    head = (lq_node_t *)osal_atomic_ptr_exchange(&queue->head, NULL);
    
    if (head == NULL)
        return NULL; /* Queue is completely empty */

    /* The list we got is in reverse order (LIFO). We must reverse it to restore FIFO. */
    curr = head;
    prev = NULL;
    while (curr != NULL)
    {
        next = (lq_node_t *)osal_atomic_ptr_load(&curr->next);
        osal_atomic_ptr_store(&curr->next, prev);
        prev = curr;
        curr = next;
    }

    /* 'prev' is now the oldest node. 
       Its next pointer points to the second oldest, etc.
       We return 'prev', and keep the rest in queue->tail for future pops. */
    queue->tail = (lq_node_t *)osal_atomic_ptr_load(&prev->next);
    
    return prev;
}

int32_t lq_queue_is_empty(lq_queue_t *queue)
{
    if (!queue)
        return 1;
    
    if (queue->tail != NULL)
        return 0;
        
    return (osal_atomic_ptr_load(&queue->head) == NULL);
}