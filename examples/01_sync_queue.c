/**
 * examples/01_sync_queue.c
 *
 * Demonstrates the basic synchronous queue workflow:
 *   - queue_init with POLICY_CONTINUE
 *   - enqueue several nodes with simple process() functions
 *   - run_queue_synchronous processes and destroys them in FIFO order
 *   - queue_destroy releases all resources
 *
 * Build:  make examples
 * Run:    ./examples/bin/01_sync_queue
 */

#include <stdio.h>
#include "queue.h"

/* ---- work functions ------------------------------------------------------- */

static int greet(t_node *self)
{
    const char *name = (const char *)self->args;
    printf("[%s] Hello from the queue!\n", name);
    return (0);
}

static void on_error(t_node *self, int code)
{
    fprintf(stderr, "node '%s' failed with code %d\n", self->name, code);
}

/* --------------------------------------------------------------------------- */

int main(void)
{
    t_queue queue;
    const t_queue_config cfg = { POLICY_CONTINUE, 3, on_error };

    if (!queue_init(&queue, 8, &cfg))
    {
        fprintf(stderr, "queue_init failed\n");
        return (1);
    }

    /* Enqueue three nodes.  args is a string literal — no destructor needed. */
    const t_node_config nodes[] = {
        { "alice", greet, (void *)"Alice", NULL, -1 },
        { "bob",   greet, (void *)"Bob",   NULL, -1 },
        { "carol", greet, (void *)"Carol", NULL, -1 },
    };

    for (int i = 0; i < 3; i++)
        add_node_to_queue(&queue, new_node(&queue, &nodes[i]));

    printf("Queue size before run: %d\n", size_of_queue(queue.head));

    run_queue_synchronous(&queue);   /* runs + destroys every node */

    printf("Queue size after run:  %d\n", size_of_queue(queue.head));

    queue_destroy(&queue);
    return (0);
}
