

































































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
  friend class WeakPtr<T>;
  friend class SupportsWeakPtr<T>;

  void detach() { mPtr = nullptr; }

  T* mPtr;
};

} 

template <typename T>
class SupportsWeakPtr
{
public:
  WeakPtr<T> asWeakPtr()
  {
    if (!weakRef) {
      weakRef = new detail::WeakReference<T>(static_cast<T*>(this));
    }
    return WeakPtr<T>(weakRef);
  }

protected:
  ~SupportsWeakPtr()
  {
    static_assert(IsBaseOf<SupportsWeakPtr<T>, T>::value,
                  "T must derive from SupportsWeakPtr<T>");
    if (weakRef) {
      weakRef->detach();
    }
  }

private:
  friend class WeakPtr<T>;

  RefPtr<detail::WeakReference<T>> weakRef;
};

template <typename T>
class WeakPtr
{
public:
  WeakPtr(const WeakPtr<T>& aOther)
    : mRef(aOther.mRef)
  {}

  
  WeakPtr() : mRef(new detail::WeakReference<T>(nullptr)) {}

  operator T*() const { return mRef->get(); }
  T& operator*() const { return *mRef->get(); }

  T* operator->() const { return mRef->get(); }

  T* get() const { return mRef->get(); }

private:
  friend class SupportsWeakPtr<T>;

  explicit WeakPtr(const RefPtr<detail::WeakReference<T>>& aOther) : mRef(aOther) {}

  RefPtr<detail::WeakReference<T>> mRef;
};

} 

#endif 
