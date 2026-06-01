#include "libqueue.h"

int32_t     ring_push(t_ring *ring, t_node *node)
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

size_t  ring_size(t_ring *ring)
{
    size_t  write;
    size_t  read;

    write = osal_atomic_size_load(&ring->write_idx);
    read  = osal_atomic_size_load(&ring->read_idx);
    return (write - read);
}

int32_t     ring_is_empty(t_ring *ring)
{
    return (ring_size(ring) == 0);
}

int32_t     ring_is_full(t_ring *ring)
{
    return (ring_size(ring) >= ring->capacity);
}