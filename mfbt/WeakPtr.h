
























































#ifndef mozilla_WeakPtr_h_
#define mozilla_WeakPtr_h_

#include "mozilla/Assertions.h"
#include "mozilla/NullPtr.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TypeTraits.h"

namespace mozilla {

template <typename T> class WeakPtr;

template <typename T>
class SupportsWeakPtr
{
  public:
    WeakPtr<T> asWeakPtr() {
      if (!weakRef)
        weakRef = new WeakReference(static_cast<T*>(this));
      return WeakPtr<T>(weakRef);
    }

  protected:
    ~SupportsWeakPtr() {
      MOZ_STATIC_ASSERT((IsBaseOf<SupportsWeakPtr<T>, T>::value), "T must derive from SupportsWeakPtr<T>");
      if (weakRef)
        weakRef->detach();
    }

  private:
    friend class WeakPtr<T>;

    
    class WeakReference : public RefCounted<WeakReference>
    {
      public:
        explicit WeakReference(T* p) : ptr(p) {}
        T* get() const {
          return ptr;
        }

      private:
        friend class WeakPtr<T>;
        friend class SupportsWeakPtr<T>;
        void detach() {
          ptr = nullptr;
        }
        T* ptr;
    };

    RefPtr<WeakReference> weakRef;
};

template <typename T>
class WeakPtr
{
  public:
    WeakPtr(const WeakPtr<T>& o) : ref(o.ref) {}
    
    WeakPtr() : ref(new typename SupportsWeakPtr<T>::WeakReference(nullptr)) {}

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
    friend class SupportsWeakPtr<T>;

    explicit WeakPtr(const RefPtr<typename SupportsWeakPtr<T>::WeakReference> &o) : ref(o) {}

    RefPtr<typename SupportsWeakPtr<T>::WeakReference> ref;
};

} 

#endif 
