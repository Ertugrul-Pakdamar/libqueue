#include "libqueue.h"
#include <stdlib.h>

static inline int32_t get_highest_priority(uint32_t mask)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(mask);
#else
    int32_t i = 0;
    while ((mask & 1) == 0) {
        mask >>= 1;
        i++;
    }
    return i;
#endif
}

int32_t     prio_queue_init(t_prio_queue *pq, const size_t *capacities, const t_queue_config *configs, int32_t num_levels)
{
    int32_t i;

    if (!pq || !capacities || num_levels <= 0 || num_levels > PRIORITY_MAX)
        return (0);

    pq->num_levels = num_levels;
    pq->ready_mask = 0;

    pq->queues = (t_queue *)malloc(sizeof(t_queue) * num_levels);
    if (!pq->queues)
        return (0);

    for (i = 0; i < num_levels; i++)
    {
        const t_queue_config *cfg = configs ? &configs[i] : NULL;
        if (!queue_init(&pq->queues[i], capacities[i], cfg))
        {
            while (--i >= 0)
                queue_destroy(&pq->queues[i]);
            free(pq->queues);
            pq->queues = NULL;
            return (0);
        }
    }
    return (1);
}

void    prio_queue_destroy(t_prio_queue *pq)
{
    int32_t i;

    if (!pq || !pq->queues)
        return ;

    for (i = 0; i < pq->num_levels; i++)
    {
        queue_destroy(&pq->queues[i]);
    }
    free(pq->queues);
    pq->queues = NULL;
    pq->num_levels = 0;
}

void    prio_queue_push(t_prio_queue *pq, int32_t level, t_node *node)
{
    if (!pq || !pq->queues || level < 0 || level >= pq->num_levels)
        return ;

    queue_push(&pq->queues[level], node);
    pq->ready_mask |= (1U << level);
}

t_node  *prio_queue_pop(t_prio_queue *pq)
{
    int32_t     level;
    t_node *node;

    if (!pq || !pq->queues)
        return (NULL);

    while (pq->ready_mask != 0)
    {
        level = get_highest_priority(pq->ready_mask);
        
        if (pq->queues[level].head)
        {
            /* Pop the head */
            node = pq->queues[level].head;
            pq->queues[level].head = node->next;
            node->next = NULL;
            
            /* If queue became empty, clear the bit */
            if (!pq->queues[level].head)
                pq->ready_mask &= ~(1U << level);
                
            return (node);
        }
        else
        {
            /* Should not happen if mask is correct, but clear bit just in case */
            pq->ready_mask &= ~(1U << level);
        }
    }

    return (NULL);
}
