/**
 * examples/03_async_listener.c
 *
 * Demonstrates how to build an async listener worker using the lock-free ring.
 *
 * The ring buffer is the hand-off point between the producer (main) and
 * the consumer (worker thread). No explicit locking is needed.
 * This replaces the old internal t_listener, showing how easy it is to
 * build custom worker threads using libosal and libqueue.
 *
 * Build:  make examples
 * Run:    ./examples/bin/03_async_listener
 */

#include <stdio.h>
#include "libqueue.h"

# define EVENT_PROCESS_TASK 1

/* ---- context & worker ----------------------------------------------------- */

typedef struct {
    t_ring            *ring;
    t_queue           *queue;
    osal_atomic_int_t  running;
} worker_ctx_t;

static int32_t process_task(t_node *self)
{
    printf("[worker] executing: %s\n", self->name);
    return (0);
}

/* Custom worker thread that drains the ring synchronously until stopped */
static void *worker_thread(void *arg)
{
    worker_ctx_t *ctx = (worker_ctx_t *)arg;
    
    while (osal_atomic_int_load(&ctx->running))
    {
        /* ring_run_sync drains the ring completely each loop iteration */
        ring_run_sync(ctx->ring, ctx->queue);
    }
    
    /* One last sync in case tasks were pushed while shutting down */
    ring_run_sync(ctx->ring, ctx->queue);
    
    return NULL;
}

/* --------------------------------------------------------------------------- */

int main(void)
{
    t_queue      queue;
    t_ring       ring;
    osal_task_t  task;
    worker_ctx_t ctx;

    const t_queue_config qcfg = { POLICY_CONTINUE, 0, NULL };

    if (!queue_init(&queue, 16, &qcfg))
    {
        fprintf(stderr, "queue_init failed\n");
        return (1);
    }

    queue_register_handler(&queue, EVENT_PROCESS_TASK, process_task);

    if (!ring_init(&ring, 8))
    {
        queue_destroy(&queue);
        fprintf(stderr, "ring_init failed\n");
        return (1);
    }

    /* Start worker thread */
    ctx.ring = &ring;
    ctx.queue = &queue;
    osal_atomic_int_init(&ctx.running, 1);
    if (osal_task_create(&task, worker_thread, &ctx) != 0)
    {
        ring_destroy(&ring);
        queue_destroy(&queue);
        fprintf(stderr, "osal_task_create failed\n");
        return (1);
    }

    /* Producer: push five tasks onto the ring. */
    const char *task_names[] = {
        "compress_frame",
        "send_packet",
        "log_event",
        "update_cache",
        "flush_buffer",
    };

    for (int32_t i = 0; i < 5; i++)
    {
        const t_node_config cfg = { task_names[i], EVENT_PROCESS_TASK, NULL, NULL, -1 };
        t_node *node = node_new(&queue, &cfg);
        if (!node)
        {
            fprintf(stderr, "pool exhausted at i=%d\n", i);
            break;
        }
        while (!ring_push(&ring, node))
            ;   /* spin if ring is momentarily full */
        printf("[main]   pushed:    %s\n", task_names[i]);
    }

    /* Wait for the worker to drain all nodes. */
    while (!ring_is_empty(&ring))
        ;

    /* Signal stop and wait for worker to exit */
    osal_atomic_int_store(&ctx.running, 0);
    osal_task_join(&task);

    ring_destroy(&ring);
    queue_destroy(&queue);

    printf("[main]   done.\n");
    return (0);
}
