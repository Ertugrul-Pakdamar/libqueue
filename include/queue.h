#ifndef QUEUE_H
# define QUEUE_H

# include <stdio.h>
# include <stdint.h>
# include <stdlib.h>
# include <string.h>
# include <stddef.h>
# include <stdatomic.h>
# include "../libmem/include/memory.h"

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
    size_t          args_size;
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
char    **get_node_names(t_node *first_node);
int     size_of_queue(t_node *node);
t_node  *get_last_node_of_queue(t_node *queue);

typedef struct __attribute__((aligned(RING_CACHE_LINE))) s_ring
{
    t_node          **buffer;
    size_t            capacity;
    size_t            mask;
    char              _pad0[RING_CACHE_LINE
                            - sizeof(t_node **)
                            - 2 * sizeof(size_t)];

    _Atomic size_t    write_idx;
    char              _pad1[RING_CACHE_LINE - sizeof(_Atomic size_t)];

    _Atomic size_t    read_idx;
    char              _pad2[RING_CACHE_LINE - sizeof(_Atomic size_t)];
}   t_ring;

/* Ring Ops */
int     ring_init(t_ring *ring, size_t capacity);
void    ring_destroy(t_ring *ring);
int     ring_push(t_ring *ring, t_node *node);
t_node  *ring_pop(t_ring *ring);

#endif