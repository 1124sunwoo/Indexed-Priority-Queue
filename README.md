# Indexed Priority Queue

A C++17 indexed min-priority queue built with a binary min-heap and a custom
linear-probing hash table. Tasks have unique string IDs and integer priorities;
smaller numbers indicate higher priority. The index allows a task to be updated
or removed by ID without scanning the heap.

Originally developed by Sunwoo Choi for SFU CMPT 225 Assignment 3. This repository
is a portfolio revision of that implementation. Post-course debugging,
regression tests, build configuration, and documentation were developed with
OpenAI Codex assistance. The original heap-and-hash-table design is retained.

## Build and run

Requirements: a C++17 compiler and GNU Make (Linux, macOS with developer tools,
or Windows through WSL).

```sh
make test       # deterministic regressions and randomized reference-model tests
make demo       # a small task-scheduling example
make sanitize   # AddressSanitizer + UndefinedBehaviorSanitizer; supported compiler required
make clean      # remove generated executables
```

Demo output:

```text
write-report
fix-bug
```

Alternatively, compile directly from the repository root:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -I. tests/test_indpq.cpp -o test_indpq
./test_indpq
```

## Example

```cpp
#include "IndPQ.h"

IndPQ tasks;
tasks.insert("write-report", 3);
tasks.insert("fix-bug", 1);
tasks.updatePriority("write-report", 0);
auto next = tasks.deleteMin(); // "write-report"
```

## API and behavior

| Operation | Behavior |
| --- | --- |
| `insert(id, priority)` | Add a unique task; duplicate IDs throw `std::runtime_error`. |
| `getMin() const` | Return a read-only reference to a minimum-priority task ID. |
| `deleteMin()` | Remove and return a minimum-priority task ID. |
| `updatePriority(id, priority)` | Change a task's priority in either direction. |
| `remove(id)` | Remove a task by ID. |
| `size()` / `isEmpty()` | Query the number of tasks / whether the queue is empty. |
| `clear()` | Remove all tasks, retaining allocated table capacity. |
| `display()` / `ddisplay()` | Print heap contents / heap contents and hash-table indices. |

Empty minimum operations and updates/removals of missing IDs throw
`std::runtime_error`. Negative priorities and empty-string IDs are supported.
Equal-priority tasks have no guaranteed removal order. Copy the result of
`getMin()` if it must survive a queue mutation: mutations can invalidate its
reference. The queue is not thread-safe.

Copy and move operations are deliberately disabled: the nested heap refers to
its owning queue's hash table, and implicit copying would retain a reference to
the wrong object. Independent queue copying is outside this version's scope.

## How it works

- The heap stores `(task ID, priority)` pairs, with a minimum at the root.
- The hash table maps each task ID to its current heap index.
- Every heap swap updates both affected hash-table entries.
- Deletion marks a hash slot as a tombstone so lookups continue through a
  collision chain. Every probe loop is bounded by the table's capacity.
- The table doubles before live load exceeds 70%. It also rebuilds at the same
  capacity when occupied-plus-deleted load would exceed 70%, clearing tombstones.

With expected constant-time hash lookups, insertion, minimum removal, priority
updates, and removal by ID take expected amortized O(log n) time. An individual
insertion can trigger an O(n) table rebuild under ordinary hash distribution.
Adversarial collisions can degrade these bounds. `getMin()`, `size()`, and
`isEmpty()` are O(1). `clear()` is O(n + table capacity). Storage is proportional
to heap storage and hash-table capacity; clearing does not shrink allocations.

## Improvements from the course version

| Original issue | Revision |
| --- | --- |
| Clearing a deleted slot made later colliding keys unreachable. | Tombstones preserve the probe chain. |
| The fixed 100-slot table could probe forever when full. | Dynamic resizing and bounded probing. |
| Mutable `getMin()` allowed task IDs to change without updating the index. | Const access to the minimum ID. |
| Implicit copying could connect a new heap to the original queue's map. | Explicitly disabled copy/move operations. |
| A failed hash insertion could leave an extra heap entry. | Roll back the appended entry if map insertion fails. |
| The Makefile's default target did not match its explicit build rule. | Consistent build, test, demo, and sanitizer targets. |
| The sample program printed a few operations without assertions. | Automated behavioral regressions and an independent reference model. |

## Verification

The test suite checks empty/missing/duplicate errors, extreme integer priorities,
wraparound collision chains, deletion and reinsertion, growth to 5,000 tasks,
priority changes, arbitrary removals, and queue reuse. It also runs 90,000 seeded
randomized operations against a `std::map` reference model, comparing size and
minimum priority after each operation. Ties are checked by priority rather than
assuming a specific ID order. Tests remain active in optimized builds.

GitHub Actions runs the tests, demo, and sanitizer target on pushes and pull
requests. Local passing results do not imply that a hosted Actions run has
already occurred.

## Files

- `IndPQ.h`: header-only queue implementation
- `tests/test_indpq.cpp`: regression and reference-model tests
- `examples/task_scheduler.cpp`: usage example
- `Makefile`: local build and verification commands
- `.github/workflows/ci.yml`: automated GitHub checks

## Upload to GitHub

Extract the archive and use this directory as the repository root. Include the
hidden `.gitignore` and `.github` directory; do not upload the ZIP or `build/`.
Create an empty GitHub repository, then run these commands in this directory,
replacing `YOUR_USERNAME` with your account name:

```sh
git init
git add .
git commit -m "Add indexed priority queue with collision handling and tests"
git branch -M main
git remote add origin https://github.com/YOUR_USERNAME/indexed-priority-queue.git
git push -u origin main
```

Suggested repository description:

> C++ indexed priority queue using a binary min-heap and custom hash table, with dynamic resizing and automated regression tests.

### Local verification for this revision

GCC: `make test demo` passed. AddressSanitizer and UndefinedBehaviorSanitizer
passed with `ASAN_OPTIONS=detect_leaks=0 make sanitize`. LeakSanitizer could not
run in the preparation environment because process inspection was restricted;
leak detection was therefore not verified locally. The default sanitizer target
leaves leak detection enabled for environments that support it.
