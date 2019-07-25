





































#ifndef nsAlgorithm_h___
#define nsAlgorithm_h___

#ifndef nsCharTraits_h___
#include "nsCharTraits.h"
  
#endif

#ifndef prtypes_h___
#include "prtypes.h"
  
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
NS_MIN( const T& a, const T& b )
  {
    return b < a ? b : a;
  }

template <class T>
inline
const T&
NS_MAX( const T& a, const T& b )
  {
    return a > b ? a : b;
  }

template <class T>
inline
T
NS_ABS( const T& a )
  {
    return a < 0 ? -a : a;
  }

template <class InputIterator, class T>
inline
PRUint32
NS_COUNT( InputIterator& first, const InputIterator& last, const T& value )
  {
    PRUint32 result = 0;
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
