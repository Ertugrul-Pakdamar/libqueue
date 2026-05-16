#include "queue.h"

void	destroy_node(t_node *node)
{
	if (!node)
		return ;
	free(node->name);
	if (node->args && node->del_for_args)
		(node->del_for_args)(node->args);
	free(node);
}

void	clear_queue(t_node **queue)
{
	t_node	*temp;

	if (!queue)
		return ;
	while (*queue)
	{
        temp = (*queue)->next;
		destroy_node(*queue);
		*queue = temp;
	}
	*queue = NULL;
}
