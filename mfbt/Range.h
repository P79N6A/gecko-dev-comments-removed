





#ifndef mozilla_Range_h
#define mozilla_Range_h

#include "mozilla/NullPtr.h"
#include "mozilla/RangedPtr.h"

#include <stddef.h>

namespace mozilla {


template <typename T>
class Range
{
    const RangedPtr<T> mStart;
    const RangedPtr<T> mEnd;

    typedef void (Range::* ConvertibleToBool)();
    void nonNull() {}

  public:
    Range() : mStart(nullptr, 0), mEnd(nullptr, 0) {}
    Range(T* p, size_t len)
      : mStart(p, p, p + len),
        mEnd(p + len, p, p + len)
    {}

    RangedPtr<T> start() const { return mStart; }
    RangedPtr<T> end() const { return mEnd; }
    size_t length() const { return mEnd - mStart; }

    T& operator[](size_t offset) const {
      return mStart[offset];
    }

    operator ConvertibleToBool() const { return mStart ? &Range::nonNull : 0; }
};

} 

#endif 
