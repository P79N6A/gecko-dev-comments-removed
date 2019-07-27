







#ifndef mozilla_Maybe_h
#define mozilla_Maybe_h

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/Move.h"
#include "mozilla/TypeTraits.h"

#include <new>  

namespace mozilla {

struct Nothing { };



























































template<class T>
class Maybe
{
  typedef void (Maybe::* ConvertibleToBool)(float*****, double*****);
  void nonNull(float*****, double*****) {}

  bool mIsSome;
  AlignedStorage2<T> mStorage;

public:
  typedef T ValueType;

  Maybe() : mIsSome(false) { }
  ~Maybe() { reset(); }

  explicit Maybe(Nothing) : mIsSome(false) { }

  Maybe(const Maybe& aOther)
    : mIsSome(false)
  {
    if (aOther.mIsSome) {
      emplace(*aOther);
    }
  }

  Maybe(Maybe&& aOther)
    : mIsSome(aOther.mIsSome)
  {
    if (aOther.mIsSome) {
      ::new (mStorage.addr()) T(Move(*aOther));
      aOther.reset();
    }
  }

  Maybe& operator=(const Maybe& aOther)
  {
    if (&aOther != this) {
      if (aOther.mIsSome) {
        if (mIsSome) {
          
          
          


          reset();
          emplace(*aOther);
        } else {
          emplace(*aOther);
        }
      } else {
        reset();
      }
    }
    return *this;
  }

  Maybe& operator=(Maybe&& aOther)
  {
    MOZ_ASSERT(this != &aOther, "Self-moves are prohibited");

    if (aOther.mIsSome) {
      if (mIsSome) {
        ref() = Move(aOther.ref());
      } else {
        mIsSome = true;
        ::new (mStorage.addr()) T(Move(*aOther));
      }
      aOther.reset();
    } else {
      reset();
    }

    return *this;
  }

  
  operator ConvertibleToBool() const { return mIsSome ? &Maybe::nonNull : 0; }
  bool isSome() const { return mIsSome; }
  bool isNothing() const { return !mIsSome; }

  
  T value() const
  {
    MOZ_ASSERT(mIsSome);
    return ref();
  }

  
  T* ptr()
  {
    MOZ_ASSERT(mIsSome);
    return &ref();
  }

  const T* ptr() const
  {
    MOZ_ASSERT(mIsSome);
    return &ref();
  }

  T* operator->()
  {
    MOZ_ASSERT(mIsSome);
    return ptr();
  }

  const T* operator->() const
  {
    MOZ_ASSERT(mIsSome);
    return ptr();
  }

  
  T& ref()
  {
    MOZ_ASSERT(mIsSome);
    return *mStorage.addr();
  }

  const T& ref() const
  {
    MOZ_ASSERT(mIsSome);
    return *mStorage.addr();
  }

  T& operator*()
  {
    MOZ_ASSERT(mIsSome);
    return ref();
  }

  const T& operator*() const
  {
    MOZ_ASSERT(mIsSome);
    return ref();
  }

  
  void reset()
  {
    if (isSome()) {
      ref().~T();
      mIsSome = false;
    }
  }

  






  void emplace()
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T();
    mIsSome = true;
  }

  template<typename T1>
  void emplace(T1&& t1)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1));
    mIsSome = true;
  }

  template<typename T1, typename T2>
  void emplace(T1&& t1, T2&& t2)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3>
  void emplace(T1&& t1, T2&& t2, T3&& t3)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6, typename T7>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6,
               T7&& t7)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6),
                              Forward<T7>(t7));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6, typename T7, typename T8>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6,
               T7&& t7, T8&& t8)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6),
                              Forward<T7>(t7), Forward<T8>(t8));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6, typename T7, typename T8, typename T9>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6,
               T7&& t7, T8&& t8, T9&& t9)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6),
                              Forward<T7>(t7), Forward<T8>(t8), Forward<T9>(t9));
    mIsSome = true;
  }

  template<typename T1, typename T2, typename T3, typename T4, typename T5,
           typename T6, typename T7, typename T8, typename T9, typename T10>
  void emplace(T1&& t1, T2&& t2, T3&& t3, T4&& t4, T5&& t5, T6&& t6,
               T7&& t7, T8&& t8, T9&& t9, T10&& t10)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<T1>(t1), Forward<T2>(t2), Forward<T3>(t3),
                              Forward<T4>(t4), Forward<T5>(t5), Forward<T6>(t6),
                              Forward<T7>(t7), Forward<T8>(t8), Forward<T9>(t9),
                              Forward<T1>(t10));
    mIsSome = true;
  }
};











template<typename T>
Maybe<typename RemoveCV<typename RemoveReference<T>::Type>::Type>
Some(T&& aValue)
{
  typedef typename RemoveCV<typename RemoveReference<T>::Type>::Type U;
  Maybe<U> value;
  value.emplace(Forward<T>(aValue));
  return value;
}

} 

#endif 
