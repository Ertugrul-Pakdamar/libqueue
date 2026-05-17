#include "queue.h"

void	node_destroy(t_queue *queue, t_node *node)
{
	if (!node)
		return ;
	if (node->args && node->del_for_args)
		(node->del_for_args)(node->args);
	pool_free(&queue->node_pool, node);
}

void	clear_queue(t_queue *queue)
{
	t_node	*current;
	t_node	*next;

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
