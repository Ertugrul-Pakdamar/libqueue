/*
 * MCU-style example: ISR-safe producer (simulated) + main-loop consumer.
 * - Preallocate a small pool of nodes at init time.
 * - ISR (simulated by a lightweight function) only calls `ring_push` with
 *   preallocated nodes. No dynamic allocation or mutexes inside ISR.
 * - Main loop polls `ring_pop` and dispatches via `run_node` / registered
 *   handlers.
 * This example is intended as guidance for embedded/bare-metal usage.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "queue.h"

#define RING_SIZE 8
#define NODE_POOL_SIZE 8

static t_ring ring;
static t_queue queue;

/* Preallocated nodes visible to both ISR and main */
static t_node *isr_nodes[NODE_POOL_SIZE];
static size_t isr_nodes_top = 0; /* simple stack for example; init-only */

/* Event type */
#define EVT_SENSOR_SAMPLE 1

int sensor_handler(t_node *n)
{
    const char *s = (const char *)n->args;
    printf("[handler] processed node: %s\n", s);
    return 0;
}

/* Simulated ISR: only performs ring_push of a preallocated node. */
void simulated_isr_emit(void)
{
    /* Pop a node from the preallocated stack (non-atomic; in real ISR this
       would be from a lock-free free list or a static array indexed by head).
       For this host-side example we keep it simple. */
    if (isr_nodes_top == 0)
        return; /* no free nodes */

    t_node *n = isr_nodes[--isr_nodes_top];
    /* populate args (in real code, ISR should write small POD data) */
    n->args = (void *)"sensor-sample";

    if (!ring_push(&ring, n)) {
        /* ring full: drop new sample (policy choice) */
        /* return node to pool for this example */
        isr_nodes[isr_nodes_top++] = n;
    }
}

int main(void)
{
    const t_queue_config qcfg = { .policy = POLICY_CONTINUE, .max_retries = 1, .on_error = NULL };

    if (!queue_init(&queue, 16, &qcfg)) {
        fprintf(stderr, "queue_init failed\n");
        return 1;
    }

    queue_register_handler(&queue, EVT_SENSOR_SAMPLE, sensor_handler);

    if (!ring_init(&ring, RING_SIZE)) {
        fprintf(stderr, "ring_init failed\n");
        return 1;
    }

    /* Preallocate NODE_POOL_SIZE nodes up front and push them to the free stack */
    for (size_t i = 0; i < NODE_POOL_SIZE; ++i) {
        t_node *n = new_node(&queue, &(const t_node_config){
            .name = "isr-node",
            .event_type = EVT_SENSOR_SAMPLE,
            .args = NULL,
            .del_for_args = NULL,
            .max_retries = -1
        });
        if (!n) {
            fprintf(stderr, "new_node failed at %zu\n", i);
            return 1;
        }
        isr_nodes[isr_nodes_top++] = n;
    }

    /* Simulate periodic ISR emissions */
    for (int tick = 0; tick < 20; ++tick) {
        /* Simulate ISR firing multiple times between main loop polls */
        simulated_isr_emit();
        simulated_isr_emit();

        /* Main loop: process all available items */
        t_node *n;
        while ((n = ring_pop(&ring)) != NULL) {
            /* Dispatch using queue's run_node (synchronous execution) */
            run_node(&queue, n);
            /* Return node to preallocated pool for reuse */
            isr_nodes[isr_nodes_top++] = n;
        }
    }

    /* Cleanup */
    ring_destroy(&ring);
    queue_destroy(&queue);

    return 0;
}
