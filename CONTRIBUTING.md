# Contributing to libqueue

← Back to [README](README.md)

Contributions are welcome. Please read this guide before opening a PR.

---

## What can be contributed

- Bug fixes in `src/`
- New fail policies (add a value to `t_fail_policy` in `queue.h`, handle it in `queue_run.c`)
- Performance improvements to the ring buffer (`src/ring_ops.c`)
- Additional queue utility functions (`src/queue_utils.c`)
- Test coverage improvements

---

## How to contribute

```bash
# 1. Fork the repo on GitHub
# 2. Clone your fork
git clone https://github.com/<your-username>/libqueue.git
cd libqueue

# 3. Set up dependencies
git clone https://github.com/Ertugrul-Pakdamar/libmem.git  deps/mem
git clone https://github.com/Ertugrul-Pakdamar/libosal.git deps/osal

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
- Public API changes must update `include/queue.h` with Doxygen `/** @brief */` comments
- No `malloc` / `free` in library code — all memory comes from `libmem` allocators

---

## Dependency contributions

If you want to contribute to the allocators or the OS abstraction layer, those
are separate repositories:

- Allocators → [Ertugrul-Pakdamar/libmem](https://github.com/Ertugrul-Pakdamar/libmem)
- OS abstraction (new platform ports) → [Ertugrul-Pakdamar/libosal](https://github.com/Ertugrul-Pakdamar/libosal)
