/**
 * examples/02_fail_policy.c
 *
 * Demonstrates the three fail policies:
 *   POLICY_CONTINUE — skip failing node, keep going
 *   POLICY_STOP     — halt execution on first failure
 *   POLICY_RETRY    — retry up to max_retries times before giving up
 *
 * Build:  make examples
 * Run:    ./examples/bin/02_fail_policy
 */

#include <stdio.h>
#include "libqueue.h"

# define EVENT_OK    1
# define EVENT_FAIL  2
# define EVENT_RETRY 3

/* ---- helpers ------------------------------------------------------------- */

static int always_ok(t_node *self)
{
    printf("  [ok]   %s\n", self->name);
    return (0);
}

static int always_fail(t_node *self)
{
    printf("  [fail] %s\n", self->name);
    return (1);
}

static int fail_then_ok(t_node *self)
{
    /* Succeeds on the third attempt (retry_count is 0-based). */
    if (self->retry_count < 2)
    {
        printf("  [retry %d] %s\n", self->retry_count, self->name);
        return (1);
    }
    printf("  [ok after %d retries] %s\n", self->retry_count, self->name);
    return (0);
}

static void on_error(t_node *self, int code)
{
    printf("  [error callback] node='%s' code=%d\n", self->name, code);
}

static void run_demo(const char *label, t_fail_policy policy, int max_retries)
{
    t_queue queue;
    const t_queue_config cfg = { policy, max_retries, on_error };

    printf("\n=== %s ===\n", label);
    if (!queue_init(&queue, 8, &cfg))
        return;

    queue_register_handler(&queue, EVENT_OK, always_ok);
    queue_register_handler(&queue, EVENT_FAIL, always_fail);
    queue_register_handler(&queue, EVENT_RETRY, fail_then_ok);

    const t_node_config nodes[] = {
        { "node_ok_1",  EVENT_OK,    NULL, NULL, -1 },
        { "node_fail",  EVENT_FAIL,  NULL, NULL, -1 },
        { "node_ok_2",  EVENT_OK,    NULL, NULL, -1 },
        { "node_retry", EVENT_RETRY, NULL, NULL, -1 },
        { "node_ok_3",  EVENT_OK,    NULL, NULL, -1 },
    };

    for (int i = 0; i < 5; i++)
        add_node_to_queue(&queue, new_node(&queue, &nodes[i]));

    run_queue_synchronous(&queue);
    queue_destroy(&queue);
}

/* --------------------------------------------------------------------------- */

int main(void)
{
    run_demo("POLICY_CONTINUE (skip failures, keep running)", POLICY_CONTINUE, 0);
    run_demo("POLICY_STOP (halt on first failure)",           POLICY_STOP,     0);
    run_demo("POLICY_RETRY (retry up to 3 times)",           POLICY_RETRY,    3);
    return (0);
}
