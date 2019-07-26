







#ifndef mozilla_Move_h
#define mozilla_Move_h

#include "mozilla/TypeTraits.h"

namespace mozilla {














































































































































































template<typename T>
inline typename RemoveReference<T>::Type&&
Move(T&& a)
{
  return static_cast<typename RemoveReference<T>::Type&&>(a);
}





template<typename T>
inline T&&
Forward(typename RemoveReference<T>::Type& a)
{
  return static_cast<T&&>(a);
}

template<typename T>
inline T&&
Forward(typename RemoveReference<T>::Type&& t)
{
  static_assert(!IsLvalueReference<T>::value,
                "misuse of Forward detected!  try the other overload");
  return static_cast<T&&>(t);
}


template<typename T>
inline void
Swap(T& t, T& u)
{
  T tmp(Move(t));
  t = Move(u);
  u = Move(tmp);
}

} 

#endif 
