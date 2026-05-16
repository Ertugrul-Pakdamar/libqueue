#include <unistd.h>
#include "include/queue.h"

void sum(int a, int b)
{
    printf("%d\n", a + b);
}

void diff(int a, int b)
{
    printf("%d\n", a - b);
}

void print_name(t_node *this)
{
    char **data = (char**)this->args;
    printf("%s\n", this->name);
    printf("%s\n", data[0]);
}

char **get_allocated_array()
{
    char **res = (char**)malloc((sizeof(char*) * 3));
    res[0] = strdup("abc");
    res[1] = strdup("123");
    res[2] = NULL;
    return (res);
}

void clear_double_array(void *array)
{
    void **my_array = (void**)array;
    int i = 0;
    
    while (my_array[i])
        free(my_array[i++]);
    free(my_array);
}

t_node *get_test_queue()
{
    return (
        new_node("test_node_1", NULL, get_allocated_array(), clear_double_array,
        new_node("test_node_2", NULL, get_allocated_array(), clear_double_array,
        new_node("test_node_3", NULL, get_allocated_array(), clear_double_array,
        NULL)))
    );
}

t_node *get_test_queue_2()
{
    return (
        new_node("test_node_1", print_name, get_allocated_array(), clear_double_array,
        new_node("test_node_2", print_name, get_allocated_array(), clear_double_array,
        new_node("test_node_3", print_name, get_allocated_array(), clear_double_array,
        NULL)))
    );
}
