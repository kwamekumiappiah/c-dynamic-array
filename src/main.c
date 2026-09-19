#include <stdio.h>
#include "dynamic_array.h"


int main(void) {
    dynamic_array_t *my_array = create_array(5);
    push_array(my_array, 1);
    push_array(my_array, 2);
    push_array(my_array, 3);
    push_array(my_array, 4);
    push_array(my_array, 5);
    push_array(my_array, 42);
    
    printf("%d\n", contains(my_array, -1));   
    
    if (contains(my_array, 42)) {
        printf("Found 42!\n");
    }

    set_at(my_array, 3, 100);

    int result = 0;
    get_element(my_array, 3, &result);
    printf("%d", result);

    destroy_array(my_array);

    return 0;
}