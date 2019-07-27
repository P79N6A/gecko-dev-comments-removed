







#ifndef mozilla_dom_OwningNonNull_h
#define mozilla_dom_OwningNonNull_h

#include "nsAutoPtr.h"
#include "nsCycleCollectionNoteChild.h"

namespace mozilla {
namespace dom {

template<class T>
class OwningNonNull
{
public:
  OwningNonNull() {}

  MOZ_IMPLICIT OwningNonNull(T& aValue)
  {
    init(&aValue);
  }

  template<class U>
  MOZ_IMPLICIT OwningNonNull(already_AddRefed<U>&& aValue)
  {
    init(aValue.take());
  }

  
  operator T&() const
  {
    MOZ_ASSERT(mInited);
    MOZ_ASSERT(mPtr, "OwningNonNull<T> was set to null");
    return *mPtr;
  }

  operator T*() const
  {
    MOZ_ASSERT(mInited);
    MOZ_ASSERT(mPtr, "OwningNonNull<T> was set to null");
    return mPtr;
  }

  
  explicit operator bool() const = delete;

  T*
  operator->() const
  {
    MOZ_ASSERT(mInited);
    MOZ_ASSERT(mPtr, "OwningNonNull<T> was set to null");
    return mPtr;
  }

  OwningNonNull<T>&
  operator=(T* aValue)
  {
    init(aValue);
    return *this;
  }

  OwningNonNull<T>&
  operator=(T& aValue)
  {
    init(&aValue);
    return *this;
  }

  OwningNonNull<T>&
  operator=(const already_AddRefed<T>& aValue)
  {
    init(aValue);
    return *this;
  }

  
  void operator=(decltype(nullptr)) = delete;

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

  template<typename U>
  void swap(U& aOther)
  {
    mPtr.swap(aOther);
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
  bool mInited = false;
#endif
};

template <typename T>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            OwningNonNull<T>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  CycleCollectionNoteChild(aCallback, aField.get(), aName, aFlags);
}

} 
} 


template<class T> template<class U>
nsCOMPtr<T>::nsCOMPtr(const mozilla::dom::OwningNonNull<U>& aOther)
  : nsCOMPtr(aOther.get())
{}

template<class T> template<class U>
nsCOMPtr<T>&
nsCOMPtr<T>::operator=(const mozilla::dom::OwningNonNull<U>& aOther)
{
  return operator=(aOther.get());
}


template<class T> template<class U>
nsRefPtr<T>::nsRefPtr(const mozilla::dom::OwningNonNull<U>& aOther)
  : nsRefPtr(aOther.get())
{}

template<class T> template<class U>
nsRefPtr<T>&
nsRefPtr<T>::operator=(const mozilla::dom::OwningNonNull<U>& aOther)
{
  return operator=(aOther.get());
}

#endif 
