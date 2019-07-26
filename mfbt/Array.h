







#ifndef mozilla_Array_h
#define mozilla_Array_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include <stddef.h>

namespace mozilla {

template<typename T, size_t Length>
class Array
{
    T arr[Length];

  public:
    T& operator[](size_t i) {
      MOZ_ASSERT(i < Length);
      return arr[i];
    }

    const T& operator[](size_t i) const {
      MOZ_ASSERT(i < Length);
      return arr[i];
    }
};

template<typename T>
class Array<T, 0>
{
  public:
    T& operator[](size_t i) {
      MOZ_CRASH("indexing into zero-length array");
    }

    const T& operator[](size_t i) const {
      MOZ_CRASH("indexing into zero-length array");
    }
};

}  

#endif 
