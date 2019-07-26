







#ifndef mozilla_Move_h
#define mozilla_Move_h

#include "mozilla/TypeTraits.h"

namespace mozilla {



























































































































template<typename T>
class MoveRef
{
    T* pointer;

  public:
    explicit MoveRef(T& t) : pointer(&t) { }
    T& operator*() const { return *pointer; }
    T* operator->() const { return pointer; }
    operator T& () const { return *pointer; }
};

template<typename T>
inline MoveRef<T>
OldMove(T& t)
{
  return MoveRef<T>(t);
}

template<typename T>
inline MoveRef<T>
OldMove(const T& t)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  return MoveRef<T>(const_cast<T&>(t));
}





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
  T tmp(OldMove(t));
  t = OldMove(u);
  u = OldMove(tmp);
}

} 

#endif 
