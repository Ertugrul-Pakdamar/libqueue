# libqueue

A high-performance, deterministic, and lock-free event framework for C11 — designed specifically for safety-critical environments such as **Aerospace, Automotive, and Defense**.

`libqueue` provides a clean separation between **Data Transport** (Lock-free MPSC Queues and SPSC Rings) and **Event Execution** (Dispatcher), ensuring bounded execution and zero-malloc operation suitable for bare-metal MCUs and RTOS environments.

---

## Key Architectural Principles

### 1. Zero-Malloc (100% Static Allocation)
In safety-critical systems, heap fragmentation is a forbidden risk. `libqueue` has **zero dependencies on `malloc` or `free`**. All memory buffers must be provided by the user at initialization time (typically as static arrays), ensuring total memory determinism.

### 2. Lock-Free & ISR Safe
Traditional mutexes cause deadlocks and priority inversion in Interrupt Service Routines (ISRs). `libqueue` utilizes **C11 Atomics** (via `libosal`) to implement:
- **Lock-Free MPSC Queue:** Multiple producers (e.g., different ISRs) can push events concurrently without blocking.
- **Lock-Free Node Pool:** Constant-time, thread-safe node allocation via `libmem`.
- **SPSC Ring Buffer:** Ultra-fast, single-producer single-consumer data pipe.

### 3. Clean Architecture (Transport vs. Execution)
Unlike simple queue libraries, `libqueue` separates *how* data moves from *how* it is processed:
- **Transport Layer (`lq_queue`, `lq_ring`):** "Dumb" lock-free pipes that only move node pointers.
- **Execution Layer (`lq_dispatcher`):** A sophisticated engine that handles event registration, retry policies, and bounded execution loops.

---

## Examples

The `examples/` directory contains comprehensive programs demonstrating every feature:

| Example | Feature | Description |
|---|---|---|
| **[01_queue_mpsc.c](examples/01_queue_mpsc.c)** | MPSC Queue | Multiple producers (ISRs/Tasks) pushing to a single queue. |
| **[02_ring_spsc.c](examples/02_ring_spsc.c)** | SPSC Ring | Single-producer single-consumer high-speed data stream. |
| **[03_fail_policy.c](examples/03_fail_policy.c)** | Fail Policies | Using `RETRY` and `STOP` policies with error callbacks. |
| **[04_bounded_execution.c](examples/04_bounded_execution.c)** | Determinism | Processing events in bounded chunks to keep main loop alive. |

### Running the examples

```bash
make examples
./examples/bin/01_queue_mpsc
./examples/bin/02_ring_spsc
```

---

## Getting Started (Quick Look)

```c
#include "libqueue.h"

/* 1. Static Allocation (Opaque-safe macros) */
static LQ_STATIC_NODES(node_buffer, 32);
static mem_pool_t      pool;
static LQ_STATIC_QUEUE(queue_mem);
static LQ_STATIC_DISPATCHER(disp_mem);

#define my_queue ((lq_queue_t*)queue_mem)
#define my_disp  ((lq_dispatcher_t*)disp_mem)

void setup() {
    /* Initialize using libmem pool_init */
    pool_init(&pool, node_buffer, LQ_NODE_SIZE, 32);
    lq_queue_init(my_queue);
    
    lq_dispatcher_init(my_disp, NULL);
}

void loop() {
    /* Bounded execution: process max 5 events per loop tick */
    lq_dispatcher_run_queue(my_disp, my_queue, &pool, 5);
}
```

---

## Detailed Features

### Lock-Free MPSC Queue
Multi-Producer Single-Consumer. Multiple execution contexts (ISRs, threads, or different CPU cores) can push events into the queue simultaneously without using mutexes or disabling interrupts. The single consumer (usually the main loop) processes them in FIFO order.

### Lock-Free SPSC Ring
Single-Producer Single-Consumer. The fastest possible way to move data pointers between two contexts. Uses a circular buffer and atomic indices with acquire/release memory barriers.

### Fail Policies & Retries
- `LQ_POLICY_CONTINUE`: Log the error and move to the next event.
- `LQ_POLICY_STOP`: Halt execution immediately on the first error.
- `LQ_POLICY_RETRY`: Re-queue the failed node and try again up to `max_retries`. Perfect for flaky hardware communications.

---

## Compliance

- **Portable:** Zero dependencies on OS primitives. Uses `libosal` for cross-platform atomics.
- **MISRA C:2012:** Designed to adhere to strict safety guidelines (Fixed-width types, No dynamic allocation, Data hiding).

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
