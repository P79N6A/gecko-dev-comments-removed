













#ifndef mozilla_RangedArray_h
#define mozilla_RangedArray_h

#include "mozilla/Array.h"

namespace mozilla {

template<typename T, size_t MinIndex, size_t Length>
class RangedArray
{
public:
  T& operator[](size_t aIndex)
  {
    MOZ_ASSERT(aIndex == MinIndex || aIndex > MinIndex);
    return mArr[aIndex - MinIndex];
  }

  const T& operator[](size_t aIndex) const
  {
    MOZ_ASSERT(aIndex == MinIndex || aIndex > MinIndex);
    return mArr[aIndex - MinIndex];
  }

private:
  Array<T, Length> mArr;
};

} 

#endif 
