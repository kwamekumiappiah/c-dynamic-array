# c-dynamic-array

A custom **dynamic array of `int`s in C**, built from scratch to strengthen my skills in dynamic memory management, pointers, and low-level programming. It behaves like a small `std::vector` / Python `list`: it grows and shrinks automatically, and every operation is bounds-checked and reports failure through return codes instead of crashing.

It comes with a **69-test suite** that checks edge cases, integer overflow, and a randomised comparison against a reference model.

---

## Features

- Automatic **growth** (capacity doubles when full) and **shrinking** (capacity halves when the array is mostly empty)
- **Opaque type**: the struct is hidden from users of the library (proper encapsulation in C)
- Consistent API: `0` means success, `1` means failure, and results come back through output parameters
- NULL checks and bounds checks on every public function
- Overflow protection when growing
- No memory leaks on failed allocations (`realloc` goes through a temporary pointer, so a failure never loses the original buffer)
- Vacated slots are zeroed, so everything past `size` is always `0`

## Project structure

```
c-dynamic-array/
├── include/
│   └── dynamic_array.h      # public API (opaque type + documented prototypes)
├── src/
│   ├── dynamic_array.c      # implementation
│   └── main.c               # example usage
├── tests/
│   └── test_dynamic_array.c # test suite
├── build/                   # compiled output
└── README.md
```

## API

| Function | Description | Time |
|---|---|---|
| `create_array(capacity)` | Allocate a new array (returns `NULL` if capacity is 0 or allocation fails) | O(n) |
| `destroy_array(arr)` | Free the array | O(n) |
| `push_array(arr, value)` | Append to the end, growing if needed | O(1) amortised |
| `pop_array(arr, &out)` | Remove and return the last element, shrinking if needed | O(1) amortised |
| `insert_at(arr, i, value)` | Insert at index `i` (0 to `size`), shifting elements right | O(n) |
| `remove_at(arr, i, &out)` | Remove the element at `i`, shifting elements left | O(n) |
| `set_at(arr, i, value)` | Overwrite the element at `i` | O(1) |
| `get_element(arr, i, &out)` | Read the element at `i` | O(1) |
| `get_arr_size(arr, &out)` | Number of stored elements | O(1) |
| `get_total_capacity(arr, &out)` | Allocated capacity | O(1) |
| `contains(arr, value)` | Linear search, returns `1` if found | O(n) |

All functions except `create_array` and `contains` return `0` on success and `1` on failure (NULL argument, out-of-bounds index, empty array, or allocation failure).

### Example

```c
#include <stdio.h>
#include "dynamic_array.h"

int main(void) {
    dynamic_array_t *arr = create_array(4);
    if (!arr) return 1;

    for (int i = 0; i < 10; i++) push_array(arr, i * i);   // grows past 4 automatically

    insert_at(arr, 0, -1);        // [-1, 0, 1, 4, 9, ...]

    int last;
    pop_array(arr, &last);        // last == 81

    printf("contains 16? %d\n", contains(arr, 16));

    destroy_array(arr);
    return 0;
}
```

## Build and run

### Example program

```bash
gcc -std=c11 -Wall -Wextra -g src/main.c src/dynamic_array.c -Iinclude -o build/main
```

### Tests

The test file `#include`s `dynamic_array.c` so it can check internal state (capacity, unused slots). That means you compile **only the test file**, not `dynamic_array.c` as well.

**Windows (MSYS2 / MinGW), from the project root:**

```powershell
gcc -std=c11 -Wall -Wextra -g .\tests\test_dynamic_array.c -Iinclude -o .\build\test.exe
.\build\test.exe
```

**Linux / macOS / WSL, optionally with sanitizers:**

```bash
gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined \
    tests/test_dynamic_array.c -Iinclude -o build/test
ASAN_OPTIONS=allocator_may_return_null=1 ./build/test
```

Each test runs in its own process, so one crash doesn't stop the rest. The runner prints `ok`, `FAIL`, or `CRASH` for every test and a summary at the end.

## Testing

The suite has **69 tests**:

- **Creation and destruction:** capacity 0 and 1, huge capacities, and `capacity * sizeof(int)` overflow
- **Growth and shrinking:** exact doubling and halving thresholds, capacity-1 arrays, and push/pop oscillation at the boundary
- **Edge cases:** empty arrays, `index == size`, `index == capacity`, and `SIZE_MAX`
- **Extreme values:** `INT_MIN`, `INT_MAX`, and stored zeros
- **Invariants after every operation:** `size <= capacity`, and every slot past `size` is zero
- **Randomised differential testing:** 6 seeded runs of 20,000 mixed operations, each compared against a plain-array reference model

## What I learned

**Memory management**
- Using `calloc` and `realloc` correctly, and checking every allocation
- Reallocating through a temporary pointer so a failure can't leak or lose data
- Freeing in the right order and cleaning up after partial failures
- Growing and shrinking a buffer by resizing (allocate, copy, free)

**Data structure design**
- Separating `size` from `capacity`
- Amortised O(1) appends by doubling capacity
- Using different grow and shrink thresholds so the array doesn't resize back and forth
- Using `memmove` (not `memcpy`) for overlapping shifts

**API design**
- Opaque pointers for encapsulation
- Include guards, doc comments, and consistent return codes with output parameters
- Defensive programming: NULL checks, bounds checks, and overflow guards

**Testing and debugging** (the most valuable part)

Writing the test suite found real bugs in my own code:

1. **Integer overflow in bounds checks.** `index + 1 > size` wraps to `0` when `index == SIZE_MAX`, so the check passes and the function writes out of bounds. The fix is to write `index >= size` instead. I had done this correctly in `get_element` but missed it in `set_at` and `insert_at`.
2. **A missing `#include <stdint.h>`** for `SIZE_MAX`, which compiled on one toolchain and not another.
3. **Compiler warnings that matter:** a useless `const` on return types and swapped `calloc` arguments. Compiling with `-Wall -Wextra` catches these.

I also learned that wiping memory with `memset` right before `free` isn't a reliable secure erase. The compiler is allowed to remove it as a dead store, and `realloc` doesn't wipe the old block it releases. The zeroing in this project keeps a simple "unused slots are zero" invariant that helps with debugging, but it is **not** a security guarantee. Secrets would need a non-optimisable wipe such as `explicit_bzero` or `SecureZeroMemory`.

## Possible next steps

- Make the array generic (`void *` with an element size)
- Add a `Makefile` or CMake build
- Swap the shrink path for a plain `realloc` and drop the extra copy
- Add a sorting or binary-search helper
- Follow up with a linked list and a hash table

## Author

**Kwame Kumi Appiah**

- GitHub: [@kwamekumiappiah](https://github.com/kwamekumiappiah)
- LinkedIn: [kwameappiah-kumi-appiah](https://www.linkedin.com/in/kwameappiah-kumi-appiah/)
- Email: [kwameappiahkumi@gmail.com](mailto:kwameappiahkumi@gmail.com)