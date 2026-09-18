#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stddef.h>

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

#endif