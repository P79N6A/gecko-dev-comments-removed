

































































#ifndef mozilla_WeakPtr_h
#define mozilla_WeakPtr_h

#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/NullPtr.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TypeTraits.h"

#include <string.h>

namespace mozilla {

template <typename T> class WeakPtr;
template <typename T> class SupportsWeakPtr;

namespace detail {



template<class T>
class WeakReference : public ::mozilla::RefCounted<WeakReference<T> >
{
public:
  explicit WeakReference(T* p) : mPtr(p) {}

  T* get() const { return mPtr; }

#ifdef MOZ_REFCOUNTED_LEAK_CHECKING
#ifdef XP_WIN
#define snprintf _snprintf
#endif
  const char* typeName() const
  {
    static char nameBuffer[1024];
    const char* innerType = mPtr->typeName();
    
    
    
    MOZ_ASSERT(strlen(innerType) + sizeof("WeakReference<>") <
               ArrayLength(nameBuffer),
               "Exceedingly large type name");
    snprintf(nameBuffer, ArrayLength(nameBuffer), "WeakReference<%s>",
             innerType);
    
    
    return nameBuffer;
  }

  size_t typeSize() const { return sizeof(*this); }
#undef snprintf
#endif

private:
  friend class mozilla::SupportsWeakPtr<T>;

  void detach() { mPtr = nullptr; }

  T* mPtr;
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

  T* operator->() const { return mRef->get(); }

  T* get() const { return mRef->get(); }

private:
  friend class SupportsWeakPtr<T>;

  explicit WeakPtr(const RefPtr<WeakReference>& aOther) : mRef(aOther) {}

  RefPtr<WeakReference> mRef;
};

} 

#endif 
