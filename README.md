# libqueue

> **Repo:** [github.com/Ertugrul-Pakdamar/libqueue](https://github.com/Ertugrul-Pakdamar/libqueue)

A C11 library providing a **dynamic node queue** and a **lock-free SPSC ring buffer** — designed for deterministic, zero-runtime-malloc environments. Optimized for mission-critical and defense-industry applications adhering to strict MISRA C:2012 principles.

---

## Features

| Feature | Detail |
|---|---|
| Dynamic queue | Intrusive linked list of work nodes backed by a fixed-size pool |
| Lock-free ring | SPSC ring buffer with cache-line padding, release/acquire ordering |
| Bare-metal native | Build your own async worker or MCU main-loop without OS dependencies |
| Fail policies | `POLICY_CONTINUE`, `POLICY_STOP`, `POLICY_RETRY` |
| Per-node retry | Independent `max_retries` per node; falls back to queue default |
| Zero runtime malloc | All memory allocated once at `init` time — no `malloc` after that |
| Priority Support | O(1) bitmask-based priority selection for both queues and rings |
| MISRA Compliant | Adheres to MISRA C:2012 directives, uses fixed-width types (`int32_t`) |
| OSAL | Platform specifics isolated in a separate library ([libosal][osal-repo]) |

[osal-repo]: https://github.com/Ertugrul-Pakdamar/libosal

---

## Repository Layout

```
libqueue/
├── deps/
│   ├── libmem/              → github.com/Ertugrul-Pakdamar/libmem
│   └── libosal/             → github.com/Ertugrul-Pakdamar/libosal
├── examples/
│   ├── 01_sync_queue.c
│   ├── 02_fail_policy.c
│   ├── 03_async_listener.c
│   ├── 04_mcu_main_loop.c
│   └── 05_priority_ring.c
├── include/
│   └── libqueue.h              public API  ← start here
├── src/                     library implementation
│   ├── node_ops.c
│   ├── queue_core.c
│   ├── queue_ops.c
│   ├── queue_run.c
│   ├── ring_core.c
│   ├── ring_ops.c
│   ├── ring_run.c
│   ├── queue_priority.c
│   └── ring_priority.c
├── LICENSE
├── Makefile
├── README.md
└── CONTRIBUTING.md
```

---

## Dependencies

libqueue depends on two standalone libraries, each in its own repository:

| Library | Repo | Role |
|---|---|---|
| **libmem** | [Ertugrul-Pakdamar/libmem][mem-repo] | Pool / arena / slab allocators |
| **libosal** | [Ertugrul-Pakdamar/libosal][osal-repo] | Mutex, task, and atomic abstractions |

[mem-repo]: https://github.com/Ertugrul-Pakdamar/libmem

These repos live under `deps/` when building libqueue. See [Getting Started](#getting-started).

---

## Getting Started

### 1 — Clone the repo

```bash
git clone https://github.com/Ertugrul-Pakdamar/libqueue.git
cd libqueue
```

### 2 — Get the dependencies

```bash
# Clone libmem into deps/libmem
git clone https://github.com/Ertugrul-Pakdamar/libmem.git deps/libmem

# Clone libosal into deps/libosal
git clone https://github.com/Ertugrul-Pakdamar/libosal.git deps/libosal
```

### 3 — Build

```bash
make all      # builds deps/libmem, deps/libosal, then libqueue.a
```

---

## Usage

### Synchronous queue

```c
#include "libqueue.h"
#include <stdint.h>
#include <stdio.h>

#define EVENT_TASK_RUN 1

int32_t my_task(t_node *node)
{
    const char *name = (const char *)node->args;
    printf("running: %s\n", name);
    return 0;  /* non-zero triggers the queue's fail policy */
}

int main(void)
{
    t_queue queue;
    const t_queue_config cfg = { .policy = POLICY_CONTINUE, .max_retries = 3, .on_error = NULL };

    if (queue_init(&queue, 16, &cfg) == 0)
        return 1;

    queue_register_handler(&queue, EVENT_TASK_RUN, my_task);

    const t_node_config ncfg = {
        .name = "task-1",
        .event_type = EVENT_TASK_RUN,
        .args = (void *)"task-1",
        .del_for_args = NULL,
        .max_retries = -1
    };
    queue_push(&queue, node_new(&queue, &ncfg));

    queue_run_sync(&queue);
    queue_destroy(&queue);
    return 0;
}
```

### Priority Management

libqueue provides multi-level priority support without sacrificing its lock-free or zero-malloc guarantees. Priority is implemented via bitmasking, enabling extremely fast, deterministic `O(1)` checking of pending events.

There are two dedicated priority structures depending on your threading model:
- `t_prio_queue`: Synchronous multi-level priority queue backed by an array of `t_queue`.
- `t_prio_ring`: Asynchronous/ISR-safe multi-level priority ring backed by an array of lock-free `t_ring` buffers.

**Example: Asynchronous Priority Ring**
```c
t_prio_ring pr;
size_t capacities[3] = {16, 16, 16}; /* 3 priority levels */

/* Initialize */
prio_ring_init(&pr, capacities, 3);

/* Push (0 is highest priority) */
prio_ring_push(&pr, 2, low_priority_node);
prio_ring_push(&pr, 0, high_priority_node);

/* Pop always retrieves the highest priority available node first */
t_node *node = prio_ring_pop(&pr); /* Returns high_priority_node */
```

### Fail policies

| Policy | Behaviour |
|---|---|
| `POLICY_CONTINUE` | Invoke `on_error`, continue with next node |
| `POLICY_STOP` | Stop immediately on first failure |
| `POLICY_RETRY` | Retry up to `max_retries` times, then invoke `on_error` |

---

## MISRA C:2012 Compliance

This library is designed for safety-critical systems and adheres to the following MISRA C:2012 principles:

| Rule / Dir | Status | Detail |
|---|---|---|
| **Dir 4.6** | Compliant | Uses fixed-width integers (`int32_t`, `uint32_t`) exclusively. |
| **Rule 21.3** | Deviation | `malloc`/`free` used **only** during initialization to back static pools. |
| **Rule 1.2** | Compliant | Compiler-specific built-ins have standard C fallbacks for maximum portability. |
| **Rule 8.13** | Compliant | Constant pointers used where data is not modified. |

---

## Versioning

This project follows [Semantic Versioning](https://semver.org/) (`MAJOR.MINOR.PATCH`).

| Change | Bump |
|---|---|
| Bug fix, no API change | `PATCH` → `0.1.0 → 0.1.1` |
| New function or feature, existing API unchanged | `MINOR` → `0.1.0 → 0.2.0` |
| Function signature changed, type removed, or struct layout broken | `MAJOR` → `0.x.y → 1.0.0` |

> While `MAJOR == 0` (pre-release), breaking changes may be reflected in `MINOR` instead.  
> The API is not considered stable until `v1.0.0`.

### Current version

`v0.2.0` — defined in `include/libqueue.h`:

```c
#define LIBQUEUE_VERSION_MAJOR 0
#define LIBQUEUE_VERSION_MINOR 2
#define LIBQUEUE_VERSION_PATCH 0
#define LIBQUEUE_VERSION       "0.2.0"
```
