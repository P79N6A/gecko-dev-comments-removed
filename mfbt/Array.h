







#ifndef mozilla_Array_h
#define mozilla_Array_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

#include <stddef.h>

namespace mozilla {

template<typename T, size_t Length>
class Array
{
  T mArr[Length];

public:
  T& operator[](size_t aIndex)
  {
    MOZ_ASSERT(aIndex < Length);
    return mArr[aIndex];
  }

  const T& operator[](size_t aIndex) const
  {
    MOZ_ASSERT(aIndex < Length);
    return mArr[aIndex];
  }
};

template<typename T>
class Array<T, 0>
{
public:
  T& operator[](size_t aIndex)
  {
    MOZ_CRASH("indexing into zero-length array");
  }

  const T& operator[](size_t aIndex) const
  {
    MOZ_CRASH("indexing into zero-length array");
  }
};

}  

#endif 
