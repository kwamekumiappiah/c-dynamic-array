#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "dynamic_array.h"

typedef struct dynamic_array_t{
    size_t capacity;
    size_t size;
    int *data;
} dynamic_array_t;



/* ---------------------------------------------------------------------
 * Helper functions
 * ---------------------------------------------------------------------
 */
/*
 * Safely move the array to another memory space and clear the old buffer before freeing it.
 */
const int realloc_array(dynamic_array_t *arr) {
    if (arr->capacity > SIZE_MAX / (2 * sizeof(int))) {
        return 1; // Overflow protection
    }

    // 1. Reallocate memory into a temporary pointer
    int *temp = realloc((void *)arr->data, sizeof(int) * (arr->capacity * 2));
    if (!temp) {
        return 1; // Allocation failed; original data is untouched
    }

    // 2. Save old capacity to track the start of the new memory block
    size_t old_capacity = arr->capacity;

    // 3. Zero-fill ONLY the new block using `temp` (never old arr->data)
    memset((void *)(temp + old_capacity), 0, sizeof(int) * old_capacity);

    // 4. Update array structure safely
    arr->data = temp;
    arr->capacity *= 2;

    return 0;
}



/* ---------------------------------------------------------------------
 * Create and Destroy
 * ---------------------------------------------------------------------
 */
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


/* ---------------------------------------------------------------------
 * Getters
 * ---------------------------------------------------------------------
 */

/*
 * Safely retrieves the total allocated capacity of the array.
 */
int get_total_capacity(const dynamic_array_t *arr, size_t *capacity) {
    if (!arr || !capacity) return 1;
    *capacity = arr->capacity;
    return 0;
}


/*
 * Safely retrieves an element at a specific zero-based index.
 */
int get_element(const dynamic_array_t *arr, size_t index, int *out_value) {
    if (!arr || !out_value) return 1;
    // Prevent out-of-bounds access and integer overflow (avoids index + 1 wrap-around)
    if (index >= arr->size) return 1;
    *out_value = *(arr->data + index);
    return 0;
}


/*
 * Safely retrieves the current number of elements stored in the array.
 */
int get_arr_size(const dynamic_array_t *arr, size_t *out_value) {
    if (!arr || !out_value) return 1;
    *out_value = arr->size;
    return 0;
}


/* ---------------------------------------------------------------------
 * Mutators
 * ---------------------------------------------------------------------
 */

/**
 * @brief Removes and retrieves the last element from the array.
 *
 * @param[in,out] arr       Pointer to the dynamic array structure.
 * @param[out]    out_value Pointer where the popped integer will be stored.
 *
 * @return int 0 on success; 1 if arr/out_value is NULL or if the array is empty.
 */
int pop_array(dynamic_array_t *arr, int *out_value) {
    if (!arr || !out_value) return 1;
    if (arr->size == 0) return 1;
    *out_value = arr->data[arr->size - 1];
    arr->data[arr->size - 1] = 0;
    arr->size--;
    // We make sure to zero out the memory before freeing it and moving to another memory space
    if (arr->capacity > 10 && arr->size < (arr->capacity / 4)) {
        int *temp = calloc(sizeof(int), (arr->capacity / 2));
        if (!temp) return 0;

        memcpy(temp, arr->data, sizeof(int) * arr->size);
        memset(arr->data, 0, sizeof(int) * arr->size);
        free(arr->data);
        arr->data = temp;
        temp = NULL;
        arr->capacity = arr->capacity / 2; 
    }
    return 0;
}

/*
 * Appends a new integer value to the end of the array, expanding capacity if needed.
 */
int push_array(dynamic_array_t *arr, int data) {

    // 🛡️ Guard against NULL pointer dereference
    if (!arr) {
        return 1;
    }

    // 📈 Expand buffer if capacity limit is reached
    if (arr->size >= arr->capacity) {
        if (realloc_array(arr) == 1) return 1;
    }

    // 📥 Append value and update element count
    arr->data[arr->size] = data;
    arr->size++;

    return 0; // Success status
}

/*
 * Set an element at an index to a given value.
 */
int set_at(dynamic_array_t *arr, size_t index, int value) {
    if (!arr) return 1;
    if (index + 1 > arr->size) return 1;
    arr->data[index] = value;
    return 0;
}


/*
 * Shift the array by 1 and insert at a specified index
 */
int insert_at(dynamic_array_t *arr, size_t index, int value) {
    if (!arr) return 1;
    if ((index) == arr->size) return push_array(arr, value); // Add push element if attempting to insert at the end of the array
    if ((index + 1) > arr->size) return 1;
    if (arr->size >= arr->capacity) {
        if (realloc_array(arr) == 1) return 1;
    }
    // Shift the array by one to make space for the new value
    memmove(arr->data + index + 1, arr->data + index, sizeof(int) * (arr->size - (index)));
    arr->data[index] = value;
    arr->size++;
    return 0;
}

/* ---------------------------------------------------------------------
 * MISC
 * ---------------------------------------------------------------------
 */

/*
 * Check if the array contains an element and send an apropriate response.
 */
int contains(dynamic_array_t *arr, int data) {
    if (!arr || !arr->data) return 0;
    for (size_t i = 0; i < arr->size; i++) {
        if ((arr->data[i]) == data) return 1;
    }
    return 0;
}

