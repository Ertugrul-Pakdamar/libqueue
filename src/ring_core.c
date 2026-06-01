/*
** MISRA C:2012 Rule 21.3 deviation:
**   malloc() is called once in ring_init() to allocate the ring buffer array,
**   and free() is called once in ring_destroy() to release it.
**   No dynamic allocation occurs after ring_init() returns.
**   Rationale: identical to queue_core.c — one-time allocation backs all
**   lock-free operations at zero further cost.
*/
#include <stdlib.h>
#include "libqueue.h"

static size_t   next_power_of_two(size_t n)
{
    size_t  p;

    p = 1;
    while (p < n)
        p <<= 1;
    return (p);
}

int     ring_init(t_ring *ring, size_t capacity)
{
    void    *buffer;

    capacity = next_power_of_two(capacity);
    buffer = malloc(sizeof(t_node *) * capacity);
    if (!buffer)
        return (0);
    arena_init(&ring->arena, buffer, sizeof(t_node *) * capacity);
    ring->buffer = (t_node **)arena_alloc(&ring->arena,
                        sizeof(t_node *) * capacity);
    ring->capacity = capacity;
    ring->mask = capacity - 1;
    osal_atomic_size_init(&ring->write_idx, 0);
    osal_atomic_size_init(&ring->read_idx, 0);
    return (1);
}

void    ring_destroy(t_ring *ring)
{
    void    *raw;

    raw = ring->arena.start_addr;
    arena_reset(&ring->arena);
    free(raw);
    ring->buffer = NULL;
}

void    ring_drain(t_ring *ring, t_queue *queue)
{
    t_node  *node;

    while ((node = ring_pop(ring)) != NULL)
        node_destroy(queue, node);
}