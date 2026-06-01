# Contributing to libqueue

Contributions are welcome. Please read this guide before opening a PR.

---

## What can be contributed

- Bug fixes in `src/`
- New fail policies (add a value to `t_fail_policy` in `libqueue.h`, handle it in `queue_run.c` or `ring_run.c`)
- Performance improvements to the lock-free ring buffer (`src/ring_ops.c`, `src/ring_core.c`)
- Additional queue utility operations (`src/queue_ops.c`)
- New examples in `examples/` (see [Adding an example](#adding-an-example) below)
- Test coverage improvements

---

## Architecture & File Structure

To keep the library highly cohesive, `src/` files follow a strict naming convention indicating their responsibility. If you add new functions, they must be placed in the appropriate file:

- **`*_core.c` (e.g. `queue_core.c`, `ring_core.c`)**: Reserved for initialization, destruction, and memory/pool management (`init`, `destroy`, `drain`). Code here handles the **lifecycle** of the data structures.
- **`*_ops.c` (e.g. `queue_ops.c`, `ring_ops.c`, `node_ops.c`)**: Reserved for pure data manipulation and utility operations (`push`, `pop`, `size`, `is_empty`, `new`). Code here handles the **state** and operates ideally in `O(1)` time without triggering side effects.
- **`*_run.c` (e.g. `queue_run.c`, `ring_run.c`)**: Reserved for execution, event dispatching, and applying retry/fail policies (`run_sync`, `node_run`). Code here handles the **behavior** and executes the user's callbacks.

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

## Code style

- C11, compiled with `-Wall -Wextra -Werror` — zero warnings is a hard requirement
- Follow the existing naming convention: `t_` prefix for types, `snake_case` for functions
- Public API changes must update `include/libqueue.h` with Doxygen `/** @brief */` comments
- No `malloc` / `free` in library code — all memory comes from `libmem` allocators

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
2. The top of the file has a doc comment explaining what the example demonstrates
   and how to build and run it.
3. `main()` returns `0` on success, non-zero on failure — so CI can detect broken examples.
4. No `malloc` / `free` in the example itself — use the queue's node pool
   and provide a `del_for_args` destructor if `args` requires cleanup.
5. Add a row to the **Examples** table in `README.md`.

---

## Bumping the Version

When a contribution changes the public API, the version macros in `include/libqueue.h`
must be updated as part of the **same commit**.  
Follow [Semantic Versioning](https://semver.org/):

| What changed | Which macro | Reset |
|---|---|---|
| Bug fix, no API change | `PATCH` | — |
| New public function or type added | `MINOR` | `PATCH → 0` |
| Existing function removed or signature changed | `MAJOR` | `MINOR → 0`, `PATCH → 0` |

> While `MAJOR == 0` (pre-release), breaking changes may be reflected in `MINOR` instead.

### Steps

1. Update the three numeric macros in `include/libqueue.h`.
2. Update `LIBQUEUE_VERSION` string to match.
3. The maintainer will create a git tag after merging (`git tag vX.Y.Z`).
