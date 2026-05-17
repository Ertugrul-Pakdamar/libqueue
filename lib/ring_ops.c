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
    atomic_init(&ring->write_idx, 0);
    atomic_init(&ring->read_idx, 0);
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

    write = atomic_load_explicit(&ring->write_idx, memory_order_relaxed);
    read  = atomic_load_explicit(&ring->read_idx,  memory_order_acquire);
    if ((write - read) >= ring->capacity)
        return (0);
    ring->buffer[write & ring->mask] = node;
    atomic_store_explicit(&ring->write_idx, write + 1, memory_order_release);
    return (1);
}

size_t  ring_size(t_ring *ring)
{
    size_t  write;
    size_t  read;

    write = atomic_load_explicit(&ring->write_idx, memory_order_acquire);
    read  = atomic_load_explicit(&ring->read_idx,  memory_order_acquire);
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

    read  = atomic_load_explicit(&ring->read_idx,  memory_order_relaxed);
    write = atomic_load_explicit(&ring->write_idx, memory_order_acquire);
    if (read == write)
        return (NULL);
    node = ring->buffer[read & ring->mask];
    atomic_store_explicit(&ring->read_idx, read + 1, memory_order_release);
    return (node);
}
