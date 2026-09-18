#include "dynamic_array.h"

int main(void) {
    dynamic_array_t *my_array = create_array(5);
    push_array(my_array, 1);
    push_array(my_array, 2);
    push_array(my_array, 3);
    push_array(my_array, 4);
    push_array(my_array, 5);
    push_array(my_array, 6);
    destroy_array(my_array);
    return 0;
}