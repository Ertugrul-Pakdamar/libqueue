#include "libqueue.h"
#include <stdio.h>
#include <stdlib.h>

#define EVENT_PRINT 1

static int print_handler(t_node *node)
{
    const char *msg = (const char *)node->args;
    printf("Executed: %s\n", msg);
    return 0;
}

int main(void)
{
    t_prio_ring pr;
    size_t capacities[3] = {4, 4, 4}; /* 3 levels of priority */
    t_node *n1, *n2, *n3;
    t_queue queue; /* Needed for node allocation and handler registry */
    
    printf("--- Priority Ring Example (MCU style) ---\n\n");
    
    /* 1. Initialize a queue to act as our memory pool and dispatcher */
    if (!queue_init(&queue, 10, NULL))
    {
        printf("Failed to init queue.\n");
        return 1;
    }
    queue_register_handler(&queue, EVENT_PRINT, print_handler);
    
    /* 2. Initialize priority ring with 3 levels */
    if (!prio_ring_init(&pr, capacities, 3))
    {
        printf("Failed to init priority ring.\n");
        queue_destroy(&queue);
        return 1;
    }

    /* 3. Create nodes */
    t_node_config cfg = { 
        .name = "low", 
        .event_type = EVENT_PRINT, 
        .args = (void *)"Low Priority Event", 
        .max_retries = -1, 
        .del_for_args = NULL 
    };
    n3 = new_node(&queue, &cfg);
    
    cfg.name = "high";
    cfg.args = (void *)"High Priority Event";
    n1 = new_node(&queue, &cfg);
    
    cfg.name = "normal";
    cfg.args = (void *)"Normal Priority Event";
    n2 = new_node(&queue, &cfg);

    /* 4. Push in reverse priority order: Low, then Normal, then High */
    /* Level 0 is Highest Priority, Level 2 is Lowest */
    printf("Pushing Low Priority (Level 2)...\n");
    prio_ring_push(&pr, 2, n3);
    
    printf("Pushing Normal Priority (Level 1)...\n");
    prio_ring_push(&pr, 1, n2);
    
    printf("Pushing High Priority (Level 0)...\n");
    prio_ring_push(&pr, 0, n1);
    
    printf("\nSimulating MCU main loop polling...\n");
    t_node *popped;
    
    /* 5. Pop and execute. It should pop High, then Normal, then Low. */
    while ((popped = prio_ring_pop(&pr)) != NULL)
    {
        run_node(&queue, popped);
        node_destroy(&queue, popped);
    }
    
    printf("\nDone.\n");
    
    /* Cleanup */
    prio_ring_destroy(&pr);
    queue_destroy(&queue);
    
    return 0;
}
