#include "queue.h"

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

char    **get_node_names(t_node *first_node)
{
    char **queue;
    t_node *scanner;
    int queue_length;

    if (first_node == NULL)
        return (NULL);

    queue_length = size_of_queue(first_node);

    queue = (char**)malloc((sizeof(char*) * queue_length) + sizeof(char*));
    scanner = first_node;
    int i = 0;
    while (i < queue_length)
    {
        queue[i] = strdup(scanner->name);
        scanner = scanner->next;
        i++;
    }
    queue[i] = NULL;

    return (queue);
}
