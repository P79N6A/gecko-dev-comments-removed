







#ifndef mozilla_Maybe_h
#define mozilla_Maybe_h

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
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

  MOZ_IMPLICIT Maybe(Nothing) : mIsSome(false) { }

  Maybe(const Maybe& aOther)
    : mIsSome(false)
  {
    if (aOther.mIsSome) {
      emplace(*aOther);
    }
  }

  Maybe(Maybe&& aOther)
    : mIsSome(false)
  {
    if (aOther.mIsSome) {
      emplace(Move(*aOther));
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
        emplace(Move(*aOther));
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

  



  template<typename V>
  T valueOr(V&& aDefault) const
  {
    if (isSome()) {
      return ref();
    }
    return Forward<V>(aDefault);
  }

  



  template<typename F>
  T valueOrFrom(F&& aFunc) const
  {
    if (isSome()) {
      return ref();
    }
    return aFunc();
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

  



  T* ptrOr(T* aDefault)
  {
    if (isSome()) {
      return ptr();
    }
    return aDefault;
  }

  const T* ptrOr(const T* aDefault) const
  {
    if (isSome()) {
      return ptr();
    }
    return aDefault;
  }

  



  template<typename F>
  T* ptrOrFrom(F&& aFunc)
  {
    if (isSome()) {
      return ptr();
    }
    return aFunc();
  }

  template<typename F>
  const T* ptrOrFrom(F&& aFunc) const
  {
    if (isSome()) {
      return ptr();
    }
    return aFunc();
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

  



  T& refOr(T& aDefault)
  {
    if (isSome()) {
      return ref();
    }
    return aDefault;
  }

  const T& refOr(const T& aDefault) const
  {
    if (isSome()) {
      return ref();
    }
    return aDefault;
  }

  



  template<typename F>
  T& refOrFrom(F&& aFunc)
  {
    if (isSome()) {
      return ref();
    }
    return aFunc();
  }

  template<typename F>
  const T& refOrFrom(F&& aFunc) const
  {
    if (isSome()) {
      return ref();
    }
    return aFunc();
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

  

  template<typename F>
  void apply(F&& aFunc)
  {
    if (isSome()) {
      aFunc(ref());
    }
  }

  template<typename F>
  void apply(F&& aFunc) const
  {
    if (isSome()) {
      aFunc(ref());
    }
  }

  
  template<typename F, typename A>
  void apply(F&& aFunc, A&& aArg)
  {
    if (isSome()) {
      aFunc(ref(), Forward<A>(aArg));
    }
  }

  template<typename F, typename A>
  void apply(F&& aFunc, A&& aArg) const
  {
    if (isSome()) {
      aFunc(ref(), Forward<A>(aArg));
    }
  }

  



  template<typename R>
  Maybe<R> map(R(*aFunc)(T&))
  {
    if (isSome()) {
      Maybe<R> val;
      val.emplace(aFunc(ref()));
      return val;
    }
    return Maybe<R>();
  }

  template<typename R>
  Maybe<R> map(R(*aFunc)(const T&)) const
  {
    if (isSome()) {
      Maybe<R> val;
      val.emplace(aFunc(ref()));
      return val;
    }
    return Maybe<R>();
  }

  
  template<typename R, typename FA, typename A>
  Maybe<R> map(R(*aFunc)(T&, FA), A&& aArg)
  {
    if (isSome()) {
      Maybe<R> val;
      val.emplace(aFunc(ref(), Forward<A>(aArg)));
      return val;
    }
    return Maybe<R>();
  }

  template<typename R, typename FA, typename A>
  Maybe<R> map(R(*aFunc)(const T&, FA), A&& aArg) const
  {
    if (isSome()) {
      Maybe<R> val;
      val.emplace(aFunc(ref(), Forward<A>(aArg)));
      return val;
    }
    return Maybe<R>();
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

template<typename T>
Maybe<typename RemoveCV<typename RemoveReference<T>::Type>::Type>
ToMaybe(T* aPtr)
{
  if (aPtr) {
    return Some(*aPtr);
  }
  return Nothing();
}






template<typename T> bool
operator==(const Maybe<T>& aLHS, const Maybe<T>& aRHS)
{
  if (aLHS.isNothing() != aRHS.isNothing()) {
    return false;
  }
  return aLHS.isNothing() || *aLHS == *aRHS;
}

template<typename T> bool
operator!=(const Maybe<T>& aLHS, const Maybe<T>& aRHS)
{
  return !(aLHS == aRHS);
}





template<typename T> bool
operator==(const Maybe<T>& aLHS, const Nothing& aRHS)
{
  return aLHS.isNothing();
}

template<typename T> bool
operator!=(const Maybe<T>& aLHS, const Nothing& aRHS)
{
  return !(aLHS == aRHS);
}

template<typename T> bool
operator==(const Nothing& aLHS, const Maybe<T>& aRHS)
{
  return aRHS.isNothing();
}

template<typename T> bool
operator!=(const Nothing& aLHS, const Maybe<T>& aRHS)
{
  return !(aLHS == aRHS);
}





template<typename T> bool
operator<(const Maybe<T>& aLHS, const Maybe<T>& aRHS)
{
  if (aLHS.isNothing()) {
    return aRHS.isSome();
  }
  if (aRHS.isNothing()) {
    return false;
  }
  return *aLHS < *aRHS;
}

template<typename T> bool
operator>(const Maybe<T>& aLHS, const Maybe<T>& aRHS)
{
  return !(aLHS < aRHS || aLHS == aRHS);
}

template<typename T> bool
operator<=(const Maybe<T>& aLHS, const Maybe<T>& aRHS)
{
  return aLHS < aRHS || aLHS == aRHS;
}

template<typename T> bool
operator>=(const Maybe<T>& aLHS, const Maybe<T>& aRHS)
{
  return !(aLHS < aRHS);
}

} 

#endif 
