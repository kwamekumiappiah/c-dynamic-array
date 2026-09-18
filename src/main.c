#include <stdio.h>
#include "dynamic_array.h"


int main(void) {
    dynamic_array_t *my_array = create_array(5);
    push_array(my_array, 1);
    push_array(my_array, 2);
    push_array(my_array, 3);
    push_array(my_array, 4);
    push_array(my_array, 5);
    push_array(my_array, 6);
    size_t total_capacity;
    int element = 0;
    get_total_capacity(my_array, &total_capacity);

    size_t size;
    get_arr_size(my_array, &size);
    printf("%zu\n", total_capacity);
    printf("%zu\n", size);
    destroy_array(my_array);

    return 0;
}