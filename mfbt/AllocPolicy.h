










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

  template <typename T>
  T* pod_calloc(size_t aNumElems)
  {
    return static_cast<T*>(calloc(aNumElems, sizeof(T)));
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
