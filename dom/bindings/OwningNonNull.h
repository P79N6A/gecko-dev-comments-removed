







#ifndef mozilla_dom_OwningNonNull_h
#define mozilla_dom_OwningNonNull_h

#include "nsAutoPtr.h"

namespace mozilla {
namespace dom {

template<class T>
class OwningNonNull
{
public:
  OwningNonNull()
#ifdef DEBUG
    : mInited(false)
#endif
  {}

  operator T&()
  {
    MOZ_ASSERT(mInited);
    MOZ_ASSERT(mPtr, "OwningNonNull<T> was set to null");
    return *mPtr;
  }

  void operator=(T* aValue)
  {
    init(aValue);
  }

  void operator=(const already_AddRefed<T>& aValue)
  {
    init(aValue);
  }

  already_AddRefed<T> forget()
  {
#ifdef DEBUG
    mInited = false;
#endif
    return mPtr.forget();
  }

  
  T* get() const
  {
    MOZ_ASSERT(mInited);
    MOZ_ASSERT(mPtr);
    return mPtr;
  }

protected:
  template<typename U>
  void init(U aValue)
  {
    mPtr = aValue;
    MOZ_ASSERT(mPtr);
#ifdef DEBUG
    mInited = true;
#endif
  }

  nsRefPtr<T> mPtr;
#ifdef DEBUG
  bool mInited;
#endif
};

} 
} 

#endif 
