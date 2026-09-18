#include "dynamic_array.h"

int main(void) {
    dynamic_array_t *my_array = create_array(5);
    destroy_array(my_array);
    return 0;
}