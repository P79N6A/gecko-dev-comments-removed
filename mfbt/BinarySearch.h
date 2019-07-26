





#ifndef mozilla_BinarySearch_h
#define mozilla_BinarySearch_h

#include "mozilla/Assertions.h"

#include <stddef.h>

namespace mozilla {


















template <typename Container, typename T>
bool
BinarySearch(const Container &c, size_t begin, size_t end, T target, size_t *matchOrInsertionPoint)
{
  MOZ_ASSERT(begin <= end);

  size_t low = begin;
  size_t high = end;
  while (low != high) {
    size_t middle = low + (high - low) / 2;
    const T &middleValue = c[middle];

    if (target == middleValue) {
      *matchOrInsertionPoint = middle;
      return true;
    }

    if (target < middleValue)
      high = middle;
    else
      low = middle + 1;
  }

  *matchOrInsertionPoint = low;
  return false;
}

} 

#endif 
