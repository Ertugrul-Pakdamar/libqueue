#include "test.c"
#include "include/queue.h"

int main()
{
    t_node *test_queue = get_test_queue_2();

    run_queue_synchronous(&test_queue);

    return (0);
}
