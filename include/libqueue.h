/**
 * @file libqueue.h
 * @brief libqueue public API — dynamic queue, lock-free ring buffer, async listener.
 *
 * All memory is allocated up front (pool, arena). No malloc occurs after
 * queue_init() / ring_init(), making the library suitable for deterministic
 * real-time environments.
 *
 * Threading model:
 *  - t_queue and its pool are protected by an internal mutex (pool_lock).
 *  - t_ring is a single-producer / single-consumer (SPSC) lock-free ring.
 *  - t_listener spawns one worker task; safe to push from one producer only.
 */

#ifndef LIBQUEUE_H
# define LIBQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Version ------------------------------------------------------------- */
# define LIBQUEUE_VERSION_MAJOR 0
# define LIBQUEUE_VERSION_MINOR 2
# define LIBQUEUE_VERSION_PATCH 0
# define LIBQUEUE_VERSION       "0.2.0"

# include "../deps/libmem/include/libmem.h"
# include "../deps/libosal/include/libosal.h"
#include <libmem.h>
#include <stdint.h>

# define NODE_NAME_MAX   64  /**< Maximum length of a node name (incl. NUL). */
# define RING_CACHE_LINE 64  /**< Cache line size used for ring buffer padding. */

/**
 * @brief Controls queue behaviour when a node handler returns non-zero.
 */
typedef enum e_fail_policy
{
    POLICY_CONTINUE, /**< Skip the failed node and continue with the next. */
    POLICY_STOP,     /**< Stop execution immediately after the first failure. */
    POLICY_RETRY     /**< Retry the failed node up to max_retries times. */
}   t_fail_policy;

/**
 * @brief Event type identifier for a work node.
 *
 * Event types are numeric identifiers used by the queue dispatcher to route
 * incoming work items to the correct handler.
 */
typedef int32_t             t_event_type;

# define EVENT_TYPE_NONE 0
# define EVENT_TYPE_MAX  32 /**< Maximum number of event types supported. */

/**
 * @brief A single unit of work in the queue.
 *
 * Nodes are allocated from the queue's internal pool. Do not allocate or free
 * nodes manually; use new_node() and node_destroy() instead.
 */
typedef struct s_node
{
    char            name[NODE_NAME_MAX]; /**< Human-readable node identifier. */
    t_event_type    event_type;         /**< Event type handled by the dispatcher. */
    void            *args;              /**< Optional arguments passed to the handler. */
    void            (*del_for_args)(void *); /**< Destructor for args. May be NULL. */
    int32_t             retry_count;        /**< Retries attempted so far (reset each run). */
    int32_t             max_retries;        /**< Per-node retry limit. -1 = use queue default. */
    struct s_node   *next;              /**< Next node in the intrusive linked list. */
}   t_node;

typedef int32_t (*t_event_handler)(t_node *node);

/**
 * @brief Configuration passed to new_node().
 *
 * Decouples construction parameters from internal node state to provide a
 * stable API independent of the t_node layout.
 */
typedef struct s_node_config
{
    const char      *name;             /**< Node name (copied; caller retains ownership). */
    t_event_type     event_type;        /**< Event type that will be dispatched. */
    void            *args;             /**< Arguments for the handler. Ownership transferred to node. */
    void           (*del_for_args)(void *); /**< Destructor for args. May be NULL. */
    int32_t              max_retries;      /**< Per-node retry limit. -1 = use queue default. */
}   t_node_config;

/**
 * @brief A FIFO queue of t_node work items backed by a fixed-size pool.
 */
typedef struct s_queue
{
    t_node          *head;        /**< Front of the queue (next node to run). */
    mem_pool_t       node_pool;   /**< Pool allocator for t_node instances. */
    osal_mutex_t     pool_lock;   /**< Protects pool_alloc/pool_free across threads. */
    t_fail_policy    policy;      /**< Default failure policy. */
    int32_t              max_retries; /**< Retry limit used when node->max_retries == -1. */
    void           (*on_error)(t_node *, int32_t); /**< Called after retries are exhausted. May be NULL. */
    t_event_handler  handlers[EVENT_TYPE_MAX]; /**< Event dispatch table. */
}   t_queue;

/**
 * @brief Configuration passed to queue_init().
 */
typedef struct s_queue_config
{
    t_fail_policy   policy;      /**< Failure policy. */
    int32_t             max_retries; /**< Default retry limit (0 falls back to 3). */
    void          (*on_error)(t_node *, int32_t); /**< Error callback. May be NULL. */
}   t_queue_config;

/**
 * @brief Initialize a queue and allocate its node pool.
 * @param queue    Uninitialized queue.
 * @param capacity Maximum number of live nodes at any one time.
 * @param config   Configuration. Pass NULL for defaults.
 * @return 1 on success, 0 on allocation failure.
 */
