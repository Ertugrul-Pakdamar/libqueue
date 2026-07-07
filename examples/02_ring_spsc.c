/**
 * @example 02_ring_spsc.c
 * @brief Demonstrates a Single-Producer Single-Consumer (SPSC) Lock-Free Ring.
 *
 * SPSC Rings are ideal for high-frequency point-to-point data streams,
 * such as UART or ADC data being passed to a processing task.
 */

#include "libqueue.h"
#include <stdio.h>

#define EVENT_DATA_SAMP 2
#define RING_CAPACITY   16
#define POOL_CAPACITY   16

static LQ_STATIC_NODES(node_buffer, POOL_CAPACITY);
static mem_pool_t      pool;
static LQ_STATIC_RING(ring_mem);
static LQ_STATIC_DISPATCHER(disp_mem);

/* SPSC Ring needs an array to store pointers */
static lq_node_t* ring_ptr_buffer[RING_CAPACITY];

#define ring       ((lq_ring_t*)ring_mem)
#define dispatcher ((lq_dispatcher_t*)disp_mem)

int32_t data_handler(lq_node_t *node)
{
    int val = (int)(intptr_t)lq_node_get_args(node);
    printf("Consumer: Processing sample %d\n", val);
    return 0;
}

int main(void)
{
    pool_init(&pool, node_buffer, LQ_NODE_SIZE, POOL_CAPACITY);
    lq_ring_init(ring, ring_ptr_buffer, RING_CAPACITY);
    
    lq_dispatcher_init(dispatcher, NULL);
    lq_dispatcher_register(dispatcher, EVENT_DATA_SAMP, data_handler);

    printf("Producer: Streaming data to ring...\n");
    for (int i = 100; i < 105; ++i)
    {
        lq_node_config_t ncfg = {
            .name = "sample",
            .event_type = EVENT_DATA_SAMP,
            .args = (void *)(intptr_t)i
        };
        
        lq_node_t *node = lq_node_new(&pool, &ncfg);
        if (node)
        {
            lq_ring_push(ring, node);
            printf("Producer: Pushed %d\n", i);
        }
    }

    printf("\nMain Loop: Draining Ring...\n");
    lq_dispatcher_run_ring(dispatcher, ring, &pool, 0);

    return 0;
}
