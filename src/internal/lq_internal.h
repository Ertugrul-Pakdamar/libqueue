/**
 * @file lq_internal.h
 * @brief Internal structural definitions for libqueue.
 *
 * This file hides the implementation details from the public API, enforcing
 * Clean Architecture and preventing direct manipulation of the structures.
 *
 * This implementation relies solely on libosal for atomics and portability.
 */

#ifndef LQ_INTERNAL_H
# define LQ_INTERNAL_H

# include "libqueue.h"

/* Note on MISRA C:2012 Rule 1.2 / Dir 4.6:
 * We use libosal's atomic abstractions. All OS/Compiler specific headers
 * (like <stdatomic.h>) are confined to the OSAL implementation files.
 */

struct s_lq_node
{
    char             name[LQ_NODE_NAME_MAX];
    lq_event_type_t  event_type;
    void            *args;
    void           (*del_for_args)(void *);
    int32_t          retry_count;
    int32_t          max_retries;
    osal_atomic_ptr_t next; /* Portable atomic pointer */
};

/* Lock-Free MPSC Queue (Multi-Producer Single-Consumer) */
struct s_lq_queue
{
    osal_atomic_ptr_t head; /* Written by producers via XCHG */
    lq_node_t        *tail; /* Owned by consumer, not atomic */
};

/* Lock-Free SPSC Ring Buffer */
#define LQ_RING_CACHE_LINE 64

struct __attribute__((aligned(LQ_RING_CACHE_LINE))) s_lq_ring
{
    lq_node_t      **buffer;
    size_t           capacity;
    size_t           mask;
    char             _pad0[LQ_RING_CACHE_LINE - sizeof(lq_node_t**) - 2*sizeof(size_t)];

    osal_atomic_size_t write_idx;
    char               _pad1[LQ_RING_CACHE_LINE - sizeof(osal_atomic_size_t)];

    osal_atomic_size_t read_idx;
    char               _pad2[LQ_RING_CACHE_LINE - sizeof(osal_atomic_size_t)];
};

/* Event Dispatcher (Execution Layer) */
struct s_lq_dispatcher
{
    lq_fail_policy_t   policy;
    int32_t            max_retries;
    lq_error_handler_t on_error;
    lq_event_handler_t handlers[LQ_EVENT_TYPE_MAX];
};

#endif