





#ifndef mozilla_BinarySearch_h
#define mozilla_BinarySearch_h

#include "mozilla/Assertions.h"

#include <stddef.h>

namespace mozilla {



















































template<typename Container, typename Comparator>
bool
BinarySearchIf(const Container& aContainer, size_t aBegin, size_t aEnd,
               const Comparator& aCompare, size_t* aMatchOrInsertionPoint)
{
  MOZ_ASSERT(aBegin <= aEnd);

  size_t low = aBegin;
  size_t high = aEnd;
  while (high != low) {
    size_t middle = low + (high - low) / 2;

    
    
    const int result = aCompare(aContainer[middle]);

    if (result == 0) {
      *aMatchOrInsertionPoint = middle;
      return true;
    }

    if (result < 0) {
      high = middle;
    } else {
      low = middle + 1;
    }
  }

  *aMatchOrInsertionPoint = low;
  return false;
}

namespace detail {

template<class T>
class BinarySearchDefaultComparator
{
public:
  BinarySearchDefaultComparator(const T& aTarget)
    : mTarget(aTarget)
  {}

  template <class U>
  int operator()(const U& val) const {
    if (mTarget == val) {
      return 0;
    }

    if (mTarget < val) {
      return -1;
    }

    return 1;
  }

private:
  const T& mTarget;
};

} 

template <typename Container, typename T>
bool
BinarySearch(const Container& aContainer, size_t aBegin, size_t aEnd,
             T aTarget, size_t* aMatchOrInsertionPoint)
{
  return BinarySearchIf(aContainer, aBegin, aEnd,
                        detail::BinarySearchDefaultComparator<T>(aTarget),
                        aMatchOrInsertionPoint);
}

} 

#endif 
