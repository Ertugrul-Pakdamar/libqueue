#ifndef QUEUE_H
# define QUEUE_H

# include <stdio.h>
# include <stdint.h>
# include <stdlib.h>
# include <string.h>
# include <stddef.h>
# include "../libmem/include/memory.h"
# include "osal.h"

# define NODE_NAME_MAX   64
# define RING_CACHE_LINE 64

typedef enum e_fail_policy
{
    POLICY_CONTINUE,
    POLICY_STOP,
    POLICY_RETRY
}   t_fail_policy;

typedef struct s_node
{
    char            name[NODE_NAME_MAX];
    int             (*process)(struct s_node *);
    void            *args;
    void            (*del_for_args)(void *);
    int             retry_count;
    int             max_retries;
    struct s_node   *next;
}   t_node;

typedef struct s_node_config
{
    const char      *name;
    int            (*process)(t_node *);
    void            *args;
    void           (*del_for_args)(void *);
    int              max_retries;
}   t_node_config;

typedef struct s_queue
{
    t_node          *head;
    mem_pool_t       node_pool;
    osal_mutex_t     pool_lock;
    t_fail_policy    policy;
    int              max_retries;
    void           (*on_error)(t_node *, int);
}   t_queue;

typedef struct s_queue_config
{
    t_fail_policy   policy;
    int             max_retries;
    void          (*on_error)(t_node *, int);
}   t_queue_config;

/* Queue Alloc */
t_node  *new_node(t_queue *queue, const t_node_config *config);
int     queue_init(t_queue *queue, size_t capacity, const t_queue_config *config);
void    queue_destroy(t_queue *queue);

/* Queue Ops */
void    add_node_to_queue(t_queue *queue, t_node *node);

/* Queue Clear */
void    node_destroy(t_queue *queue, t_node *node);
void    clear_queue(t_queue *queue);

/* Queue Run */
int     run_node(t_node *node);
void    run_queue_synchronous(t_queue *queue);

/* Utilities */
int     size_of_queue(t_node *node);
t_node  *get_last_node_of_queue(t_node *queue);

typedef struct __attribute__((aligned(RING_CACHE_LINE))) s_ring
{
    t_node         **buffer;
    size_t           capacity;
    size_t           mask;
    mem_arena_t      arena;
    char             _pad0[RING_CACHE_LINE
                            - sizeof(t_node **)
                            - 2 * sizeof(size_t)
                            - sizeof(mem_arena_t)];

    osal_atomic_size_t write_idx;
    char               _pad1[RING_CACHE_LINE - sizeof(osal_atomic_size_t)];

    osal_atomic_size_t read_idx;
    char               _pad2[RING_CACHE_LINE - sizeof(osal_atomic_size_t)];
}   t_ring;

/* Ring Ops */
int     ring_init(t_ring *ring, size_t capacity);
void    ring_destroy(t_ring *ring);
int     ring_push(t_ring *ring, t_node *node);
t_node  *ring_pop(t_ring *ring);
size_t  ring_size(t_ring *ring);
int     ring_is_empty(t_ring *ring);
int     ring_is_full(t_ring *ring);
void    ring_drain(t_ring *ring, t_queue *queue);

typedef struct s_listener
{
    osal_task_t       task;
    osal_atomic_int_t running;
    t_ring           *ring;
    t_queue          *queue;
}   t_listener;

/* Listener */
int     listener_start(t_listener *listener, t_ring *ring, t_queue *queue);
void    listener_stop(t_listener *listener);

#endif