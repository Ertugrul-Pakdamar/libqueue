#include "queue.h"

void    add_node_to_queue(t_node **queue, t_node *node)
{
	t_node	*temp;

	temp = *queue;
	if (!temp)
	{
		*queue = node;
		return ;
	}
	while (temp->next)
		temp = temp->next;
	temp->next = node;
}
