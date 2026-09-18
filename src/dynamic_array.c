#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "dynamic_array.h"

typedef struct dynamic_array_t{
    size_t capacity;
    size_t size;
    int *data;
} dynamic_array_t;


/*
 * Allocates and initializes a new dynamic array structure and its buffer.
 */
dynamic_array_t *create_array(size_t total_capacity) {
    // 🛡️ Guard against invalid initial capacity
    if (total_capacity == 0) {
        return NULL;
    }

    // 📦 Allocate and zero-initialize the outer container struct
    dynamic_array_t *arr = calloc(1, sizeof(dynamic_array_t));
    if (!arr) {
        return NULL;
    }

    // 🔢 Allocate and zero-initialize the internal integer buffer
    arr->data = calloc(total_capacity, sizeof(int));
    if (!arr->data) {
        // 🧹 Clean up the allocated struct metadata if buffer allocation fails
        memset((void *)arr, 0, sizeof(dynamic_array_t));
        free(arr);
        return NULL;
    }

    arr->capacity = total_capacity;
    arr->size = 0;

    return arr;
}

/*
 * Safely wipes, frees, and destroys a dynamic array instance.
 */
int destroy_array(dynamic_array_t *arr) {
    // 🛡️ Guard against NULL pointer dereference
    if (!arr) {
        return 1;
    }

    // 🧼 Zero-wipe and free the internal buffer if it exists
    if (arr->data) {
        memset((void *)arr->data, 0, arr->capacity * sizeof(int));
    }
    free(arr->data);

    // 🧼 Zero-wipe and free the metadata struct container
    memset((void *)arr, 0, sizeof(dynamic_array_t));
    free(arr);

    return 0; // Success status
}