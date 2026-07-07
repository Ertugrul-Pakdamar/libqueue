/**
 * @file libqueue.h
 * @brief libqueue public API — Lock-Free MPSC Queue, SPSC Ring, and Dispatcher.
 *
 * This framework is designed for safety-critical (MISRA C:2012) and bare-metal
 * environments. It features true zero-malloc allocation (100% static) and
 * lock-free (mutex-free) atomic data structures ensuring safe ISR execution.
 */

#ifndef LIBQUEUE_H
# define LIBQUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

# include "../deps/libmem/include/libmem.h"
# include "../deps/libosal/include/libosal.h"
# include <stdint.h>
# include <stddef.h>

/* ---- Version ------------------------------------------------------------- */
# define LIBQUEUE_VERSION_MAJOR 0
# define LIBQUEUE_VERSION_MINOR 3
# define LIBQUEUE_VERSION_PATCH 0
# define LIBQUEUE_VERSION       "0.3.0"

# define LQ_NODE_NAME_MAX   64
# define LQ_EVENT_TYPE_MAX  32
# define LQ_PRIORITY_MAX    32

/* ---- Status Codes -------------------------------------------------------- */
typedef enum e_lq_status
{
    LQ_SUCCESS = 0,
    LQ_ERR_NULL_PTR = -1,
    LQ_ERR_INVALID_PARAM = -2,
    LQ_ERR_OUT_OF_MEMORY = -3,
    LQ_ERR_QUEUE_FULL = -4,
    LQ_ERR_QUEUE_EMPTY = -5,
    LQ_ERR_HANDLER_NOT_FOUND = -6
} lq_status_t;

/* ---- Fail Policies ------------------------------------------------------- */
typedef enum e_lq_fail_policy
{
    LQ_POLICY_CONTINUE,
    LQ_POLICY_STOP,
    LQ_POLICY_RETRY
} lq_fail_policy_t;

typedef int32_t lq_event_type_t;
# define LQ_EVENT_TYPE_NONE 0

/* ---- Opaque Types (Clean Architecture) ----------------------------------- */
typedef struct s_lq_node lq_node_t;
typedef struct s_lq_queue lq_queue_t;
typedef struct s_lq_ring lq_ring_t;
typedef struct s_lq_dispatcher lq_dispatcher_t;

/* Size constants for static allocation (Update these if internal structs change) */
# define LQ_NODE_SIZE       128
# define LQ_QUEUE_SIZE      32
# define LQ_RING_SIZE       256
# define LQ_DISPATCHER_SIZE 512

/**
 * Macros to define static storage for opaque types.
 * Usage: LQ_STATIC_QUEUE(my_queue);
 */
# define LQ_STATIC_QUEUE(name)      uint8_t name[LQ_QUEUE_SIZE] __attribute__((aligned(8)))
# define LQ_STATIC_RING(name)       uint8_t name[LQ_RING_SIZE] __attribute__((aligned(8)))
# define LQ_STATIC_DISPATCHER(name) uint8_t name[LQ_DISPATCHER_SIZE] __attribute__((aligned(8)))
# define LQ_STATIC_NODES(name, cap) uint8_t name[(cap) * LQ_NODE_SIZE] __attribute__((aligned(8)))

typedef int32_t (*lq_event_handler_t)(lq_node_t *node);
typedef void (*lq_error_handler_t)(lq_node_t *node, int32_t result);

/* ---- Node Configuration -------------------------------------------------- */
typedef struct s_lq_node_config
{
    const char      *name;
    lq_event_type_t  event_type;
    void            *args;
    void           (*del_for_args)(void *);
    int32_t          max_retries;
} lq_node_config_t;

/* ---- Node Operations ----------------------------------------------------- */
/**
 * @brief Allocate and initialize a node from a libmem pool.
 * @note libmem pool operations are not thread-safe by default.
 */
lq_node_t  *lq_node_new(mem_pool_t *pool, const lq_node_config_t *config);
void        lq_node_destroy(mem_pool_t *pool, lq_node_t *node);
void       *lq_node_get_args(lq_node_t *node);
const char *lq_node_get_name(lq_node_t *node);

/* ---- Queue Operations (MPSC Lock-Free) ----------------------------------- */
lq_status_t lq_queue_init(lq_queue_t *queue);
lq_status_t lq_queue_push(lq_queue_t *queue, lq_node_t *node);
lq_node_t  *lq_queue_pop(lq_queue_t *queue);
int32_t     lq_queue_is_empty(lq_queue_t *queue);

/* ---- Ring Operations (SPSC Lock-Free) ------------------------------------ */
lq_status_t lq_ring_init(lq_ring_t *ring, void *buffer, size_t capacity);
lq_status_t lq_ring_push(lq_ring_t *ring, lq_node_t *node);
lq_node_t  *lq_ring_pop(lq_ring_t *ring);
int32_t     lq_ring_is_empty(lq_ring_t *ring);
int32_t     lq_ring_is_full(lq_ring_t *ring);

/* ---- Dispatcher Operations (Execution Layer) ----------------------------- */
typedef struct s_lq_dispatcher_config
{
    lq_fail_policy_t   policy;
    int32_t            max_retries;
    lq_error_handler_t on_error;
} lq_dispatcher_config_t;

lq_status_t lq_dispatcher_init(lq_dispatcher_t *disp, const lq_dispatcher_config_t *config);
lq_status_t lq_dispatcher_register(lq_dispatcher_t *disp, lq_event_type_t type, lq_event_handler_t handler);

/**
 * @brief Consumes nodes from the given MPSC queue and executes them.
 * @param max_events Maximum nodes to process per call (bounded execution). 0 = unlimited.
 */
lq_status_t lq_dispatcher_run_queue(lq_dispatcher_t *disp, lq_queue_t *queue, mem_pool_t *pool, size_t max_events);

/**
 * @brief Consumes nodes from the given SPSC ring and executes them.
 * @param max_events Maximum nodes to process per call (bounded execution). 0 = unlimited.
 */
lq_status_t lq_dispatcher_run_ring(lq_dispatcher_t *disp, lq_ring_t *ring, mem_pool_t *pool, size_t max_events);

#ifdef __cplusplus
}
#endif
#endif