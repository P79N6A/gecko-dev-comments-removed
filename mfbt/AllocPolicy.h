










#ifndef mozilla_AllocPolicy_h
#define mozilla_AllocPolicy_h

#include <stddef.h>
#include <stdlib.h>

namespace mozilla {































class MallocAllocPolicy
{
public:
  void* malloc_(size_t aBytes)
  {
    return malloc(aBytes);
  }

  void* calloc_(size_t aBytes)
  {
    return calloc(aBytes, 1);
  }

  void* realloc_(void* aPtr, size_t aOldBytes, size_t aBytes)
  {
    return realloc(aPtr, aBytes);
  }

  void free_(void* aPtr)
  {
    free(aPtr);
  }

  void reportAllocOverflow() const
  {
  }
};

} 

#endif 