int32_t     queue_init(t_queue *queue, size_t capacity, const t_queue_config *config);

/**
 * @brief Destroy a queue, releasing all nodes and the pool buffer.
 * @param queue Initialized queue.
 */
void    queue_destroy(t_queue *queue);

/**
 * @brief Append a node to the tail of the queue.
 * @param queue Initialized queue.
 * @param node  Node created by node_new().
 */
void    queue_push(t_queue *queue, t_node *node);

/**
 * @brief Pop the oldest node from the front of the queue.
 * @param queue Initialized queue.
 * @return Next node pointer, or NULL if the queue is empty.
 */
t_node  *queue_pop(t_queue *queue);

/**
 * @brief Destroy every node currently in the queue.
 * @param queue Initialized queue.
 */
void    queue_clear(t_queue *queue);

/**
 * @brief Count the nodes in the queue.
 * @param queue Initialized queue.
 * @return Number of nodes.
 */
int32_t     queue_size(t_queue *queue);

/**
 * @brief Check whether the queue has no items.
 * @param queue Initialized queue.
 * @return 1 if empty, 0 otherwise.
 */
int32_t     queue_is_empty(t_queue *queue);

/**
 * @brief Find the last node in the queue.
 * @param queue Initialized queue.
 * @return Pointer to the tail node, or NULL if the queue is empty.
 */
t_node  *queue_tail(t_queue *queue);

/**
 * @brief Run every node in the queue sequentially, then destroy each one.
 *
 * Respects the queue's failure policy and retry configuration.
 * @param queue Initialized queue.
 */
void    queue_run_sync(t_queue *queue);

/**
 * @brief Register an event handler for a queue.
 * @param queue Initialized queue.
 * @param type  Event type.
 * @param handler Handler function.
 * @return 1 on success, 0 if the type is invalid.
 */
int32_t     queue_register_handler(t_queue *queue, t_event_type type, t_event_handler handler);

/* ---- Node Operations ----------------------------------------------------- */

/**
 * @brief Allocate a new node from the queue's pool.
 * @param queue  Initialized queue (source pool).
 * @param config Node parameters. @c args ownership is transferred to the node.
 * @return Pointer to the new node, or NULL if the pool is exhausted.
 */
t_node  *node_new(t_queue *queue, const t_node_config *config);

/**
 * @brief Call del_for_args on the node's args and return it to the pool.
 * @param queue Owning queue.
 * @param node  Node to release. Must not be accessed after this call.
 */
void    node_destroy(t_queue *queue, t_node *node);

/**
 * @brief Invoke the dispatcher for a single node's event.
 * @param queue Queue providing handler lookup and retry configuration.
 * @param node Node to dispatch.
 * @return Handler return code. 0 indicates success.
 */
int32_t     node_run(t_queue *queue, t_node *node);

/**
 * @brief Lock-free SPSC ring buffer of t_node pointers.
 *
 * write_idx and read_idx live on separate cache lines to prevent false
 * sharing between the producer and consumer threads.
 */
typedef struct __attribute__((aligned(RING_CACHE_LINE))) s_ring
{
    t_node         **buffer;   /**< Circular array of node pointers. */
    size_t           capacity; /**< Slot count (always a power of two). */
    size_t           mask;     /**< capacity - 1; enables fast index wrap via &. */
    mem_arena_t      arena;    /**< Arena backing the buffer array. */
    char             _pad0[RING_CACHE_LINE
                            - sizeof(t_node **)
                            - 2 * sizeof(size_t)
                            - sizeof(mem_arena_t)];

    osal_atomic_size_t write_idx; /**< Next write slot (producer-owned). */
    char               _pad1[RING_CACHE_LINE - sizeof(osal_atomic_size_t)];

    osal_atomic_size_t read_idx;  /**< Next read slot (consumer-owned). */
    char               _pad2[RING_CACHE_LINE - sizeof(osal_atomic_size_t)];
}   t_ring;

/**
 * @brief Initialize a ring buffer and allocate its backing array.
 * @param ring     Uninitialized ring.
 * @param capacity Desired capacity; rounded up to the next power of two.
 * @return 1 on success, 0 on allocation failure.
 */
int32_t     ring_init(t_ring *ring, size_t capacity);

/**
 * @brief Free the ring buffer's backing array.
 *
 * Nodes still in the ring are not freed. Call ring_drain() first if needed.
 * @param ring Initialized ring.
 */
void    ring_destroy(t_ring *ring);

/**
 * @brief Push a node onto the ring (producer side).
 * @param ring Ring buffer.
 * @param node Node pointer to enqueue.
 * @return 1 on success, 0 if the ring is full.
 */
int32_t     ring_push(t_ring *ring, t_node *node);

