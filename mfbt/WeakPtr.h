

































































#ifndef mozilla_WeakPtr_h
#define mozilla_WeakPtr_h

#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/NullPtr.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TypeTraits.h"

#include <string.h>

namespace mozilla {

template <typename T, class WeakReference> class WeakPtrBase;
template <typename T, class WeakReference> class SupportsWeakPtrBase;

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
  friend class WeakPtrBase<T, WeakReference<T> >;
  friend class SupportsWeakPtrBase<T, WeakReference<T> >;

  void detach() { mPtr = nullptr; }

  T* mPtr;
};

} 

template <typename T, class WeakReference>
class SupportsWeakPtrBase
{
public:
  WeakPtrBase<T, WeakReference> asWeakPtr()
  {
    if (!weakRef) {
      weakRef = new WeakReference(static_cast<T*>(this));
    }
    return WeakPtrBase<T, WeakReference>(weakRef);
  }

protected:
  ~SupportsWeakPtrBase()
  {
    static_assert(IsBaseOf<SupportsWeakPtrBase<T, WeakReference>, T>::value,
                  "T must derive from SupportsWeakPtrBase<T, WeakReference>");
    if (weakRef) {
      weakRef->detach();
    }
  }

private:
  friend class WeakPtrBase<T, WeakReference>;

  RefPtr<WeakReference> weakRef;
};

template <typename T>
class SupportsWeakPtr : public SupportsWeakPtrBase<T, detail::WeakReference<T> >
{
};

template <typename T, class WeakReference>
class WeakPtrBase
{
public:
  WeakPtrBase(const WeakPtrBase<T, WeakReference>& aOther)
    : mRef(aOther.mRef)
  {}

  
  WeakPtrBase() : mRef(new WeakReference(nullptr)) {}

  operator T*() const { return mRef->get(); }
  T& operator*() const { return *mRef->get(); }

  T* operator->() const { return mRef->get(); }

  T* get() const { return mRef->get(); }

private:
  friend class SupportsWeakPtrBase<T, WeakReference>;

  explicit WeakPtrBase(const RefPtr<WeakReference>& aOther) : mRef(aOther) {}

  RefPtr<WeakReference> mRef;
};

template <typename T>
class WeakPtr : public WeakPtrBase<T, detail::WeakReference<T> >
{
  typedef WeakPtrBase<T, detail::WeakReference<T> > Base;
public:
  WeakPtr(const WeakPtr<T>& aOther) : Base(aOther) {}
  MOZ_IMPLICIT WeakPtr(const Base& aOther) : Base(aOther) {}
  WeakPtr() {}
};

} 

#endif 
