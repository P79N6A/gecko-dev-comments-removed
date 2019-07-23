





































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
copy_string( InputIterator& first, const InputIterator& last, OutputIterator& result )
  {
    typedef nsCharSourceTraits<InputIterator> source_traits;
    typedef nsCharSinkTraits<OutputIterator>  sink_traits;

    while ( first != last )
      {
        PRInt32 count_copied = PRInt32(sink_traits::write(result, source_traits::read(first), source_traits::readable_distance(first, last)));
        NS_ASSERTION(count_copied > 0, "|copy_string| will never terminate");
        source_traits::advance(first, count_copied);
      }

    return result;
  }

template <class InputIterator, class OutputIterator>
OutputIterator&
copy_string_backward( const InputIterator& first, InputIterator& last, OutputIterator& result )
  {
    while ( first != last )
      {
        last.normalize_backward();
        result.normalize_backward();
        PRUint32 lengthToCopy = PRUint32( NS_MIN(last.size_backward(), result.size_backward()) );
        if ( first.fragment().mStart == last.fragment().mStart )
          lengthToCopy = NS_MIN(lengthToCopy, PRUint32(last.get() - first.get()));

        NS_ASSERTION(lengthToCopy, "|copy_string_backward| will never terminate");

#ifdef _MSC_VER
        
        nsCharTraits<OutputIterator::value_type>::move(result.get()-lengthToCopy, last.get()-lengthToCopy, lengthToCopy);
#else
        nsCharTraits<typename OutputIterator::value_type>::move(result.get()-lengthToCopy, last.get()-lengthToCopy, lengthToCopy);
#endif

        last.advance( -PRInt32(lengthToCopy) );
        result.advance( -PRInt32(lengthToCopy) );
      }

    return result;
  }

#endif 
