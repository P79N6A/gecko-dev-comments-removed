






#ifndef mozilla_Array_h_
#define mozilla_Array_h_

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
      MOZ_ASSUME_UNREACHABLE("indexing into zero-length array");
    }

    const T& operator[](size_t i) const {
      MOZ_ASSUME_UNREACHABLE("indexing into zero-length array");
    }
};

}  

#endif 
