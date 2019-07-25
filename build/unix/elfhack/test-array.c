



#include "test.c"

__attribute__((section(".init_array"), used))
static void (*init_array[])() = { end_test, test };