/**
 * @brief Pop the oldest node from the ring (consumer side).
 * @param ring Ring buffer.
 * @return Next node pointer, or NULL if the ring is empty.
 */
t_node  *ring_pop(t_ring *ring);

/**
 * @brief Return the current number of items in the ring.
 * @param ring Initialized ring.
 * @return Occupancy count.
 */
size_t  ring_size(t_ring *ring);

/**
 * @brief Check whether the ring has no items.
 * @param ring Initialized ring.
 * @return 1 if empty, 0 otherwise.
 */
int32_t     ring_is_empty(t_ring *ring);

/**
 * @brief Check whether the ring has no free slots.
 * @param ring Initialized ring.
 * @return 1 if full, 0 otherwise.
 */
int32_t     ring_is_full(t_ring *ring);

/**
 * @brief Pop and destroy every node remaining in the ring.
 *
 * Use before ring_destroy() when the ring may still contain live nodes.
 * @param ring  Initialized ring.
 * @param queue Owning queue used by node_destroy().
 */
void    ring_drain(t_ring *ring, t_queue *queue);

/**
 * @brief Drain the ring and run all nodes sequentially until empty.
 *
 * Respects the queue's failure policy and retry configuration.
 * @param ring  Initialized ring buffer.
 * @param queue Queue providing execution context.
 */
void    ring_run_sync(t_ring *ring, t_queue *queue);

/* ---- Priority Management ------------------------------------------------- */

# define PRIORITY_MAX 32 /**< Maximum number of priority levels. */

/**
 * @brief Synchronous multi-level priority queue backed by an array of t_queue.
 */
typedef struct s_prio_queue
{
    t_queue  *queues;      /**< Array of initialized queues (one per level). */
    int32_t       num_levels;  /**< Number of priority levels. */
    uint32_t  ready_mask;  /**< Bitmask indicating which levels contain nodes. */
}   t_prio_queue;

/**
 * @brief Initialize a synchronous priority queue group.
 * @param pq          Uninitialized priority queue structure.
 * @param capacities  Array of capacities for each priority level queue.
 * @param configs     Array of configurations for each priority level queue (can be NULL).
 * @param num_levels  Number of priority levels (max PRIORITY_MAX).
 * @return 1 on success, 0 on allocation failure.
 */
int32_t     prio_queue_init(t_prio_queue *pq, const size_t *capacities, const t_queue_config *configs, int32_t num_levels);

/**
 * @brief Destroy a synchronous priority queue group.
 * @param pq Initialized priority queue structure.
 */
void    prio_queue_destroy(t_prio_queue *pq);

/**
 * @brief Push a node into a specific priority level of the synchronous queue.
 * @param pq    Initialized priority queue structure.
 * @param level Priority level (0 is highest priority).
 * @param node  Node to enqueue.
 */
void    prio_queue_push(t_prio_queue *pq, int32_t level, t_node *node);

/**
 * @brief Pop the highest priority node from the synchronous priority queues.
 * @param pq Initialized priority queue structure.
 * @return Highest priority node, or NULL if all queues are empty.
 */
t_node  *prio_queue_pop(t_prio_queue *pq);

/**
 * @brief Asynchronous/ISR-safe multi-level priority ring backed by an array of t_ring.
 */
typedef struct s_prio_ring
{
    t_ring              *rings;      /**< Array of initialized ring buffers. */
    int32_t                  num_levels; /**< Number of priority levels. */
    osal_atomic_size_t   ready_mask; /**< Atomic bitmask of non-empty levels. */
}   t_prio_ring;

/**
 * @brief Initialize an asynchronous priority ring group.
 * @param pr         Uninitialized priority ring structure.
 * @param capacities Array of capacities for each priority level ring.
 * @param num_levels Number of priority levels (max PRIORITY_MAX).
 * @return 1 on success, 0 on allocation failure.
 */
int32_t     prio_ring_init(t_prio_ring *pr, const size_t *capacities, int32_t num_levels);

/**
 * @brief Destroy an asynchronous priority ring group.
 * @param pr Initialized priority ring structure.
 */
void    prio_ring_destroy(t_prio_ring *pr);

/**
 * @brief Push a node into a specific priority level of the asynchronous ring.
 * @param pr    Initialized priority ring structure.
 * @param level Priority level (0 is highest priority).
 * @param node  Node to enqueue.
 * @return 1 on success, 0 if the specified ring is full or level is invalid.
 */
int32_t     prio_ring_push(t_prio_ring *pr, int32_t level, t_node *node);

/**
 * @brief Pop the highest priority node from the asynchronous priority rings.
 * @param pr Initialized priority ring structure.
 * @return Highest priority node, or NULL if all rings are empty.
 */
t_node  *prio_ring_pop(t_prio_ring *pr);

#ifdef __cplusplus
}
#endif
#endif