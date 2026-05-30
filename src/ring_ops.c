/*
** MISRA C:2012 Rule 21.3 deviation:
**   malloc() is called once in ring_init() to allocate the ring buffer array,
**   and free() is called once in ring_destroy() to release it.
**   No dynamic allocation occurs after ring_init() returns.
**   Rationale: identical to queue_alloc.c — one-time allocation backs all
**   lock-free operations at zero further cost.
*/
#include <stdlib.h>
#include "libqueue.h"

_Static_assert(
    offsetof(t_ring, write_idx) % RING_CACHE_LINE == 0,
    "write_idx must be aligned to a cache line");
_Static_assert(
    offsetof(t_ring, read_idx) % RING_CACHE_LINE == 0,
    "read_idx must be aligned to a cache line");

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

int     ring_push(t_ring *ring, t_node *node)
{
    size_t  write;
    size_t  read;

    write = osal_atomic_size_load_relaxed(&ring->write_idx);
    read  = osal_atomic_size_load(&ring->read_idx);
    if ((write - read) >= ring->capacity)
        return (0);
    ring->buffer[write & ring->mask] = node;
    osal_atomic_size_store(&ring->write_idx, write + 1);
    return (1);
}

size_t  ring_size(t_ring *ring)
{
    size_t  write;
    size_t  read;

    write = osal_atomic_size_load(&ring->write_idx);
    read  = osal_atomic_size_load(&ring->read_idx);
    return (write - read);
}

int     ring_is_empty(t_ring *ring)
{
    return (ring_size(ring) == 0);
}

int     ring_is_full(t_ring *ring)
{
    return (ring_size(ring) >= ring->capacity);
}

void    ring_drain(t_ring *ring, t_queue *queue)
{
    t_node  *node;

    while ((node = ring_pop(ring)) != NULL)
        node_destroy(queue, node);
}

t_node  *ring_pop(t_ring *ring)
{
    size_t  read;
    size_t  write;
    t_node  *node;

    read  = osal_atomic_size_load_relaxed(&ring->read_idx);
    write = osal_atomic_size_load(&ring->write_idx);
    if (read == write)
        return (NULL);
    node = ring->buffer[read & ring->mask];
    osal_atomic_size_store(&ring->read_idx, read + 1);
    return (node);
}
