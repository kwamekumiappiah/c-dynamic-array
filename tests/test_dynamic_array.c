/*
 * test_dynamic_array.c
 *
 * Test suite for dynamic_array.c / dynamic_array.h  (Windows/MinGW + Linux/macOS)
 *
 * DESIGN NOTES
 *  - White-box: this file #includes dynamic_array.c directly so it can look at
 *    the struct internals (capacity, data) and verify invariants such as
 *    "every slot in [size, capacity) is zero". Therefore:
 *        * compile ONLY this file (do NOT also pass dynamic_array.c to gcc)
 *        * pass -Iinclude so dynamic_array.h is found (source found via ../src/)
 *  - Crash isolation: the runner re-launches this same executable once per
 *    test ("test.exe --run N"). If one test crashes, the rest still run.
 *    Works on Windows and POSIX (no fork needed).
 *
 * BUILD (Windows, MSYS2 UCRT64, from the project root):
 *    gcc -std=c11 -Wall -Wextra -g tests\test_dynamic_array.c -Iinclude -Isrc -o build\test.exe
 *    .\build\test.exe
 *
 * BUILD (Linux/macOS, optionally with sanitizers):
 *    gcc -std=c11 -Wall -Wextra -g -fsanitize=address,undefined \
 *        tests/test_dynamic_array.c -Iinclude -o build/test
 *    ASAN_OPTIONS=allocator_may_return_null=1 ./build/test
 */

#define __USE_MINGW_ANSI_STDIO 1   /* proper %zu on MinGW */
#include <stdint.h>   /* SIZE_MAX -- dynamic_array.c uses it but doesn't include it */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif

/* White-box: pull in the implementation so we can see the struct definition.
 * Looks in the -I paths first, then falls back to ../src/ relative to this file. */
#if defined(__has_include)
#  if __has_include("dynamic_array.c")
#    include "dynamic_array.c"
#  else
#    include "../src/dynamic_array.c"
#  endif
#else
#  include "../src/dynamic_array.c"
#endif

/* ------------------------------------------------------------------ */
/* Tiny test framework                                                */
/* ------------------------------------------------------------------ */
static int g_fail = 0;   /* failed checks inside the current test (child) */

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            printf("      FAIL  line %d: %s\n", __LINE__, #cond);            \
            g_fail++;                                                        \
        }                                                                    \
    } while (0)

