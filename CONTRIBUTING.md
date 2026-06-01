# Contributing to libqueue

Contributions are welcome. Please read this guide before opening a PR.

---

## What can be contributed

- Bug fixes in `src/`
- New fail policies (add a value to `t_fail_policy` in `libqueue.h`, handle it in `queue_run.c` or `ring_run.c`)
- Performance improvements to the lock-free ring buffer (`src/ring_ops.c`, `src/ring_core.c`)
- Additional queue utility operations (`src/queue_ops.c`)
- Priority management enhancements (`src/queue_priority.c`, `src/ring_priority.c`)
- New examples in `examples/`
- Test coverage improvements

---

## Architecture & File Structure

To keep the library highly cohesive, `src/` files follow a strict naming convention indicating their responsibility. If you add new functions, they must be placed in the appropriate file:

- **`*_core.c` (e.g. `queue_core.c`, `ring_core.c`)**: Reserved for initialization, destruction, and memory/pool management (`init`, `destroy`, `drain`). Code here handles the **lifecycle** of the data structures.
- **`*_ops.c` (e.g. `queue_ops.c`, `ring_ops.c`, `node_ops.c`)**: Reserved for pure data manipulation and utility operations (`push`, `pop`, `size`, `is_empty`, `new`). Code here handles the **state** and operates ideally in `O(1)` time without triggering side effects.
- **`*_run.c` (e.g. `queue_run.c`, `ring_run.c`)**: Reserved for execution, event dispatching, and applying retry/fail policies (`run_sync`, `node_run`). Code here handles the **behavior** and executes the user's callbacks.
- **`*_priority.c` (e.g. `queue_priority.c`, `ring_priority.c`)**: Specialized modules for multi-level priority management.

---

## How to contribute

```bash
# 1. Fork the repo on GitHub
# 2. Clone your fork
git clone https://github.com/Ertugrul-Pakdamar/libqueue.git
cd libqueue

# 3. Set up dependencies
git clone https://github.com/Ertugrul-Pakdamar/libmem.git  deps/libmem
git clone https://github.com/Ertugrul-Pakdamar/libosal.git deps/libosal

# 4. Create a feature branch
git checkout -b feature/my-improvement

# 5. Make changes, then verify the build is clean
make re

# 6. Open a pull request against main
```

---

## Code style (Mission Critical Standards)

- **C11 Standard**: Compiled with `-Wall -Wextra -Werror`. Zero warnings allowed.
- **MISRA C:2012**: Adherence to MISRA principles is mandatory.
    - Use fixed-width types (`int32_t`, `uint32_t`, etc.) from `<stdint.h>` exclusively.
    - No bare `int`, `char`, or `long` types.
    - No dynamic memory allocation (`malloc`/`free`) outside of `*_init` functions.
- **Naming**: `t_` prefix for types, `snake_case` for functions and variables.
- **Documentation**: Public API changes must update `include/libqueue.h` with Doxygen `/** @brief */` comments.

---

## Dependency contributions

If you want to contribute to the allocators or the OS abstraction layer, those
are separate repositories:

- Allocators → [Ertugrul-Pakdamar/libmem](https://github.com/Ertugrul-Pakdamar/libmem)
- OS abstraction (new platform ports) → [Ertugrul-Pakdamar/libosal](https://github.com/Ertugrul-Pakdamar/libosal)

---

## Adding an example

Examples live in `examples/` and are built with `make examples`. Each example
is a single self-contained `.c` file that links against `libqueue.a`,
`libmem.a`, and `libosal.a`.

### Naming convention

```
examples/NN_short_name.c     (NN = two-digit number, e.g. 04_ring_backpressure.c)
```

### Checklist

1. The file compiles without warnings under `-Wall -Wextra -Werror`.
2. The top of the file has a doc comment explaining what the example demonstrates.
3. Use fixed-width types in examples to match library standards.
4. `main()` returns `0` on success, non-zero on failure.
5. No `malloc` / `free` in the example itself — use the queue's node pool.
6. Add a row to the **Examples** table in `README.md`.
