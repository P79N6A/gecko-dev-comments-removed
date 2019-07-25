









#include <stdlib.h>

void* GrMalloc(size_t bytes) {
    void* ptr = ::malloc(bytes);
    if (NULL == ptr) {
        ::exit(-1);
    }
    return ptr;
}

void GrFree(void* ptr) {
    if (ptr) {
        ::free(ptr);
    }
}


