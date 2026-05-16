#include "queue.h"

void    run_node(t_node *node)
{
    if (node == NULL || node->process == NULL)
        return ;
    node->process(node);
}

void    run_queue_synchronous(t_node *first_node)
{
    if (first_node == NULL || first_node->process == NULL)
        return ;

    while (first_node->next)
    {
        run_node(first_node);
        first_node = first_node->next;
    }
    run_node(first_node);
}