#define CHECK_EQ_SZ(actual, expected)                                        \
    do {                                                                     \
        size_t a_ = (size_t)(actual), e_ = (size_t)(expected);               \
        if (a_ != e_) {                                                      \
            printf("      FAIL  line %d: %s == %zu (got %zu)\n",             \
                   __LINE__, #actual, e_, a_);                               \
            g_fail++;                                                        \
        }                                                                    \
    } while (0)

#define CHECK_EQ_INT(actual, expected)                                       \
    do {                                                                     \
        int a_ = (int)(actual), e_ = (int)(expected);                        \
        if (a_ != e_) {                                                      \
            printf("      FAIL  line %d: %s == %d (got %d)\n",               \
                   __LINE__, #actual, e_, a_);                               \
            g_fail++;                                                        \
        }                                                                    \
    } while (0)

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */
static size_t sz(const dynamic_array_t *a) { size_t s = 999999; get_arr_size(a, &s); return s; }
static size_t cap(const dynamic_array_t *a) { size_t c = 999999; get_total_capacity(a, &c); return c; }
static int    at(const dynamic_array_t *a, size_t i) { int v = -777777; get_element(a, i, &v); return v; }

/* Checks structural invariants (white-box). Call after mutating operations. */
static void check_invariants(const dynamic_array_t *a) {
    CHECK(a != NULL);
    if (!a) return;
    CHECK(a->data != NULL);
    CHECK(a->capacity >= 1);
    CHECK(a->size <= a->capacity);
    /* Security invariant promised by the implementation: unused slots are zero */
    for (size_t i = a->size; i < a->capacity; i++) {
        if (a->data[i] != 0) {
            printf("      FAIL  invariant: data[%zu]=%d beyond size=%zu (cap=%zu) is not zero\n",
                   i, a->data[i], a->size, a->capacity);
            g_fail++;
            break;
        }
    }
}

/* Fill array with 0..n-1 */
static dynamic_array_t *make_filled(size_t initial_cap, int n) {
    dynamic_array_t *a = create_array(initial_cap);
    for (int i = 0; i < n; i++) push_array(a, i);
    return a;
}

/* Verify array contains exactly expected[0..n-1] */
static void expect_contents(const dynamic_array_t *a, const int *expected, size_t n) {
    CHECK_EQ_SZ(sz(a), n);
    for (size_t i = 0; i < n; i++) {
        int v = -999999;
        int rc = get_element(a, i, &v);
        if (rc != 0 || v != expected[i]) {
            printf("      FAIL  contents[%zu]: expected %d, got %d (rc=%d)\n", i, expected[i], v, rc);
            g_fail++;
            return;
        }
    }
}

/* ================================================================== */
/* create_array / destroy_array                                       */
/* ================================================================== */
static void t_create_zero_capacity_returns_null(void) {
    CHECK(create_array(0) == NULL);
}

static void t_create_capacity_one(void) {
    dynamic_array_t *a = create_array(1);
    CHECK(a != NULL);
    CHECK_EQ_SZ(cap(a), 1);
    CHECK_EQ_SZ(sz(a), 0);
    check_invariants(a);
    destroy_array(a);
}

static void t_create_typical_capacity_initialised(void) {
    dynamic_array_t *a = create_array(64);
    CHECK(a != NULL);
    CHECK_EQ_SZ(cap(a), 64);
    CHECK_EQ_SZ(sz(a), 0);
    check_invariants(a);           /* all 64 slots zero */
    destroy_array(a);
}

static void t_create_huge_capacity_fails_cleanly(void) {
    /* SIZE_MAX ints can never be allocated: must return NULL, not crash */
    CHECK(create_array(SIZE_MAX) == NULL);
}

static void t_create_capacity_overflow_multiplication(void) {
    /* capacity * sizeof(int) overflows size_t: calloc must reject it */
    CHECK(create_array(SIZE_MAX / sizeof(int) + 1) == NULL);
}

static void t_destroy_null_returns_1(void) {
    CHECK_EQ_INT(destroy_array(NULL), 1);
}

static void t_destroy_valid_returns_0(void) {
    dynamic_array_t *a = make_filled(4, 4);
    CHECK_EQ_INT(destroy_array(a), 0);
}

static void t_destroy_after_growth_and_shrink(void) {
    dynamic_array_t *a = make_filled(2, 100);
    int v;
    while (sz(a) > 0) pop_array(a, &v);
    CHECK_EQ_INT(destroy_array(a), 0);   /* ASan verifies no leak / bad free */
}

/* ================================================================== */
/* Getters                                                            */
/* ================================================================== */
static void t_get_capacity_null_args(void) {
    dynamic_array_t *a = create_array(4);
    size_t c = 123;
    CHECK_EQ_INT(get_total_capacity(NULL, &c), 1);
    CHECK_EQ_INT(get_total_capacity(a, NULL), 1);
    CHECK_EQ_INT(get_total_capacity(NULL, NULL), 1);
    CHECK_EQ_SZ(c, 123);                 /* untouched on failure */
    destroy_array(a);
}

static void t_get_size_null_args(void) {
    dynamic_array_t *a = create_array(4);
    size_t s = 123;
    CHECK_EQ_INT(get_arr_size(NULL, &s), 1);
    CHECK_EQ_INT(get_arr_size(a, NULL), 1);
    CHECK_EQ_INT(get_arr_size(NULL, NULL), 1);
    CHECK_EQ_SZ(s, 123);
    destroy_array(a);
}

static void t_get_element_null_args(void) {
    dynamic_array_t *a = make_filled(4, 2);
    int v = 55;
    CHECK_EQ_INT(get_element(NULL, 0, &v), 1);
    CHECK_EQ_INT(get_element(a, 0, NULL), 1);
    CHECK_EQ_INT(get_element(NULL, 0, NULL), 1);
    CHECK_EQ_INT(v, 55);
    destroy_array(a);
}

static void t_get_element_empty_array(void) {
    dynamic_array_t *a = create_array(4);
    int v = 55;
    CHECK_EQ_INT(get_element(a, 0, &v), 1);
    CHECK_EQ_INT(v, 55);
    destroy_array(a);
}

static void t_get_element_boundaries(void) {
    dynamic_array_t *a = make_filled(8, 5);   /* size 5, capacity 8 */
    int v = 55;
    CHECK_EQ_INT(get_element(a, 0, &v), 0);  CHECK_EQ_INT(v, 0);
    CHECK_EQ_INT(get_element(a, 4, &v), 0);  CHECK_EQ_INT(v, 4);
    v = 55;
    CHECK_EQ_INT(get_element(a, 5, &v), 1);  /* == size */
    CHECK_EQ_INT(get_element(a, 7, &v), 1);  /* < capacity but >= size */
    CHECK_EQ_INT(get_element(a, 8, &v), 1);  /* == capacity */
    CHECK_EQ_INT(get_element(a, SIZE_MAX, &v), 1);
    CHECK_EQ_INT(get_element(a, SIZE_MAX - 1, &v), 1);
    CHECK_EQ_INT(v, 55);
    destroy_array(a);
}

/* ================================================================== */
/* push_array                                                         */
/* ================================================================== */
static void t_push_null(void) {
    CHECK_EQ_INT(push_array(NULL, 1), 1);
}

static void t_push_within_capacity_no_growth(void) {
    dynamic_array_t *a = create_array(4);
    for (int i = 0; i < 4; i++) CHECK_EQ_INT(push_array(a, i * 10), 0);
    CHECK_EQ_SZ(sz(a), 4);
    CHECK_EQ_SZ(cap(a), 4);              /* exactly full, not yet grown */
    check_invariants(a);
    destroy_array(a);
}

static void t_push_triggers_doubling(void) {
    dynamic_array_t *a = create_array(4);
    for (int i = 0; i < 5; i++) push_array(a, i);
    CHECK_EQ_SZ(sz(a), 5);
    CHECK_EQ_SZ(cap(a), 8);
    int exp[] = {0, 1, 2, 3, 4};
    expect_contents(a, exp, 5);
    check_invariants(a);                 /* new half of buffer must be zeroed */
    destroy_array(a);
}

static void t_push_capacity_one_grows(void) {
    dynamic_array_t *a = create_array(1);
    push_array(a, 10);  CHECK_EQ_SZ(cap(a), 1);
    push_array(a, 20);  CHECK_EQ_SZ(cap(a), 2);
    push_array(a, 30);  CHECK_EQ_SZ(cap(a), 4);
    push_array(a, 40);  CHECK_EQ_SZ(cap(a), 4);
    push_array(a, 50);  CHECK_EQ_SZ(cap(a), 8);
    int exp[] = {10, 20, 30, 40, 50};
    expect_contents(a, exp, 5);
    check_invariants(a);
    destroy_array(a);
}

static void t_push_extreme_values(void) {
    dynamic_array_t *a = create_array(2);
    int vals[] = {INT_MIN, INT_MAX, 0, -1, 1, INT_MIN, INT_MAX};
    for (size_t i = 0; i < sizeof vals / sizeof *vals; i++) push_array(a, vals[i]);
    expect_contents(a, vals, sizeof vals / sizeof *vals);
    destroy_array(a);
}

static void t_push_many_preserves_all(void) {
    const int N = 100000;
    dynamic_array_t *a = create_array(1);
    for (int i = 0; i < N; i++) CHECK_EQ_INT(push_array(a, i), 0);
    CHECK_EQ_SZ(sz(a), (size_t)N);
    int bad = 0;
    for (int i = 0; i < N; i++) if (at(a, (size_t)i) != i) bad++;
    CHECK_EQ_INT(bad, 0);
    CHECK(cap(a) >= (size_t)N);
    check_invariants(a);
    destroy_array(a);
}

static void t_push_zero_value_is_real_element(void) {
    /* 0 is also the "empty slot" filler: make sure it still counts as an element */
    dynamic_array_t *a = create_array(2);
    push_array(a, 0);
    push_array(a, 0);
    CHECK_EQ_SZ(sz(a), 2);
    CHECK_EQ_INT(contains(a, 0), 1);
    destroy_array(a);
}

/* ================================================================== */
/* pop_array                                                          */
/* ================================================================== */
static void t_pop_null_args(void) {
    dynamic_array_t *a = make_filled(4, 2);
    int v = 55;
    CHECK_EQ_INT(pop_array(NULL, &v), 1);
    CHECK_EQ_INT(pop_array(a, NULL), 1);
    CHECK_EQ_INT(pop_array(NULL, NULL), 1);
    CHECK_EQ_SZ(sz(a), 2);               /* nothing removed */
    CHECK_EQ_INT(v, 55);
    destroy_array(a);
}

static void t_pop_empty(void) {
    dynamic_array_t *a = create_array(4);
    int v = 55;
    CHECK_EQ_INT(pop_array(a, &v), 1);
    CHECK_EQ_INT(v, 55);
    CHECK_EQ_SZ(sz(a), 0);               /* size must NOT underflow */
    destroy_array(a);
}

static void t_pop_lifo_order(void) {
    dynamic_array_t *a = make_filled(4, 5);
    for (int expected = 4; expected >= 0; expected--) {
        int v = -1;
        CHECK_EQ_INT(pop_array(a, &v), 0);
        CHECK_EQ_INT(v, expected);
        check_invariants(a);
    }
    CHECK_EQ_SZ(sz(a), 0);
    int v;
    CHECK_EQ_INT(pop_array(a, &v), 1);
    destroy_array(a);
}

static void t_pop_zeroes_vacated_slot(void) {
    dynamic_array_t *a = create_array(4);
    push_array(a, 1234);
    int v;
    pop_array(a, &v);
    CHECK_EQ_INT(a->data[0], 0);
    destroy_array(a);
}

static void t_pop_small_capacity_never_shrinks(void) {
    /* Shrink guard is capacity > 10 */
    dynamic_array_t *a = make_filled(10, 10);
    int v;
    while (sz(a) > 0) pop_array(a, &v);
    CHECK_EQ_SZ(cap(a), 10);
    destroy_array(a);
}

static void t_pop_shrink_threshold_exact(void) {
    /* cap=16: shrink when size < 4, i.e. when size drops to 3. */
    dynamic_array_t *a = make_filled(16, 16);
    int v;
    while (sz(a) > 4) pop_array(a, &v);       /* size == 4 -> no shrink (4 < 4 false) */
    CHECK_EQ_SZ(cap(a), 16);
    pop_array(a, &v);                         /* size == 3 -> shrink to 8 */
    CHECK_EQ_SZ(sz(a), 3);
    CHECK_EQ_SZ(cap(a), 8);
    int exp[] = {0, 1, 2};
    expect_contents(a, exp, 3);
    check_invariants(a);
    destroy_array(a);
}

static void t_pop_shrink_preserves_data_and_invariants(void) {
    dynamic_array_t *a = make_filled(4, 1000);
    for (int expected = 999; expected >= 0; expected--) {
        int v = -1;
        CHECK_EQ_INT(pop_array(a, &v), 0);
        CHECK_EQ_INT(v, expected);
        CHECK(a->capacity >= a->size);
        check_invariants(a);
        if (g_fail) break;
    }
    CHECK_EQ_SZ(sz(a), 0);
    destroy_array(a);
}

static void t_pop_then_push_after_shrink(void) {
    dynamic_array_t *a = make_filled(4, 200);
    int v;
    while (sz(a) > 5) pop_array(a, &v);
    size_t shrunk = cap(a);
    CHECK(shrunk < 256);
    for (int i = 0; i < 300; i++) CHECK_EQ_INT(push_array(a, 1000 + i), 0);
    CHECK_EQ_SZ(sz(a), 305);
    CHECK_EQ_INT(at(a, 0), 0);
    CHECK_EQ_INT(at(a, 4), 4);
    CHECK_EQ_INT(at(a, 5), 1000);
    CHECK_EQ_INT(at(a, 304), 1299);
    check_invariants(a);
    destroy_array(a);
}

static void t_pop_hysteresis_no_thrash(void) {
    /* Push/pop oscillation right at the boundary must stay correct */
    dynamic_array_t *a = make_filled(4, 64);
    int v;
    for (int round = 0; round < 500; round++) {
        push_array(a, round);
        pop_array(a, &v);
        CHECK_EQ_INT(v, round);
        if (g_fail) break;
    }
    check_invariants(a);
    CHECK_EQ_SZ(sz(a), 64);
    destroy_array(a);
}

/* ================================================================== */
/* set_at                                                             */
/* ================================================================== */
static void t_set_null(void) {
    CHECK_EQ_INT(set_at(NULL, 0, 1), 1);
}

static void t_set_empty_array(void) {
    dynamic_array_t *a = create_array(4);
    CHECK_EQ_INT(set_at(a, 0, 99), 1);
    CHECK_EQ_SZ(sz(a), 0);               /* set_at must not grow the array */
    check_invariants(a);
    destroy_array(a);
}

static void t_set_valid_indices(void) {
    dynamic_array_t *a = make_filled(8, 5);
    CHECK_EQ_INT(set_at(a, 0, -1), 0);
    CHECK_EQ_INT(set_at(a, 2, 42), 0);
    CHECK_EQ_INT(set_at(a, 4, INT_MAX), 0);
    int exp[] = {-1, 1, 42, 3, INT_MAX};
    expect_contents(a, exp, 5);
    destroy_array(a);
}

static void t_set_out_of_bounds(void) {
    dynamic_array_t *a = make_filled(8, 5);
    CHECK_EQ_INT(set_at(a, 5, 99), 1);   /* == size */
    CHECK_EQ_INT(set_at(a, 7, 99), 1);   /* < capacity, >= size */
    CHECK_EQ_INT(set_at(a, 8, 99), 1);   /* == capacity */
    CHECK_EQ_INT(set_at(a, 1000, 99), 1);
    check_invariants(a);                 /* nothing written into spare slots */
    destroy_array(a);
}

/* KNOWN-BUG CANDIDATE: `index + 1 > size` wraps to 0 when index == SIZE_MAX */
static void t_set_index_size_max_overflow(void) {
    dynamic_array_t *a = make_filled(8, 5);
    CHECK_EQ_INT(set_at(a, SIZE_MAX, 99), 1);
    destroy_array(a);
}

/* ================================================================== */
/* insert_at                                                          */
/* ================================================================== */
static void t_insert_null(void) {
    CHECK_EQ_INT(insert_at(NULL, 0, 1), 1);
}

static void t_insert_into_empty_at_zero(void) {
    dynamic_array_t *a = create_array(2);
    CHECK_EQ_INT(insert_at(a, 0, 7), 0);
    int exp[] = {7};
    expect_contents(a, exp, 1);
    destroy_array(a);
}

static void t_insert_into_empty_out_of_bounds(void) {
    dynamic_array_t *a = create_array(2);
    CHECK_EQ_INT(insert_at(a, 1, 7), 1);
    CHECK_EQ_SZ(sz(a), 0);
    destroy_array(a);
}

static void t_insert_at_front(void) {
    dynamic_array_t *a = make_filled(8, 3);   /* 0 1 2 */
    CHECK_EQ_INT(insert_at(a, 0, 99), 0);
    int exp[] = {99, 0, 1, 2};
    expect_contents(a, exp, 4);
    check_invariants(a);
    destroy_array(a);
}

static void t_insert_in_middle(void) {
    dynamic_array_t *a = make_filled(8, 4);   /* 0 1 2 3 */
    CHECK_EQ_INT(insert_at(a, 2, 99), 0);
    int exp[] = {0, 1, 99, 2, 3};
    expect_contents(a, exp, 5);
    check_invariants(a);
    destroy_array(a);
}

static void t_insert_at_end_is_push(void) {
    dynamic_array_t *a = make_filled(8, 3);
    CHECK_EQ_INT(insert_at(a, 3, 99), 0);
    int exp[] = {0, 1, 2, 99};
    expect_contents(a, exp, 4);
    destroy_array(a);
}

static void t_insert_at_end_when_full_grows(void) {
    dynamic_array_t *a = make_filled(3, 3);   /* full */
    CHECK_EQ_INT(insert_at(a, 3, 99), 0);
    CHECK_EQ_SZ(cap(a), 6);
    int exp[] = {0, 1, 2, 99};
    expect_contents(a, exp, 4);
    check_invariants(a);
    destroy_array(a);
}

static void t_insert_at_front_when_full_grows(void) {
    dynamic_array_t *a = make_filled(4, 4);   /* full */
    CHECK_EQ_INT(insert_at(a, 0, 99), 0);
    CHECK_EQ_SZ(cap(a), 8);
    int exp[] = {99, 0, 1, 2, 3};
    expect_contents(a, exp, 5);
    check_invariants(a);
    destroy_array(a);
}

static void t_insert_in_middle_when_full_grows(void) {
    dynamic_array_t *a = make_filled(4, 4);
    CHECK_EQ_INT(insert_at(a, 2, 99), 0);
    int exp[] = {0, 1, 99, 2, 3};
    expect_contents(a, exp, 5);
    check_invariants(a);
    destroy_array(a);
}

static void t_insert_capacity_one(void) {
    dynamic_array_t *a = create_array(1);
    insert_at(a, 0, 1);
    insert_at(a, 0, 2);
    insert_at(a, 1, 3);
    int exp[] = {2, 3, 1};
    expect_contents(a, exp, 3);
    check_invariants(a);
    destroy_array(a);
}

static void t_insert_out_of_bounds(void) {
    dynamic_array_t *a = make_filled(8, 3);
    CHECK_EQ_INT(insert_at(a, 4, 99), 1);     /* size + 1 */
    CHECK_EQ_INT(insert_at(a, 100, 99), 1);
    int exp[] = {0, 1, 2};
    expect_contents(a, exp, 3);               /* untouched */
    check_invariants(a);
    destroy_array(a);
}

/* KNOWN-BUG CANDIDATE: `(index + 1) > size` wraps for index == SIZE_MAX,
 * then memmove gets a gigantic length. */
static void t_insert_index_size_max_overflow(void) {
    dynamic_array_t *a = make_filled(8, 3);
    CHECK_EQ_INT(insert_at(a, SIZE_MAX, 99), 1);
    destroy_array(a);
}

static void t_insert_repeated_front_keeps_order(void) {
    dynamic_array_t *a = create_array(1);
    for (int i = 0; i < 500; i++) CHECK_EQ_INT(insert_at(a, 0, i), 0);
    int bad = 0;
    for (int i = 0; i < 500; i++) if (at(a, (size_t)i) != 499 - i) bad++;
    CHECK_EQ_INT(bad, 0);
    check_invariants(a);
    destroy_array(a);
}

/* ================================================================== */
/* remove_at                                                          */
/* ================================================================== */
static void t_remove_null_args(void) {
    dynamic_array_t *a = make_filled(4, 3);
    int v = 55;
    CHECK_EQ_INT(remove_at(NULL, 0, &v), 1);
    CHECK_EQ_INT(remove_at(a, 0, NULL), 1);
    CHECK_EQ_INT(remove_at(NULL, 0, NULL), 1);
    CHECK_EQ_SZ(sz(a), 3);
    CHECK_EQ_INT(v, 55);
    destroy_array(a);
}

static void t_remove_empty(void) {
    dynamic_array_t *a = create_array(4);
    int v = 55;
    CHECK_EQ_INT(remove_at(a, 0, &v), 1);
    CHECK_EQ_INT(v, 55);
    CHECK_EQ_SZ(sz(a), 0);
    destroy_array(a);
}

static void t_remove_first(void) {
    dynamic_array_t *a = make_filled(8, 4);
    int v = -1;
    CHECK_EQ_INT(remove_at(a, 0, &v), 0);
    CHECK_EQ_INT(v, 0);
    int exp[] = {1, 2, 3};
    expect_contents(a, exp, 3);
    check_invariants(a);
    destroy_array(a);
}

static void t_remove_middle(void) {
    dynamic_array_t *a = make_filled(8, 5);
    int v = -1;
    CHECK_EQ_INT(remove_at(a, 2, &v), 0);
    CHECK_EQ_INT(v, 2);
    int exp[] = {0, 1, 3, 4};
    expect_contents(a, exp, 4);
    check_invariants(a);
    destroy_array(a);
}

static void t_remove_last(void) {
    dynamic_array_t *a = make_filled(8, 4);
    int v = -1;
    CHECK_EQ_INT(remove_at(a, 3, &v), 0);
    CHECK_EQ_INT(v, 3);
    int exp[] = {0, 1, 2};
    expect_contents(a, exp, 3);
    check_invariants(a);
    destroy_array(a);
}

static void t_remove_only_element(void) {
    dynamic_array_t *a = make_filled(4, 1);
    int v = -1;
    CHECK_EQ_INT(remove_at(a, 0, &v), 0);
    CHECK_EQ_INT(v, 0);
    CHECK_EQ_SZ(sz(a), 0);
    check_invariants(a);
    destroy_array(a);
}

static void t_remove_out_of_bounds(void) {
    dynamic_array_t *a = make_filled(8, 3);
    int v = 55;
    CHECK_EQ_INT(remove_at(a, 3, &v), 1);           /* == size */
    CHECK_EQ_INT(remove_at(a, 7, &v), 1);           /* < cap, >= size */
    CHECK_EQ_INT(remove_at(a, 8, &v), 1);
    CHECK_EQ_INT(remove_at(a, SIZE_MAX, &v), 1);
    CHECK_EQ_INT(v, 55);
    CHECK_EQ_SZ(sz(a), 3);
    check_invariants(a);
    destroy_array(a);
}

static void t_remove_triggers_shrink(void) {
    dynamic_array_t *a = make_filled(16, 16);
    int v;
    while (sz(a) > 4) remove_at(a, 0, &v);
    CHECK_EQ_SZ(cap(a), 16);
    CHECK_EQ_INT(remove_at(a, 0, &v), 0);           /* size 3 -> shrink */
    CHECK_EQ_SZ(cap(a), 8);
    int exp[] = {13, 14, 15};
    expect_contents(a, exp, 3);
    check_invariants(a);
    destroy_array(a);
}

static void t_remove_drain_from_front(void) {
    dynamic_array_t *a = make_filled(2, 500);
    for (int i = 0; i < 500; i++) {
        int v = -1;
        CHECK_EQ_INT(remove_at(a, 0, &v), 0);
        CHECK_EQ_INT(v, i);
        check_invariants(a);
        if (g_fail) break;
    }
    CHECK_EQ_SZ(sz(a), 0);
    destroy_array(a);
}

/* ================================================================== */
/* contains                                                           */
/* ================================================================== */
static void t_contains_null(void) {
    CHECK_EQ_INT(contains(NULL, 0), 0);
}

static void t_contains_empty(void) {
    dynamic_array_t *a = create_array(4);
    CHECK_EQ_INT(contains(a, 0), 0);      /* buffer is zeroed, but size==0 */
    CHECK_EQ_INT(contains(a, 5), 0);
    destroy_array(a);
}

static void t_contains_found_and_not_found(void) {
    dynamic_array_t *a = make_filled(8, 5);     /* 0..4 */
    for (int i = 0; i < 5; i++) CHECK_EQ_INT(contains(a, i), 1);
    CHECK_EQ_INT(contains(a, 5), 0);
    CHECK_EQ_INT(contains(a, -1), 0);
    CHECK_EQ_INT(contains(a, INT_MAX), 0);
    destroy_array(a);
}

static void t_contains_ignores_slots_beyond_size(void) {
    /* Spare capacity is full of zeros; 0 must not be "found" unless stored */
    dynamic_array_t *a = create_array(8);
    push_array(a, 5);
    CHECK_EQ_INT(contains(a, 0), 0);
    destroy_array(a);
}

static void t_contains_after_pop_and_remove(void) {
    dynamic_array_t *a = make_filled(8, 5);
    int v;
    pop_array(a, &v);                     /* removes 4 */
    CHECK_EQ_INT(contains(a, 4), 0);
    remove_at(a, 1, &v);                  /* removes 1 */
    CHECK_EQ_INT(contains(a, 1), 0);
    CHECK_EQ_INT(contains(a, 3), 1);
    destroy_array(a);
}

static void t_contains_duplicates(void) {
    dynamic_array_t *a = create_array(4);
    push_array(a, 7); push_array(a, 7); push_array(a, 7);
    CHECK_EQ_INT(contains(a, 7), 1);
    int v;
    pop_array(a, &v); pop_array(a, &v);
    CHECK_EQ_INT(contains(a, 7), 1);      /* one left */
    pop_array(a, &v);
    CHECK_EQ_INT(contains(a, 7), 0);
    destroy_array(a);
}

static void t_contains_extreme_values(void) {
    dynamic_array_t *a = create_array(2);
    push_array(a, INT_MIN);
    push_array(a, INT_MAX);
    CHECK_EQ_INT(contains(a, INT_MIN), 1);
    CHECK_EQ_INT(contains(a, INT_MAX), 1);
    destroy_array(a);
}

/* ================================================================== */
/* Randomised differential test against a plain-array reference model */
/* ================================================================== */
#define MODEL_MAX 4096

static void run_stress(unsigned seed, int ops, size_t start_cap) {
    static int model[MODEL_MAX];
    size_t n = 0;
    srand(seed);
    dynamic_array_t *a = create_array(start_cap);
    CHECK(a != NULL);
    if (!a) return;

    for (int op = 0; op < ops && !g_fail; op++) {
        int r = rand() % 8;
        int val = (rand() % 41) - 20;                 /* small range -> collisions */

        switch (r) {
        case 0: case 1:                               /* push (weighted up) */
            if (n < MODEL_MAX) {
                CHECK_EQ_INT(push_array(a, val), 0);
                model[n++] = val;
            }
            break;
        case 2: {                                     /* pop */
            int v = -12345;
            int rc = pop_array(a, &v);
            if (n == 0) CHECK_EQ_INT(rc, 1);
            else { CHECK_EQ_INT(rc, 0); CHECK_EQ_INT(v, model[n - 1]); n--; }
            break;
        }
        case 3:                                       /* insert (valid or invalid idx) */
            if (n < MODEL_MAX) {
                size_t idx = (size_t)rand() % (n + 3);
                int rc = insert_at(a, idx, val);
                if (idx > n) CHECK_EQ_INT(rc, 1);
                else {
                    CHECK_EQ_INT(rc, 0);
                    memmove(model + idx + 1, model + idx, (n - idx) * sizeof(int));
                    model[idx] = val;
                    n++;
                }
            }
            break;
        case 4: {                                     /* remove */
            size_t idx = (size_t)rand() % (n + 2);
            int v = -12345;
            int rc = remove_at(a, idx, &v);
            if (idx >= n) CHECK_EQ_INT(rc, 1);
            else {
                CHECK_EQ_INT(rc, 0);
                CHECK_EQ_INT(v, model[idx]);
                memmove(model + idx, model + idx + 1, (n - idx - 1) * sizeof(int));
                n--;
            }
            break;
        }
        case 5: {                                     /* set */
            size_t idx = (size_t)rand() % (n + 2);
            int rc = set_at(a, idx, val);
            if (idx >= n) CHECK_EQ_INT(rc, 1);
            else { CHECK_EQ_INT(rc, 0); model[idx] = val; }
            break;
        }
        case 6: {                                     /* get */
            size_t idx = (size_t)rand() % (n + 2);
            int v = -12345;
            int rc = get_element(a, idx, &v);
            if (idx >= n) CHECK_EQ_INT(rc, 1);
            else { CHECK_EQ_INT(rc, 0); CHECK_EQ_INT(v, model[idx]); }
            break;
        }
        case 7: {                                     /* contains */
            int expect = 0;
            for (size_t i = 0; i < n; i++) if (model[i] == val) { expect = 1; break; }
            CHECK_EQ_INT(contains(a, val), expect);
            break;
        }
        }

        CHECK_EQ_SZ(sz(a), n);
        check_invariants(a);
    }

    /* Final full comparison */
    expect_contents(a, model, n);
    destroy_array(a);
}

static void t_stress_seed_1_cap1(void)   { run_stress(1,     20000, 1);  }
static void t_stress_seed_2_cap2(void)   { run_stress(2,     20000, 2);  }
static void t_stress_seed_3_cap16(void)  { run_stress(3,     20000, 16); }
static void t_stress_seed_4_cap11(void)  { run_stress(4,     20000, 11); }   /* just above shrink guard */
static void t_stress_seed_5_cap100(void) { run_stress(5,     20000, 100); }
static void t_stress_seed_6(void)        { run_stress(99999, 20000, 3);  }

/* ================================================================== */
/* Test table & runner                                                */
/* ================================================================== */
typedef struct { const char *name; void (*fn)(void); } test_case;

#define T(fn) { #fn + 2, fn }     /* strip the leading "t_" from the printed name */

static const test_case TESTS[] = {
    /* create / destroy */
    T(t_create_zero_capacity_returns_null),
    T(t_create_capacity_one),
    T(t_create_typical_capacity_initialised),
    T(t_create_huge_capacity_fails_cleanly),
    T(t_create_capacity_overflow_multiplication),
    T(t_destroy_null_returns_1),
    T(t_destroy_valid_returns_0),
    T(t_destroy_after_growth_and_shrink),
    /* getters */
    T(t_get_capacity_null_args),
    T(t_get_size_null_args),
    T(t_get_element_null_args),
    T(t_get_element_empty_array),
    T(t_get_element_boundaries),
    /* push */
    T(t_push_null),
    T(t_push_within_capacity_no_growth),
    T(t_push_triggers_doubling),
    T(t_push_capacity_one_grows),
    T(t_push_extreme_values),
    T(t_push_many_preserves_all),
    T(t_push_zero_value_is_real_element),
    /* pop */
    T(t_pop_null_args),
    T(t_pop_empty),
    T(t_pop_lifo_order),
    T(t_pop_zeroes_vacated_slot),
    T(t_pop_small_capacity_never_shrinks),
    T(t_pop_shrink_threshold_exact),
    T(t_pop_shrink_preserves_data_and_invariants),
    T(t_pop_then_push_after_shrink),
    T(t_pop_hysteresis_no_thrash),
    /* set_at */
    T(t_set_null),
    T(t_set_empty_array),
    T(t_set_valid_indices),
    T(t_set_out_of_bounds),
    T(t_set_index_size_max_overflow),
    /* insert_at */
    T(t_insert_null),
    T(t_insert_into_empty_at_zero),
    T(t_insert_into_empty_out_of_bounds),
    T(t_insert_at_front),
    T(t_insert_in_middle),
    T(t_insert_at_end_is_push),
    T(t_insert_at_end_when_full_grows),
    T(t_insert_at_front_when_full_grows),
    T(t_insert_in_middle_when_full_grows),
    T(t_insert_capacity_one),
    T(t_insert_out_of_bounds),
    T(t_insert_index_size_max_overflow),
    T(t_insert_repeated_front_keeps_order),
    /* remove_at */
    T(t_remove_null_args),
    T(t_remove_empty),
    T(t_remove_first),
    T(t_remove_middle),
    T(t_remove_last),
    T(t_remove_only_element),
    T(t_remove_out_of_bounds),
    T(t_remove_triggers_shrink),
    T(t_remove_drain_from_front),
    /* contains */
    T(t_contains_null),
    T(t_contains_empty),
    T(t_contains_found_and_not_found),
    T(t_contains_ignores_slots_beyond_size),
    T(t_contains_after_pop_and_remove),
    T(t_contains_duplicates),
    T(t_contains_extreme_values),
    /* randomised differential tests */
    T(t_stress_seed_1_cap1),
    T(t_stress_seed_2_cap2),
    T(t_stress_seed_3_cap16),
    T(t_stress_seed_4_cap11),
    T(t_stress_seed_5_cap100),
    T(t_stress_seed_6),
};

/* Runs `cmd`, returns the child's exit code (or -1 if it died from a signal on POSIX). */
static int run_child(const char *exe, size_t idx) {
    char cmd[1024];
#ifdef _WIN32
    /* Extra outer quotes: cmd.exe strips the first and last quote of the command line */
    snprintf(cmd, sizeof cmd, "\"\"%s\" --run %lu\"", exe, (unsigned long)idx);
    return system(cmd);
#else
    snprintf(cmd, sizeof cmd, "\"%s\" --run %lu", exe, (unsigned long)idx);
    int st = system(cmd);
    if (st == -1) return -1;
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    return -1;   /* killed by signal */
#endif
}

int main(int argc, char **argv) {
    const size_t total = sizeof TESTS / sizeof TESTS[0];

    setvbuf(stdout, NULL, _IONBF, 0);

    /* Child mode: run exactly one test, exit 0 = pass, 1 = failed checks */
    if (argc == 3 && strcmp(argv[1], "--run") == 0) {
        size_t i = (size_t)strtoul(argv[2], NULL, 10);
        if (i >= total) return 2;
        g_fail = 0;
        TESTS[i].fn();
        return g_fail ? 1 : 0;
    }

    size_t passed = 0, failed = 0, crashed = 0;
    printf("Running %zu tests (each isolated in its own process)\n\n", total);

    for (size_t i = 0; i < total; i++) {
        int rc = run_child(argv[0], i);
        if (rc == 0) {
            printf("[ ok  ] %s\n", TESTS[i].name);
            passed++;
        } else if (rc == 1) {
            printf("[FAIL ] %s\n", TESTS[i].name);
            failed++;
        } else {
            printf("[CRASH] %s  (exit code %d / 0x%X)\n", TESTS[i].name, rc, (unsigned)rc);
            crashed++;
        }
    }

    printf("\n==== %zu passed, %zu failed, %zu crashed (of %zu) ====\n",
           passed, failed, crashed, total);
    return (failed || crashed) ? 1 : 0;
}