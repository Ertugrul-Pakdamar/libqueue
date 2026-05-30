#include "libqueue.h"

void	node_destroy(t_queue *queue, t_node *node)
{
	if (!node)
		return ;
	if (node->args && node->del_for_args)
		(node->del_for_args)(node->args);
	osal_mutex_lock(&queue->pool_lock);
	pool_free(&queue->node_pool, node);
	osal_mutex_unlock(&queue->pool_lock);
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
