#include <stdio.h>
#include "dynamic_array.h"


int main(void) {
    dynamic_array_t *my_array = create_array(5);
    // push_array(my_array, 1);
    // push_array(my_array, 2);
    // push_array(my_array, 3);
    // push_array(my_array, 4);
    // push_array(my_array, 5);
    // push_array(my_array, 42);
    
    // printf("%d\n", contains(my_array, -1));   
    
    for (int i = 0; i < 30; i++) {
        int result = 0;
        size_t length = 0;
        insert_at(my_array, (size_t)i, i);
        get_element(my_array, i, &result);
        get_total_capacity(my_array, &length);
        printf("Data: %d\nLength: %zu\n\n", result, length);
    }

    printf("\n\n\n\n");

    for (int i = 0; i < 30; i++) {
        int result = 0;
        size_t length = 0;
        pop_array(my_array, &result);
        get_total_capacity(my_array, &length);
        printf("Data: %d\nLength: %zu\n\n", result, length);
    }

    // set_at(my_array, 3, 100);

    // int result = 0;
    // get_element(my_array, 3, &result);
    // printf("%d", result);

    destroy_array(my_array);

    return 0;
}