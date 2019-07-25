



#include "test.c"


__attribute__((section(".manual_ctors"), used))
static void (*ctors[])() = { (void (*)())-1, end_test, test, NULL };

__attribute__((section(".init")))
void _init() {
    void (**func)() = &ctors[sizeof(ctors) / sizeof(void (*)()) - 1];
    while (*(--func) != (void (*)())-1) {
        (*func)();
    }
}
