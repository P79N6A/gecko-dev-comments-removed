










#ifndef mozilla_AllocPolicy_h
#define mozilla_AllocPolicy_h

#include "mozilla/NullPtr.h"
#include "mozilla/TemplateLib.h"

#include <stddef.h>
#include <stdlib.h>

namespace mozilla {































class MallocAllocPolicy
{
public:
  template <typename T>
  T* pod_malloc(size_t aNumElems)
  {
    if (aNumElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
        return nullptr;
    return static_cast<T*>(malloc(aNumElems * sizeof(T)));
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
