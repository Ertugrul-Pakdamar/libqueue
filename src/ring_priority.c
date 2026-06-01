#include "libqueue.h"
#include <stdlib.h>

static inline int get_highest_priority(size_t mask)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzl(mask);
#else
    int i = 0;
    while ((mask & 1) == 0) {
        mask >>= 1;
        i++;
    }
    return i;
#endif
}

int     prio_ring_init(t_prio_ring *pr, const size_t *capacities, int num_levels)
{
    int i;

    if (!pr || !capacities || num_levels <= 0 || num_levels > PRIORITY_MAX)
        return (0);

    pr->num_levels = num_levels;
    osal_atomic_size_init(&pr->ready_mask, 0);

    pr->rings = (t_ring *)malloc(sizeof(t_ring) * num_levels);
    if (!pr->rings)
        return (0);

    for (i = 0; i < num_levels; i++)
    {
        if (!ring_init(&pr->rings[i], capacities[i]))
        {
            /* Rollback initialized rings */
            while (--i >= 0)
                ring_destroy(&pr->rings[i]);
            free(pr->rings);
            pr->rings = NULL;
            return (0);
        }
    }

    return (1);
}

void    prio_ring_destroy(t_prio_ring *pr)
{
    int i;

    if (!pr || !pr->rings)
        return ;

    for (i = 0; i < pr->num_levels; i++)
    {
        ring_destroy(&pr->rings[i]);
    }
    free(pr->rings);
    pr->rings = NULL;
    pr->num_levels = 0;
}

int     prio_ring_push(t_prio_ring *pr, int level, t_node *node)
{
    if (!pr || !pr->rings || level < 0 || level >= pr->num_levels)
        return (0);

    if (ring_push(&pr->rings[level], node))
    {
        osal_atomic_size_fetch_or(&pr->ready_mask, (1UL << level));
        return (1);
    }
    return (0);
}

t_node  *prio_ring_pop(t_prio_ring *pr)
{
    size_t  mask;
    int     level;
    t_node *node;

    if (!pr || !pr->rings)
        return (NULL);

    mask = osal_atomic_size_load(&pr->ready_mask);
    while (mask != 0)
    {
        level = get_highest_priority(mask);
        node = ring_pop(&pr->rings[level]);
        if (node)
            return (node);

        /* Ring popped NULL. Clear the bit to prevent infinite spin. */
        osal_atomic_size_fetch_and(&pr->ready_mask, ~(1UL << level));

        /* Double check to prevent race condition: 
           If producer pushed *after* ring_pop returned NULL but *before* we cleared the bit,
           the bit is now cleared but the ring has an item. Set it back. */
        if (!ring_is_empty(&pr->rings[level]))
        {
            osal_atomic_size_fetch_or(&pr->ready_mask, (1UL << level));
        }

        mask = osal_atomic_size_load(&pr->ready_mask);
    }

    return (NULL);
}
