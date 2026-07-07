# Contributing to libqueue

Thank you for your interest in contributing to `libqueue`. As this framework is designed for safety-critical systems (Aerospace, Automotive, Defense), we maintain strict engineering standards to ensure reliability and determinism.

## Core Mandates

### 1. Zero Dynamic Allocation
Under no circumstances should `malloc`, `free`, `realloc`, or any heap-based allocation functions be used. All components must rely on user-provided static buffers or pre-allocated pools.

### 2. Lock-Free and Mutex-Free
To ensure portability across bare-metal MCUs and safety during Interrupt Service Routines (ISRs), the core logic must remain lock-free.
- Use C11 Atomics (`stdatomic.h`) for concurrency.
- Avoid OS-specific primitives like Mutexes, Semaphores, or Critical Sections.

### 3. MISRA C:2012 Compliance
All code should strive for MISRA C compliance:
- Use fixed-width types from `<stdint.h>` (e.g., `int32_t`, `uint16_t`).
- Avoid dangerous type casting and `void *` arithmetic where possible.
- Use `const` pointers for read-only data.
- Maintain strict data hiding (Opaque pointers) by keeping struct definitions in `src/internal/`.

### 4. Determinism
Algorithms must have a predictable execution time (O(1) preferred). Avoid unbounded loops or complex recursion.

## Coding Style

- **Namespace:** All public symbols must be prefixed with `lq_` (e.g., `lq_node_t`, `lq_queue_push`).
- **Status Codes:** Functions should return `lq_status_t` to report success or specific failure reasons.
- **Formatting:** Adhere to the existing formatting (consistent with the rest of the codebase).
- **Comments:** Document the "why" behind complex lock-free logic.

## Contribution Workflow

1. **Open an Issue:** Discuss significant architectural changes before starting work.
2. **Implement & Test:** Ensure your changes do not break the lock-free guarantees or introduce `stdlib.h` dependencies.
3. **Submit a PR:** Ensure your code is clean, documented, and follows the naming conventions.

*Note: Since this library is intended for mission-critical applications, every pull request will be rigorously reviewed for race conditions and memory safety.*
