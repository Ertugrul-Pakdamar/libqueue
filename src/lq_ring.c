#include "internal/lq_internal.h"

static size_t next_power_of_two(size_t n)
{
    size_t p = 1;
    while (p < n)
        p <<= 1;
    return p;
}

lq_status_t lq_ring_init(lq_ring_t *ring, void *buffer, size_t capacity)
{
    if (!ring || !buffer || capacity == 0)
        return LQ_ERR_INVALID_PARAM;

    size_t pow2_cap = next_power_of_two(capacity);

    ring->buffer = (lq_node_t **)buffer;
    ring->capacity = pow2_cap;
    ring->mask = pow2_cap - 1;
    
    osal_atomic_size_init(&ring->write_idx, 0);
    osal_atomic_size_init(&ring->read_idx, 0);

    return LQ_SUCCESS;
}

lq_status_t lq_ring_push(lq_ring_t *ring, lq_node_t *node)
{
    size_t current_write;
    size_t current_read;
    size_t next_write;

    if (!ring || !node)
        return LQ_ERR_INVALID_PARAM;

    current_write = osal_atomic_size_load_relaxed(&ring->write_idx);
    current_read = osal_atomic_size_load(&ring->read_idx);
    
    next_write = (current_write + 1) & ring->mask;

    if (next_write == (current_read & ring->mask))
        return LQ_ERR_QUEUE_FULL;

    ring->buffer[current_write & ring->mask] = node;
    osal_atomic_size_store(&ring->write_idx, current_write + 1);

    return LQ_SUCCESS;
}

lq_node_t *lq_ring_pop(lq_ring_t *ring)
{
    size_t current_read;
    size_t current_write;
    lq_node_t *node;

    if (!ring)
        return NULL;

    current_read = osal_atomic_size_load_relaxed(&ring->read_idx);
    current_write = osal_atomic_size_load(&ring->write_idx);

    if (current_read == current_write)
        return NULL;

    node = ring->buffer[current_read & ring->mask];
    osal_atomic_size_store(&ring->read_idx, current_read + 1);

    return node;
}

int32_t lq_ring_is_empty(lq_ring_t *ring)
{
    size_t r, w;
    if (!ring) return 1;
    r = osal_atomic_size_load_relaxed(&ring->read_idx);
    w = osal_atomic_size_load_relaxed(&ring->write_idx);
    return (r == w);
}

int32_t lq_ring_is_full(lq_ring_t *ring)
{
    size_t r, w;
    if (!ring) return 0;
    r = osal_atomic_size_load_relaxed(&ring->read_idx);
    w = osal_atomic_size_load_relaxed(&ring->write_idx);
    return (((w + 1) & ring->mask) == (r & ring->mask));
}