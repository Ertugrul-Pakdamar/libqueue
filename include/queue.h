/**
 * @file queue.h
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

#ifndef QUEUE_H
# define QUEUE_H

/* ---- Version ------------------------------------------------------------- */
# define LIBQUEUE_VERSION_MAJOR 0
# define LIBQUEUE_VERSION_MINOR 1
# define LIBQUEUE_VERSION_PATCH 0
# define LIBQUEUE_VERSION       "0.1.0"

# include "../deps/libmem/include/memory.h"
# include "../deps/libosal/include/osal.h"

# define NODE_NAME_MAX   64  /**< Maximum length of a node name (incl. NUL). */
# define RING_CACHE_LINE 64  /**< Cache line size used for ring buffer padding. */

/**
 * @brief Controls queue behaviour when a node's process() returns non-zero.
 */
typedef enum e_fail_policy
{
    POLICY_CONTINUE, /**< Skip the failed node and continue with the next. */
    POLICY_STOP,     /**< Stop execution immediately after the first failure. */
    POLICY_RETRY     /**< Retry the failed node up to max_retries times. */
}   t_fail_policy;

/**
 * @brief A single unit of work in the queue.
 *
 * Nodes are allocated from the queue's internal pool. Do not allocate or free
 * nodes manually; use new_node() and node_destroy() instead.
 */
typedef struct s_node
{
    char            name[NODE_NAME_MAX]; /**< Human-readable node identifier. */
    int             (*process)(struct s_node *); /**< Work function. Returns 0 on success. */
    void            *args;              /**< Optional arguments passed to process(). */
    void            (*del_for_args)(void *); /**< Destructor for args. May be NULL. */
    int             retry_count;        /**< Retries attempted so far (reset each run). */
    int             max_retries;        /**< Per-node retry limit. -1 = use queue default. */
    struct s_node   *next;              /**< Next node in the intrusive linked list. */
}   t_node;

/**
 * @brief Configuration passed to new_node().
 *
 * Decouples construction parameters from internal node state to provide a
 * stable API independent of the t_node layout.
 */
typedef struct s_node_config
{
    const char      *name;             /**< Node name (copied; caller retains ownership). */
    int            (*process)(t_node *); /**< Work function. */
    void            *args;             /**< Arguments for process(). Ownership transferred to node. */
    void           (*del_for_args)(void *); /**< Destructor for args. May be NULL. */
    int              max_retries;      /**< Per-node retry limit. -1 = use queue default. */
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
    int              max_retries; /**< Retry limit used when node->max_retries == -1. */
    void           (*on_error)(t_node *, int); /**< Called after retries are exhausted. May be NULL. */
}   t_queue;

/**
 * @brief Configuration passed to queue_init().
 */
typedef struct s_queue_config
{
    t_fail_policy   policy;      /**< Failure policy. */
    int             max_retries; /**< Default retry limit (0 falls back to 3). */
    void          (*on_error)(t_node *, int); /**< Error callback. May be NULL. */
}   t_queue_config;

/**
 * @brief Initialize a queue and allocate its node pool.
 * @param queue    Uninitialized queue.
 * @param capacity Maximum number of live nodes at any one time.
 * @param config   Configuration. Pass NULL for defaults.
 * @return 1 on success, 0 on allocation failure.
 */
int     queue_init(t_queue *queue, size_t capacity, const t_queue_config *config);

/**
 * @brief Destroy a queue, releasing all nodes and the pool buffer.
 * @param queue Initialized queue.
 */
void    queue_destroy(t_queue *queue);

/**
 * @brief Allocate a new node from the queue's pool.
 * @param queue  Initialized queue (source pool).
 * @param config Node parameters. @c args ownership is transferred to the node.
 * @return Pointer to the new node, or NULL if the pool is exhausted.
 */
t_node  *new_node(t_queue *queue, const t_node_config *config);

/**
 * @brief Append a node to the tail of the queue.
 * @param queue Initialized queue.
 * @param node  Node created by new_node().
 */
void    add_node_to_queue(t_queue *queue, t_node *node);

/**
 * @brief Call del_for_args on the node's args and return it to the pool.
 * @param queue Owning queue.
 * @param node  Node to release. Must not be accessed after this call.
 */
void    node_destroy(t_queue *queue, t_node *node);

/**
 * @brief Destroy every node currently in the queue.
 * @param queue Initialized queue.
 */
void    clear_queue(t_queue *queue);

/**
 * @brief Invoke a single node's process() function.
 * @param node Node to run.
 * @return Return value of node->process(). 0 indicates success.
 */
int     run_node(t_node *node);

/**
 * @brief Run every node in the queue sequentially, then destroy each one.
 *
 * Respects the queue's failure policy and retry configuration.
 * @param queue Initialized queue.
 */
void    run_queue_synchronous(t_queue *queue);

/**
 * @brief Count the nodes in the queue.
 * @param node queue->head.
 * @return Number of nodes.
 */
int     size_of_queue(t_node *node);

/**
 * @brief Find the last node in the queue.
 * @param queue queue->head.
 * @return Pointer to the tail node, or NULL if the queue is empty.
 */
t_node  *get_last_node_of_queue(t_node *queue);

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
int     ring_init(t_ring *ring, size_t capacity);

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
int     ring_push(t_ring *ring, t_node *node);

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
int     ring_is_empty(t_ring *ring);

/**
 * @brief Check whether the ring has no free slots.
 * @param ring Initialized ring.
 * @return 1 if full, 0 otherwise.
 */
int     ring_is_full(t_ring *ring);

/**
 * @brief Pop and destroy every node remaining in the ring.
 *
 * Use before ring_destroy() when the ring may still contain live nodes.
 * @param ring  Initialized ring.
 * @param queue Owning queue used by node_destroy().
 */
void    ring_drain(t_ring *ring, t_queue *queue);

/**
 * @brief Async worker that consumes a ring buffer and executes each node.
 *
 * Respects the queue's failure policy and retry configuration.
 * Start with listener_start(); stop with listener_stop().
 */
typedef struct s_listener
{
    osal_task_t       task;    /**< OS task handle for the worker. */
    osal_atomic_int_t running; /**< Non-zero while the worker loop is active. */
    t_ring           *ring;    /**< Ring buffer the worker consumes. */
    t_queue          *queue;   /**< Queue providing policy, retries, and pool. */
}   t_listener;

/**
 * @brief Start the async listener worker task.
 * @param listener Uninitialized listener.
 * @param ring     Ring buffer to consume.
 * @param queue    Queue providing execution context.
 * @return 1 on success, 0 if the task could not be created.
 */
int     listener_start(t_listener *listener, t_ring *ring, t_queue *queue);

/**
 * @brief Signal the worker to stop and wait for it to exit.
 * @param listener Running listener.
 */
void    listener_stop(t_listener *listener);

#endif