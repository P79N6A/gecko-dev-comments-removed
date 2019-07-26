







#ifndef mozilla_Move_h
#define mozilla_Move_h

#include "mozilla/TypeTraits.h"

namespace mozilla {




































































































































































































template<typename T>
inline typename RemoveReference<T>::Type&&
Move(T&& aX)
{
  return static_cast<typename RemoveReference<T>::Type&&>(aX);
}





template<typename T>
inline T&&
Forward(typename RemoveReference<T>::Type& aX)
{
  return static_cast<T&&>(aX);
}

template<typename T>
inline T&&
Forward(typename RemoveReference<T>::Type&& aX)
{
  static_assert(!IsLvalueReference<T>::value,
                "misuse of Forward detected!  try the other overload");
  return static_cast<T&&>(aX);
}


template<typename T>
inline void
Swap(T& aX, T& aY)
{
  T tmp(Move(aX));
  aX = Move(aY);
  aY = Move(tmp);
}

} 

#endif 
