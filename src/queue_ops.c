#include "libqueue.h"

void    add_node_to_queue(t_queue *queue, t_node *node)
{
	t_node	*temp;

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
