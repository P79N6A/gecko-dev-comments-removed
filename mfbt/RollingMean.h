







#ifndef mozilla_RollingMean_h_
#define mozilla_RollingMean_h_

#include "mozilla/Assertions.h"
#include "mozilla/TypeTraits.h"
#include "mozilla/Vector.h"

#include <stddef.h>

namespace mozilla {










template<typename T, typename S>
class RollingMean
{
private:
  size_t mInsertIndex;
  size_t mMaxValues;
  Vector<T> mValues;
  S mTotal;

public:
  static_assert(!IsFloatingPoint<T>::value,
                "floating-point types are unsupported due to rounding "
                "errors");

  explicit RollingMean(size_t aMaxValues)
    : mInsertIndex(0),
      mMaxValues(aMaxValues),
      mTotal(0)
  {
    MOZ_ASSERT(aMaxValues > 0);
  }

  RollingMean& operator=(RollingMean&& aOther)
  {
    MOZ_ASSERT(this != &aOther, "self-assignment is forbidden");
    this->~RollingMean();
    new(this) RollingMean(aOther.mMaxValues);
    mInsertIndex = aOther.mInsertIndex;
    mTotal = aOther.mTotal;
    mValues.swap(aOther.mValues);
    return *this;
  }

  


  bool insert(T aValue)
  {
    MOZ_ASSERT(mValues.length() <= mMaxValues);

    if (mValues.length() == mMaxValues) {
      mTotal = mTotal - mValues[mInsertIndex] + aValue;
      mValues[mInsertIndex] = aValue;
    } else {
      if (!mValues.append(aValue)) {
        return false;
      }
      mTotal = mTotal + aValue;
    }

    mInsertIndex = (mInsertIndex + 1) % mMaxValues;
    return true;
  }

  


  T mean()
  {
    MOZ_ASSERT(!empty());
    return T(mTotal / int64_t(mValues.length()));
  }

  bool empty()
  {
    return mValues.empty();
  }

  


  void clear()
  {
    mValues.clear();
    mInsertIndex = 0;
    mTotal = T(0);
  }

  size_t maxValues()
  {
    return mMaxValues;
  }
};

} 

#endif 
