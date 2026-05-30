#include "libqueue.h"

int     size_of_queue(t_node *node)
{
	int	len;

	len = 0;
	if (!node)
		return (len);

    while (node->next)
	{
		node = node->next;
		len++;
	}

	return (len + 1);
}

t_node	*get_last_node_of_queue(t_node *queue)
{
	if (!queue)
		return (NULL);
	while (queue->next)
		queue = queue->next;
	return (queue);
}
