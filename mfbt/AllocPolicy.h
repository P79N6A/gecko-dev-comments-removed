









#ifndef mozilla_AllocPolicy_h_
#define mozilla_AllocPolicy_h_

#include <stddef.h>
#include <stdlib.h>

namespace mozilla {































class MallocAllocPolicy
{
  public:
    void* malloc_(size_t bytes) { return malloc(bytes); }
    void* calloc_(size_t bytes) { return calloc(bytes, 1); }
    void* realloc_(void* p, size_t oldBytes, size_t bytes) { return realloc(p, bytes); }
    void free_(void* p) { free(p); }
    void reportAllocOverflow() const {}
};


} 

#endif 
