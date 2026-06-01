# libqueue

> **Repo:** [github.com/Ertugrul-Pakdamar/libqueue](https://github.com/Ertugrul-Pakdamar/libqueue)

A C11 library providing a **dynamic node queue** and a **lock-free SPSC ring buffer** — designed for deterministic, zero-runtime-malloc environments.

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
│   └── 04_mcu_main_loop.c
├── include/
│   └── libqueue.h              public API  ← start here
├── src/                     library implementation
│   ├── node_ops.c
│   ├── queue_core.c
│   ├── queue_ops.c
│   ├── queue_run.c
│   ├── ring_core.c
│   ├── ring_ops.c
│   └── ring_run.c
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
| `examples/bin/01_sync_queue` | `examples/01_sync_queue.c` | Basic FIFO queue with `queue_run_sync` |
| `examples/bin/02_fail_policy` | `examples/02_fail_policy.c` | `POLICY_CONTINUE`, `POLICY_STOP`, `POLICY_RETRY` side-by-side |
| `examples/bin/03_async_listener` | `examples/03_async_listener.c` | Custom worker thread draining the ring buffer |
| `examples/bin/04_mcu_main_loop` | `examples/04_mcu_main_loop.c` | MCU-style main-loop consumer with ISR-safe producer (recommended for bare-metal/ISR) |

---

## Usage

### Synchronous queue

```c
#include "libqueue.h"

#define EVENT_TASK_RUN 1

int my_task(t_node *node)
{
    const char *name = (const char *)node->args;
    printf("running: %s\n", name);
    return 0;  /* non-zero triggers the queue's fail policy */
}

int main(void)
{
    t_queue queue;
    const t_queue_config cfg = { .policy = POLICY_CONTINUE, .max_retries = 3, .on_error = NULL };

    if (!queue_init(&queue, 16, &cfg))
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
}
```

### Async Worker (producer / consumer)

```c
#define EVENT_PROCESS_TASK 1

queue_register_handler(&queue, EVENT_PROCESS_TASK, process_task);

ring_init(&ring, 8);

/* Start your custom worker thread here */
// osal_task_create(&task, worker_thread_loop, ...);

/* Producer thread — push work from the main thread */
const t_node_config cfg = {
    .name = "task-1",
    .event_type = EVENT_PROCESS_TASK,
    .args = NULL,
    .del_for_args = NULL,
    .max_retries = -1
};
ring_push(&ring, node_new(&queue, &cfg));

/* Wait for all work to drain, then shut down */
while (!ring_is_empty(&ring))
    ;

/* Signal your worker to stop and wait for it */
// running = 0; osal_task_join(&task);

ring_destroy(&ring);
```

> Note: `03_async_listener` example is provided as a demo for platforms that support multithreading (OS/RTOS) showing how to easily construct a thread loop utilizing `ring_run_sync`. For embedded (bare-metal) or ISR-driven environments, prefer a main-loop consumer model or using preallocated nodes with the SPSC ring instead of spawning threads. The core library does not perform dynamic memory allocation in ISRs; it is safe to `ring_push()` only preallocated `t_node` instances from an ISR.

### MCU / Main-loop example (recommended for embedded)

The `04_mcu_main_loop` example demonstrates a canonical embedded pattern:

- Preallocate a pool of `t_node` instances at initialization time.
- Emit events from an ISR (or ISR-simulating function) by calling `ring_push()` with a preallocated node — no locking or dynamic allocation in the ISR.
- Run a simple main-loop that polls `ring_pop()`, dispatches nodes with `node_run()`, and returns nodes to the preallocated pool.

This pattern is recommended for bare-metal and ISR-driven systems because it avoids thread creation, mutexes, and runtime allocation in interrupt context.

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
cp include/libqueue.h /your/project/include/

# Also copy the dependency headers
cp deps/libmem/include/libmem.h  /your/project/include/
cp deps/libosal/include/libosal.h   /your/project/include/

# Link
cc main.c -o app -Iinclude -Llib -lqueue -lmem -losal -lpthread
```

---

## MISRA C:2012 Notes

| Rule | Deviation | Where | Rationale |
|---|---|---|---|
| 21.3 | `malloc` / `free` at init time | `src/queue_core.c`, `src/ring_core.c` | Each is called exactly once (at `queue_init` / `ring_init`) to allocate a fixed backing buffer. No dynamic allocation occurs after initialisation. |
| 21.21 | `<stdatomic.h>`, `<pthread.h>` | `deps/libosal/posix/osal_posix.c` | Release/acquire ordering is required for the lock-free SPSC ring buffer and cannot be achieved with standard C alone. Confined to a single translation unit. |

All other translation units include only MISRA-compliant headers.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

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

`v0.1.0` — defined in `include/libqueue.h`:

```c
#define LIBQUEUE_VERSION_MAJOR 0
#define LIBQUEUE_VERSION_MINOR 1
#define LIBQUEUE_VERSION_PATCH 0
#define LIBQUEUE_VERSION       "0.1.0"
```

### Compile-time version check

```c
#include "include/libqueue.h"

#if LIBQUEUE_VERSION_MAJOR == 0 && LIBQUEUE_VERSION_MINOR >= 1
    /* use a feature added in v0.1 */
#endif
```

### Git tags

Each release is tagged in the repository:

```bash
git tag -a v0.1.0 -m "Initial public release"
git push origin v0.1.0
```

List all available tags: `git tag -l`
