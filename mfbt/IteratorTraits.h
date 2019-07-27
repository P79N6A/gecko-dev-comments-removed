







#ifndef mozilla_IteratorTraits_h
#define mozilla_IteratorTraits_h

#include <stddef.h>

namespace mozilla {

template<typename Iterator>
struct IteratorTraits
{
  typedef typename Iterator::ValueType ValueType;
  typedef typename Iterator::DifferenceType DifferenceType;
};

template<typename T>
struct IteratorTraits<T*>
{
  typedef T ValueType;
  typedef ptrdiff_t DifferenceType;
};

template<typename T>
struct IteratorTraits<const T*>
{
  typedef const T ValueType;
  typedef ptrdiff_t DifferenceType;
};

} 

#endif 
