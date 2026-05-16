#include "queue.h"

void    run_node(t_node *node)
{
    if (node == NULL || node->process == NULL)
        return ;
    node->process(node);
}

void    run_queue_synchronous(t_node **first_node)
{
    t_node  *current;
    t_node  *next;

    if (first_node == NULL || *first_node == NULL)
        return ;
    current = *first_node;
    while (current)
    {
        next = current->next;
        run_node(current);
        destroy_node(current);
        current = next;
    }
    *first_node = NULL;
}
