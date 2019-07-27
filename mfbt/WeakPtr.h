

































































#ifndef mozilla_WeakPtr_h
#define mozilla_WeakPtr_h

#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TypeTraits.h"

#include <string.h>

namespace mozilla {

template <typename T> class WeakPtr;
template <typename T> class SupportsWeakPtr;

#ifdef MOZ_REFCOUNTED_LEAK_CHECKING
#define MOZ_DECLARE_WEAKREFERENCE_TYPENAME(T) \
  static const char* weakReferenceTypeName() { return "WeakReference<" #T ">"; }
#else
#define MOZ_DECLARE_WEAKREFERENCE_TYPENAME(T)
#endif

namespace detail {



template<class T>
class WeakReference : public ::mozilla::RefCounted<WeakReference<T> >
{
public:
  explicit WeakReference(T* p) : mPtr(p) {}

  T* get() const { return mPtr; }

#ifdef MOZ_REFCOUNTED_LEAK_CHECKING
  const char* typeName() const
  {
    
    
    return T::weakReferenceTypeName();
  }
  size_t typeSize() const { return sizeof(*this); }
#endif

private:
  friend class mozilla::SupportsWeakPtr<T>;

  void detach() { mPtr = nullptr; }

  T* MOZ_NON_OWNING_REF mPtr;
};

} 

template <typename T>
class SupportsWeakPtr
{
protected:
  ~SupportsWeakPtr()
  {
    static_assert(IsBaseOf<SupportsWeakPtr<T>, T>::value,
                  "T must derive from SupportsWeakPtr<T>");
    if (mSelfReferencingWeakPtr) {
      mSelfReferencingWeakPtr.mRef->detach();
    }
  }

private:
  const WeakPtr<T>& SelfReferencingWeakPtr()
  {
    if (!mSelfReferencingWeakPtr) {
      mSelfReferencingWeakPtr.mRef = new detail::WeakReference<T>(static_cast<T*>(this));
    }
    return mSelfReferencingWeakPtr;
  }

  const WeakPtr<const T>& SelfReferencingWeakPtr() const
  {
    const WeakPtr<T>& p = const_cast<SupportsWeakPtr*>(this)->SelfReferencingWeakPtr();
    return reinterpret_cast<const WeakPtr<const T>&>(p);
  }

  friend class WeakPtr<T>;
  friend class WeakPtr<const T>;

  WeakPtr<T> mSelfReferencingWeakPtr;
};

template <typename T>
class WeakPtr
{
  typedef detail::WeakReference<T> WeakReference;

public:
  WeakPtr& operator=(const WeakPtr& aOther)
  {
    mRef = aOther.mRef;
    return *this;
  }

  WeakPtr(const WeakPtr& aOther)
  {
    *this = aOther;
  }

  WeakPtr& operator=(T* aOther)
  {
    return *this = aOther->SelfReferencingWeakPtr();
  }

  MOZ_IMPLICIT WeakPtr(T* aOther)
  {
    *this = aOther;
  }

  
  WeakPtr() : mRef(new WeakReference(nullptr)) {}

  operator T*() const { return mRef->get(); }
  T& operator*() const { return *mRef->get(); }

  T* operator->() const MOZ_NO_ADDREF_RELEASE_ON_RETURN { return mRef->get(); }

  T* get() const { return mRef->get(); }

private:
  friend class SupportsWeakPtr<T>;

  explicit WeakPtr(const RefPtr<WeakReference>& aOther) : mRef(aOther) {}

  RefPtr<WeakReference> mRef;
};

} 

#endif 
