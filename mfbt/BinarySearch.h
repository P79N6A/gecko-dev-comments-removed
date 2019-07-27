





#ifndef mozilla_BinarySearch_h
#define mozilla_BinarySearch_h

#include "mozilla/Assertions.h"

#include <stddef.h>

namespace mozilla {


















template <typename Container, typename T>
bool
BinarySearch(const Container& aContainer, size_t aBegin, size_t aEnd,
             T aTarget, size_t* aMatchOrInsertionPoint)
{
  MOZ_ASSERT(aBegin <= aEnd);

  size_t low = aBegin;
  size_t high = aEnd;
  while (low != high) {
    size_t middle = low + (high - low) / 2;
    const T& middleValue = aContainer[middle];

    MOZ_ASSERT(aContainer[low] <= aContainer[middle]);
    MOZ_ASSERT(aContainer[middle] <= aContainer[high - 1]);
    MOZ_ASSERT(aContainer[low] <= aContainer[high - 1]);

    if (aTarget == middleValue) {
      *aMatchOrInsertionPoint = middle;
      return true;
    }

    if (aTarget < middleValue) {
      high = middle;
    } else {
      low = middle + 1;
    }
  }

  *aMatchOrInsertionPoint = low;
  return false;
}

} 

#endif 
