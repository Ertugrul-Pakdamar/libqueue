#include "queue.h"

void	clear_queue(t_node **queue, void (*del)(void *))
{
	t_node	*temp;

	if (!queue)
		return ;
	while (*queue)
	{
        temp = (*queue)->next;
		delete_node(*queue, (*del));
		*queue = temp;
	}
	*queue = NULL;
}

void	delete_node(t_node *node, void (*del)(void *))
{
	if (!node || !del)
		return ;
	(*del)(node->name);
	(node->del_for_args)(node->args);
	(*del)(node);
}
