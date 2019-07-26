




























































#ifndef mozilla_WeakPtr_h
#define mozilla_WeakPtr_h

#include "mozilla/Assertions.h"
#include "mozilla/Atomics.h"
#include "mozilla/NullPtr.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TypeTraits.h"

namespace mozilla {

template <typename T, class WeakReference> class WeakPtrBase;
template <typename T, class WeakReference> class SupportsWeakPtrBase;

namespace detail {


template<class T, RefCountAtomicity Atomicity>
class WeakReference : public RefCounted<WeakReference<T, Atomicity>, Atomicity>
{
  public:
    explicit WeakReference(T* p) : ptr(p) {}
    T* get() const {
      return ptr;
    }

  private:
    friend class WeakPtrBase<T, WeakReference>;
    friend class SupportsWeakPtrBase<T, WeakReference>;
    void detach() {
      ptr = nullptr;
    }
    T* ptr;
};

} 

template <typename T, class WeakReference>
class SupportsWeakPtrBase
{
  public:
    WeakPtrBase<T, WeakReference> asWeakPtr() {
      if (!weakRef)
        weakRef = new WeakReference(static_cast<T*>(this));
      return WeakPtrBase<T, WeakReference>(weakRef);
    }

  protected:
    ~SupportsWeakPtrBase() {
      MOZ_STATIC_ASSERT((IsBaseOf<SupportsWeakPtrBase<T, WeakReference>, T>::value),
                        "T must derive from SupportsWeakPtrBase<T, WeakReference>");
      if (weakRef)
        weakRef->detach();
    }

  private:
    friend class WeakPtrBase<T, WeakReference>;

    RefPtr<WeakReference> weakRef;
};

template <typename T>
class SupportsWeakPtr
  : public SupportsWeakPtrBase<T, detail::WeakReference<T, detail::NonAtomicRefCount> >
{
};

template <typename T>
class AtomicSupportsWeakPtr
  : public SupportsWeakPtrBase<T, detail::WeakReference<T, detail::AtomicRefCount> >
{
};

namespace detail {

template <typename T>
struct WeakReferenceCount
{
  static const RefCountAtomicity atomicity =
    IsBaseOf<AtomicSupportsWeakPtr<T>, T>::value
    ? AtomicRefCount
    : NonAtomicRefCount;
};

}

template <typename T, class WeakReference>
class WeakPtrBase
{
  public:
    WeakPtrBase(const WeakPtrBase<T, WeakReference>& o) : ref(o.ref) {}
    
    WeakPtrBase() : ref(new WeakReference(nullptr)) {}

    operator T*() const {
      return ref->get();
    }
    T& operator*() const {
      return *ref->get();
    }

    T* operator->() const {
      return ref->get();
    }

    T* get() const {
      return ref->get();
    }

  private:
    friend class SupportsWeakPtrBase<T, WeakReference>;

    explicit WeakPtrBase(const RefPtr<WeakReference> &o) : ref(o) {}

    RefPtr<WeakReference> ref;
};

template <typename T>
class WeakPtr : public WeakPtrBase<T, detail::WeakReference<T, detail::WeakReferenceCount<T>::atomicity> >
{
    typedef WeakPtrBase<T, detail::WeakReference<T, detail::WeakReferenceCount<T>::atomicity> > Base;
  public:
    WeakPtr(const WeakPtr<T>& o) : Base(o) {}
    WeakPtr(const Base& o) : Base(o) {}
    WeakPtr() {}
};

} 

#endif 
