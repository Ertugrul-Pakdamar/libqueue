#include "queue.h"

t_node  *new_node(const char *name, void (*process)(t_node *), void *args, void (*del_for_args)(void *), t_node *next)
{
	t_node	*new;

	new = (t_node *)malloc(sizeof(t_node));
	if (new == NULL)
		return (NULL);
	new->process = process;
    new->name = strdup(name);
	new->args = args;
	new->del_for_args = del_for_args;
	new->next = next;
	return (new);
}
