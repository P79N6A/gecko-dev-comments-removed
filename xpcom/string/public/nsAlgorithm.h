




#ifndef nsAlgorithm_h___
#define nsAlgorithm_h___

#ifndef nsCharTraits_h___
#include "nsCharTraits.h"
  
#endif

#ifndef nsDebug_h___
#include "nsDebug.h"
  
#endif


template <class T>
inline
T
NS_ROUNDUP( const T& a, const T& b )
  {
    return ((a + (b - 1)) / b) * b;
  }

template <class T>
inline
const T&
XPCOM_MIN( const T& a, const T& b )
  {
    return b < a ? b : a;
  }


template <class T>
inline
const T&
XPCOM_MAX( const T& a, const T& b )
  {
    return a > b ? a : b;
  }

#if defined(_MSC_VER) && (_MSC_VER < 1600)
namespace std {
inline
long long
abs( const long long& a )
{
  return a < 0 ? -a : a;
}
}
#endif

namespace mozilla {

template <class T>
inline
const T&
clamped( const T& a, const T& min, const T& max )
  {
    NS_ABORT_IF_FALSE(max >= min, "clamped(): max must be greater than or equal to min");
    return XPCOM_MIN(XPCOM_MAX(a, min), max);
  }

}

template <class InputIterator, class T>
inline
uint32_t
NS_COUNT( InputIterator& first, const InputIterator& last, const T& value )
  {
    uint32_t result = 0;
    for ( ; first != last; ++first )
      if ( *first == value )
        ++result;
    return result;
  }

template <class InputIterator, class OutputIterator>
inline
OutputIterator&
copy_string( const InputIterator& first, const InputIterator& last, OutputIterator& result )
  {
    typedef nsCharSourceTraits<InputIterator> source_traits;
    typedef nsCharSinkTraits<OutputIterator>  sink_traits;

    sink_traits::write(result, source_traits::read(first), source_traits::readable_distance(first, last));
    return result;
  }

#endif 
