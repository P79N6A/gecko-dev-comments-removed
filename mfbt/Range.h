





#ifndef mozilla_Range_h
#define mozilla_Range_h

#include "mozilla/RangedPtr.h"

#include <stddef.h>

namespace mozilla {


template <typename T>
class Range
{
  const RangedPtr<T> mStart;
  const RangedPtr<T> mEnd;

public:
  Range() : mStart(nullptr, 0), mEnd(nullptr, 0) {}
  Range(T* aPtr, size_t aLength)
    : mStart(aPtr, aPtr, aPtr + aLength),
      mEnd(aPtr + aLength, aPtr, aPtr + aLength)
  {}

  RangedPtr<T> start() const { return mStart; }
  RangedPtr<T> end() const { return mEnd; }
  size_t length() const { return mEnd - mStart; }

  T& operator[](size_t aOffset) const { return mStart[aOffset]; }

  explicit operator bool() const { return mStart != nullptr; }
};

} 

#endif 
