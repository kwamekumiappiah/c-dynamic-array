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
    
    // int element = 0;
    // get_element(my_array, 0, &element);
    // printf("%zu\n", element);


    for (int i = 0; i < 10; i++) {
        int value = 0;
        int error = pop_array(my_array, &value);
        size_t capacity = 0;
        int cap_err = get_total_capacity(my_array, &capacity);

        size_t size = 0;
        int size_err = get_arr_size(my_array, &size);
        printf("Popped value: %d\nCurrent capacity: %zu\nCurrent size: %zu\nPop Status Code: %d\n\n", value, capacity, size, error);
    }
    
    

    destroy_array(my_array);

    return 0;
}