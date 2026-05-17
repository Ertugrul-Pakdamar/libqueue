# libqueue

> **Repo:** [github.com/Ertugrul-Pakdamar/libqueue](https://github.com/Ertugrul-Pakdamar/libqueue)

A C11 library providing a **dynamic node queue**, a **lock-free SPSC ring buffer**,
and an **asynchronous listener** — designed for deterministic, zero-runtime-malloc
environments.

---

## Features

| Feature | Detail |
|---|---|
| Dynamic queue | Intrusive linked list of work nodes backed by a fixed-size pool |
| Lock-free ring | SPSC ring buffer with cache-line padding, release/acquire ordering |
| Async listener | Worker thread draining the ring and executing nodes |
| Fail policies | `POLICY_CONTINUE`, `POLICY_STOP`, `POLICY_RETRY` |
| Per-node retry | Independent `max_retries` per node; falls back to queue default |
| Zero runtime malloc | All memory allocated once at `init` time — no `malloc` after that |
| OSAL | Platform specifics isolated in a separate library ([libosal][osal-repo]) |

[osal-repo]: https://github.com/Ertugrul-Pakdamar/libosal

---

## Repository Layout

```
libqueue/
├── src/
│   ├── queue_alloc.c        queue lifecycle, node creation
│   ├── queue_ops.c          add_node_to_queue
│   ├── queue_clear.c        node_destroy, clear_queue
│   ├── queue_run.c          run_queue_synchronous, retry / policy logic
│   ├── queue_utils.c        size_of_queue, get_last_node_of_queue
│   ├── ring_ops.c           SPSC ring buffer
│   └── listener.c           async worker thread
├── include/
│   └── queue.h              public API  ← start here
├── deps/
│   ├── libmem/              → github.com/Ertugrul-Pakdamar/libmem
│   └── libosal/             → github.com/Ertugrul-Pakdamar/libosal
├── examples/
│   ├── 01_sync_queue.c
│   ├── 02_fail_policy.c
│   └── 03_async_listener.c
├── build/                   generated — not committed
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

### Available targets

```
make all        Build deps + libqueue.a
make examples   Build all programs in examples/
make valgrind   Run examples/01_sync_queue under Valgrind
make clean      Remove object files and example binaries
make fclean     clean + remove libqueue.a
make re         fclean + all
```

---

## Examples

Ready-to-run programs live in `examples/`. Build them all with:

```bash
make examples
```

| Binary | Source | What it shows |
|---|---|---|
| `examples/bin/01_sync_queue` | `examples/01_sync_queue.c` | Basic FIFO queue with `run_queue_synchronous` |
| `examples/bin/02_fail_policy` | `examples/02_fail_policy.c` | `POLICY_CONTINUE`, `POLICY_STOP`, `POLICY_RETRY` side-by-side |
| `examples/bin/03_async_listener` | `examples/03_async_listener.c` | Producer/consumer via ring buffer + listener thread |

---

## Usage

### Synchronous queue

```c
#include "queue.h"

int my_task(t_node *node)
{
    printf("running: %s\n", node->name);
    return 0;  /* non-zero triggers the queue's fail policy */
}

int main(void)
{
    t_queue       queue;
    t_queue_config cfg = { .policy = POLICY_CONTINUE, .max_retries = 3 };

    queue_init(&queue, 16, &cfg);

    t_node_config ncfg = { .name = "task-1", .process = my_task,
                            .args = NULL, .max_retries = -1 };
    add_node_to_queue(&queue, new_node(&queue, &ncfg));

    run_queue_synchronous(&queue);
    queue_destroy(&queue);
}
```

### Async listener (producer / consumer)

```c
t_ring     ring;
t_listener listener;

ring_init(&ring, 8);
listener_start(&listener, &ring, &queue);

/* Producer thread — push work from the main thread */
ring_push(&ring, new_node(&queue, &ncfg));

/* Wait for all work to drain, then shut down */
while (!ring_is_empty(&ring))
    ;
listener_stop(&listener);
ring_destroy(&ring);
```

### Fail policies

| Policy | Behaviour |
|---|---|
| `POLICY_CONTINUE` | Invoke `on_error`, continue with next node |
| `POLICY_STOP` | Stop immediately on first failure |
| `POLICY_RETRY` | Retry up to `max_retries` times, then invoke `on_error` |

---

## Integration into Your Project

```bash
# Copy the static library and the public header
cp libqueue.a      /your/project/lib/
cp include/queue.h /your/project/include/

# Also copy the dependency headers
cp deps/libmem/include/memory.h  /your/project/include/
cp deps/libosal/include/osal.h   /your/project/include/

# Link
cc main.c -o app -Iinclude -Llib -lqueue -lmem -losal -lpthread
```

---

## MISRA C:2012 Notes

| Rule | Deviation | Where | Rationale |
|---|---|---|---|
| 21.3 | `malloc` / `free` at init time | `src/queue_alloc.c`, `src/ring_ops.c` | Each is called exactly once (at `queue_init` / `ring_init`) to allocate a fixed backing buffer. No dynamic allocation occurs after initialisation. |
| 21.21 | `<stdatomic.h>`, `<pthread.h>` | `deps/libosal/posix/osal_posix.c` | Release/acquire ordering is required for the lock-free SPSC ring buffer and cannot be achieved with standard C alone. Confined to a single translation unit. |

All other translation units include only MISRA-compliant headers.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## Dependencies

| Library | Path | Role |
|---|---|---|
| [mem](deps/mem/README.md) | `deps/mem/` | Pool and arena allocators for nodes and ring buffer |
| [osal](deps/osal/README.md) | `deps/osal/` | Mutex, task, and atomic abstractions |

---

## Building

```bash
# Build libmem, libosal, libqueue, and the test binary
make

# Build and run
make run

# Clean build artifacts
make clean

# Remove all generated files including sub-project artifacts
make clean-all
```

Output: `libqueue.a`

---

## Quick Start

```c
#include "queue.h"

/* Work function — return 0 for success, non-zero to trigger fail policy. */
int my_task(t_node *node)
{
    printf("running: %s\n", node->name);
    return 0;
}

int main(void)
{
    /* 1. Allocate backing storage for the node pool. */
    static unsigned char pool_buf[sizeof(t_node) * 16];

    /* 2. Initialize the queue. */
    t_queue queue;
    t_queue_config cfg = { .policy = POLICY_CONTINUE, .max_retries = 3 };
    queue_init(&queue, 16, &cfg);

    /* Supply the pool buffer (pool_init is called inside queue_init). */

    /* 3. Create and enqueue nodes. */
    t_node_config ncfg = { .name = "task-1", .process = my_task,
                            .args = NULL, .max_retries = -1 };
    t_node *n = new_node(&queue, &ncfg);
    add_node_to_queue(&queue, n);

    /* 4. Run all nodes. */
    run_queue_synchronous(&queue);

    /* 5. Destroy the queue. */
    queue_destroy(&queue);
}
```

---

## Async Listener Example

```c
t_ring    ring;
t_listener listener;

ring_init(&ring, 8);
listener_start(&listener, &ring, &queue);

/* Producer — push nodes from the main thread. */
ring_push(&ring, new_node(&queue, &ncfg));

/* Wait until the ring is drained, then stop the worker. */
while (!ring_is_empty(&ring))
    ;
listener_stop(&listener);
ring_destroy(&ring);
```

---

## Fail Policies

| Policy | Behaviour |
|---|---|
| `POLICY_CONTINUE` | Log the error via `on_error`; continue with the next node |
| `POLICY_STOP` | Stop the run immediately after the first failure |
| `POLICY_RETRY` | Retry up to `max_retries` times before calling `on_error` |

---

## MISRA C:2012 Notes

All `<pthread.h>` and `<stdatomic.h>` usage is confined to
`deps/osal/posix/osal_posix.c`. Rule 21.21 deviation is documented in that file.
No other translation unit includes OS or compiler-extension headers.
