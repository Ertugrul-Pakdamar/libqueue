#include "internal/lq_internal.h"
#include <string.h>

lq_status_t lq_dispatcher_init(lq_dispatcher_t *disp, const lq_dispatcher_config_t *config)
{
    size_t i;

    if (!disp)
        return LQ_ERR_INVALID_PARAM;

    if (config)
    {
        disp->policy = config->policy;
        disp->max_retries = (config->max_retries > 0) ? config->max_retries : 3;
        disp->on_error = config->on_error;
    }
    else
    {
        disp->policy = LQ_POLICY_CONTINUE;
        disp->max_retries = 3;
        disp->on_error = NULL;
    }

    for (i = 0; i < LQ_EVENT_TYPE_MAX; ++i)
        disp->handlers[i] = NULL;

    return LQ_SUCCESS;
}

lq_status_t lq_dispatcher_register(lq_dispatcher_t *disp, lq_event_type_t type, lq_event_handler_t handler)
{
    if (!disp || type <= LQ_EVENT_TYPE_NONE || type >= LQ_EVENT_TYPE_MAX)
        return LQ_ERR_INVALID_PARAM;

    disp->handlers[type] = handler;
    return LQ_SUCCESS;
}

static lq_status_t dispatch_node(lq_dispatcher_t *disp, lq_node_t *node, int32_t *result_out)
{
    if (!disp || !node || !result_out)
        return LQ_ERR_INVALID_PARAM;

    if (node->event_type <= LQ_EVENT_TYPE_NONE || node->event_type >= LQ_EVENT_TYPE_MAX)
        return LQ_ERR_INVALID_PARAM;

    if (!disp->handlers[node->event_type])
        return LQ_ERR_HANDLER_NOT_FOUND;

    *result_out = disp->handlers[node->event_type](node);
    return LQ_SUCCESS;
}

static int32_t handle_node_execution(lq_dispatcher_t *disp, lq_node_t *node, mem_pool_t *pool)
{
    int32_t handler_result = 0;
    lq_status_t sys_result;
    int32_t effective_retries;

    sys_result = dispatch_node(disp, node, &handler_result);
    
    /* If there's a system error (no handler) or handler returns error code */
    if (sys_result != LQ_SUCCESS || handler_result != 0)
    {
        effective_retries = (node->max_retries >= 0) ? node->max_retries : disp->max_retries;

        if (disp->policy == LQ_POLICY_RETRY && node->retry_count < effective_retries)
        {
            node->retry_count++;
            return 1; /* Request caller to re-queue or handle retry */
        }

        if (disp->on_error)
            disp->on_error(node, (sys_result != LQ_SUCCESS) ? (int32_t)sys_result : handler_result);

        if (disp->policy == LQ_POLICY_STOP)
        {
            lq_node_destroy(pool, node);
            return -1; /* Request caller to stop processing */
        }
    }

    /* Success or failed-but-continue */
    lq_node_destroy(pool, node);
    return 0; /* OK */
}

lq_status_t lq_dispatcher_run_queue(lq_dispatcher_t *disp, lq_queue_t *queue, mem_pool_t *pool, size_t max_events)
{
    size_t processed = 0;
    lq_node_t *node;
    int32_t exec_result;

    if (!disp || !queue || !pool)
        return LQ_ERR_INVALID_PARAM;

    while ((max_events == 0 || processed < max_events) && (node = lq_queue_pop(queue)) != NULL)
    {
        exec_result = handle_node_execution(disp, node, pool);
        processed++;

        if (exec_result == 1) /* Retry requested */
        {
            /* Push back to queue */
            lq_queue_push(queue, node);
        }
        else if (exec_result == -1) /* Stop requested */
        {
            break;
        }
    }

    return LQ_SUCCESS;
}

lq_status_t lq_dispatcher_run_ring(lq_dispatcher_t *disp, lq_ring_t *ring, mem_pool_t *pool, size_t max_events)
{
    size_t processed = 0;
    lq_node_t *node;
    int32_t exec_result;

    if (!disp || !ring || !pool)
        return LQ_ERR_INVALID_PARAM;

    while ((max_events == 0 || processed < max_events) && (node = lq_ring_pop(ring)) != NULL)
    {
        exec_result = handle_node_execution(disp, node, pool);
        processed++;

        if (exec_result == 1) /* Retry requested */
        {
            /* Push back to ring */
            if (lq_ring_push(ring, node) != LQ_SUCCESS)
            {
                /* Ring full on retry, drop the node */
                if (disp->on_error)
                    disp->on_error(node, LQ_ERR_QUEUE_FULL);
                lq_node_destroy(pool, node);
            }
        }
        else if (exec_result == -1) /* Stop requested */
        {
            break;
        }
    }

    return LQ_SUCCESS;
}