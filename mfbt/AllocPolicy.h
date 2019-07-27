










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

  template <typename T>
  T* pod_realloc(T* aPtr, size_t aOldSize, size_t aNewSize)
  {
    if (aNewSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
        return nullptr;
    return static_cast<T*>(realloc(aPtr, aNewSize * sizeof(T)));
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
