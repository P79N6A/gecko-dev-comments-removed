







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

  
  explicit operator bool() const { return isSome(); }
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

  

  template<typename F, typename... Args>
  void apply(F&& aFunc, Args&&... aArgs)
  {
    if (isSome()) {
      aFunc(ref(), Forward<Args>(aArgs)...);
    }
  }

  template<typename F, typename... Args>
  void apply(F&& aFunc, Args&&... aArgs) const
  {
    if (isSome()) {
      aFunc(ref(), Forward<Args>(aArgs)...);
    }
  }

  



  template<typename R, typename... FArgs, typename... Args>
  Maybe<R> map(R (*aFunc)(T&, FArgs...), Args&&... aArgs)
  {
    if (isSome()) {
      Maybe<R> val;
      val.emplace(aFunc(ref(), Forward<Args>(aArgs)...));
      return val;
    }
    return Maybe<R>();
  }

  template<typename R, typename... FArgs, typename... Args>
  Maybe<R> map(R (*aFunc)(const T&, FArgs...), Args&&... aArgs) const
  {
    if (isSome()) {
      Maybe<R> val;
      val.emplace(aFunc(ref(), Forward<Args>(aArgs)...));
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

  



  template<typename... Args>
  void emplace(Args&&... aArgs)
  {
    MOZ_ASSERT(!mIsSome);
    ::new (mStorage.addr()) T(Forward<Args>(aArgs)...);
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
