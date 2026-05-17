#include "queue.h"

_Static_assert(
    offsetof(t_ring, write_idx) % RING_CACHE_LINE == 0,
    "write_idx cache line'a hizalanmamis");
_Static_assert(
    offsetof(t_ring, read_idx) % RING_CACHE_LINE == 0,
    "read_idx cache line'a hizalanmamis");

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
    capacity = next_power_of_two(capacity);
    ring->buffer = (t_node **)malloc(sizeof(t_node *) * capacity);
    if (!ring->buffer)
        return (0);
    ring->capacity = capacity;
    ring->mask = capacity - 1;
    atomic_init(&ring->write_idx, 0);
    atomic_init(&ring->read_idx, 0);
    return (1);
}

void    ring_destroy(t_ring *ring)
{
    free(ring->buffer);
    ring->buffer = NULL;
}

int     ring_push(t_ring *ring, t_node *node)
{
    size_t  write;
    size_t  read;

    write = atomic_load_explicit(&ring->write_idx, memory_order_relaxed);
    read  = atomic_load_explicit(&ring->read_idx,  memory_order_acquire);
    if ((write - read) >= ring->capacity)
        return (0);
    ring->buffer[write & ring->mask] = node;
    atomic_store_explicit(&ring->write_idx, write + 1, memory_order_release);
    return (1);
}

t_node  *ring_pop(t_ring *ring)
{
    size_t  read;
    size_t  write;
    t_node  *node;

    read  = atomic_load_explicit(&ring->read_idx,  memory_order_relaxed);
    write = atomic_load_explicit(&ring->write_idx, memory_order_acquire);
    if (read == write)
        return (NULL);
    node = ring->buffer[read & ring->mask];
    atomic_store_explicit(&ring->read_idx, read + 1, memory_order_release);
    return (node);
}
