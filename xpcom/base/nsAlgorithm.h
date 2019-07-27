





#ifndef nsAlgorithm_h___
#define nsAlgorithm_h___

#include "nsCharTraits.h"  

template <class T>
inline T
NS_ROUNDUP(const T& aA, const T& aB)
{
  return ((aA + (aB - 1)) / aB) * aB;
}




template <class T>
inline const T&
XPCOM_MIN(const T& aA, const T& aB)
{
  return aB < aA ? aB : aA;
}


template <class T>
inline const T&
XPCOM_MAX(const T& aA, const T& aB)
{
  return aA > aB ? aA : aB;
}

namespace mozilla {

template <class T>
inline const T&
clamped(const T& aA, const T& aMin, const T& aMax)
{
  MOZ_ASSERT(aMax >= aMin,
             "clamped(): aMax must be greater than or equal to aMin");
  return XPCOM_MIN(XPCOM_MAX(aA, aMin), aMax);
}

}

template <class InputIterator, class T>
inline uint32_t
NS_COUNT(InputIterator& aFirst, const InputIterator& aLast, const T& aValue)
{
  uint32_t result = 0;
  for (; aFirst != aLast; ++aFirst)
    if (*aFirst == aValue) {
      ++result;
    }
  return result;
}

template <class InputIterator, class OutputIterator>
inline OutputIterator&
copy_string(const InputIterator& aFirst, const InputIterator& aLast,
            OutputIterator& aResult)
{
  typedef nsCharSourceTraits<InputIterator> source_traits;
  typedef nsCharSinkTraits<OutputIterator>  sink_traits;

  sink_traits::write(aResult, source_traits::read(aFirst),
                     source_traits::readable_distance(aFirst, aLast));
  return aResult;
}

#endif 
