#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stddef.h>

// 🔒 Opaque pointer declaration
typedef struct dynamic_array_t dynamic_array_t;

/**
 * Allocates and initializes a new dynamic array.
 * @param total_capacity Initial capacity for elements.
 * @return Pointer to allocated dynamic_array_t, or NULL on failure.
 */
dynamic_array_t *create_array(size_t total_capacity);

/**
 * Safely wipes and frees memory for a dynamic array.
 * @param arr Pointer to the dynamic array to destroy.
 * @return 0 on success, 1 if arr is NULL.
 */
int destroy_array(dynamic_array_t *arr);

/**
 * Appends a new integer value to the end of the array.
 * @param arr Pointer to the dynamic array.
 * @param value Integer to insert.
 * @return 0 on success, 1 on allocation failure or invalid input.
 */
int push_array(dynamic_array_t *arr, int value);

/**
 * Retrieves total allocated capacity of the array.
 * @param arr Pointer to the dynamic array.
 * @param capacity Output pointer for total capacity.
 * @return 0 on success, 1 if any pointer is NULL.
 */
int get_total_capacity(const dynamic_array_t *arr, size_t *capacity);

/**
 * Retrieves the current number of stored elements.
 * @param arr Pointer to the dynamic array.
 * @param out_value Output pointer for current element count.
 * @return 0 on success, 1 if any pointer is NULL.
 */
int get_arr_size(const dynamic_array_t *arr, size_t *out_value);

/**
 * Safely reads an element at the specified index.
 * @param arr Pointer to the dynamic array.
 * @param index Zero-based index of element.
 * @param out_value Output pointer for retrieved value.
 * @return 0 on success, 1 if pointers are NULL or index is out of bounds.
 */
int get_element(const dynamic_array_t *arr, size_t index, int *out_value);

/**
 * Removes and retrieves the last element in the dynamic array.
 * @param arr Pointer to the dynamic array.
 * @param out_value Output pointer for the popped value.
 * @return 0 on success, 1 on invalid pointers or empty array.
 */
int pop_array(dynamic_array_t *arr, int *out_value);

/**
 * Checks if a specific value exists within the dynamic array.
 * @param arr Pointer to the dynamic array.
 * @param data The integer value to search for.
 * @return 1 if found, 0 if not found or if arr is NULL.
 */
int contains(dynamic_array_t *arr, int data);

/**
 * Updates the value at a specific index in the dynamic array.
 * @param arr Pointer to the dynamic array.
 * @param index The zero-based position to update.
 * @param value The new integer value to store.
 * @return 0 on success, 1 on invalid pointer or out-of-bounds index.
 */
int set_at(dynamic_array_t *arr, size_t index, int value);

/**
 * Inserts an integer value at a specific index, shifting existing elements to the right.
 * @param arr Pointer to the dynamic array.
 * @param index The zero-based position where the value should be inserted (0 to arr->size).
 * @param value The integer value to store.
 * @return 0 on success, 1 on invalid pointer, out-of-bounds index, or allocation failure.
 */
int insert_at(dynamic_array_t *arr, size_t index, int value);
#endif