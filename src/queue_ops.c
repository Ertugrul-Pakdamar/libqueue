#include "libqueue.h"

void    queue_push(t_queue *queue, t_node *node)
{
    t_node  *temp;

    if (!queue || !node)
        return ;
    if (!queue->head)
    {
        queue->head = node;
        return ;
    }
    temp = queue->head;
    while (temp->next)
        temp = temp->next;
    temp->next = node;
}

t_node  *queue_pop(t_queue *queue)
{
    t_node  *node;

    if (!queue || !queue->head)
        return (NULL);
    node = queue->head;
    queue->head = node->next;
    node->next = NULL;
    return (node);
}

void    queue_clear(t_queue *queue)
{
    t_node  *current;
    t_node  *next;

    if (!queue)
        return ;
    current = queue->head;
    while (current)
    {
        next = current->next;
        node_destroy(queue, current);
        current = next;
    }
    queue->head = NULL;
}

int     queue_size(t_queue *queue)
{
    int     len;
    t_node  *node;

    len = 0;
    if (!queue || !queue->head)
        return (len);
    node = queue->head;
    while (node)
    {
        len++;
        node = node->next;
    }
    return (len);
}

int     queue_is_empty(t_queue *queue)
{
    if (!queue || !queue->head)
        return (1);
    return (0);
}

t_node  *queue_tail(t_queue *queue)
{
    t_node  *temp;

    if (!queue || !queue->head)
        return (NULL);
    temp = queue->head;
    while (temp->next)
        temp = temp->next;
    return (temp);
}
