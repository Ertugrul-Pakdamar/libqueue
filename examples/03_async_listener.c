/**
 * examples/03_async_listener.c
 *
 * Demonstrates the async listener workflow:
 *   - listener_start  spawns a worker thread that drains the ring buffer
 *   - producer pushes nodes with ring_push from the main thread (SPSC)
 *   - listener_stop   signals the worker and joins the thread
 *
 * The ring buffer is the hand-off point between the producer (main) and
 * the consumer (listener worker).  No explicit locking is needed.
 *
 * Build:  make examples
 * Run:    ./examples/bin/03_async_listener
 */

#include <stdio.h>
#include "libqueue.h"

# define EVENT_PROCESS_TASK 1

/* ---- work functions ------------------------------------------------------- */

static int process_task(t_node *self)
{
    printf("[worker] executing: %s\n", self->name);
    return (0);
}

/* --------------------------------------------------------------------------- */

int main(void)
{
    t_queue     queue;
    t_ring      ring;
    t_listener  listener;

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
    if (!listener_start(&listener, &ring, &queue))
    {
        ring_destroy(&ring);
        queue_destroy(&queue);
        fprintf(stderr, "listener_start failed\n");
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

    for (int i = 0; i < 5; i++)
    {
        const t_node_config cfg = { task_names[i], EVENT_PROCESS_TASK, NULL, NULL, -1 };
        t_node *node = new_node(&queue, &cfg);
        if (!node)
        {
            fprintf(stderr, "pool exhausted at i=%d\n", i);
            break;
        }
        while (!ring_push(&ring, node))
            ;   /* spin if ring is momentarily full */
        printf("[main]   pushed:    %s\n", task_names[i]);
    }

    /* Wait for the worker to drain all remaining nodes. */
    while (!ring_is_empty(&ring))
        ;

    listener_stop(&listener);
    ring_destroy(&ring);
    queue_destroy(&queue);

    printf("[main]   done.\n");
    return (0);
}
