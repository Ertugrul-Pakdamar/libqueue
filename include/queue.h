#ifndef QUEUE_H
# define QUEUE_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

typedef struct s_node
{
    char            *name;
    void            (*process)(struct s_node *);
	void			*args;
	size_t			args_size;
	void			(*del_for_args)(void *);
    struct s_node   *next;
}   t_node;

t_node  *new_node(const char *name, void (*process)(t_node *), void *args, void (*del_for_args)(void *), t_node *next);
void    add_node_to_queue(t_node **queue, t_node *new_node);

char	**get_node_names(t_node *first_node);
int		size_of_queue(t_node *node);
t_node	*get_last_node_of_queue(t_node *queue);

void	destroy_node(t_node *node);
void	clear_queue(t_node **queue);

void	run_node(t_node *node);
void    run_queue_synchronous(t_node **first_node);

#endif