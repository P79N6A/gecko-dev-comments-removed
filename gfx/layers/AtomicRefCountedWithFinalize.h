




#ifndef MOZILLA_ATOMICREFCOUNTEDWITHFINALIZE_H_
#define MOZILLA_ATOMICREFCOUNTEDWITHFINALIZE_H_

#include "mozilla/RefPtr.h"
#include "mozilla/NullPtr.h"

namespace mozilla {

template<typename T>
class AtomicRefCountedWithFinalize
{
  protected:
    AtomicRefCountedWithFinalize()
      : mRecycleCallback(nullptr)
      , mRefCount(0)
    {}

    ~AtomicRefCountedWithFinalize() {}

  public:
    void AddRef() {
      MOZ_ASSERT(mRefCount >= 0);
      ++mRefCount;
    }

    void Release() {
      MOZ_ASSERT(mRefCount > 0);
      int currCount = --mRefCount;
      if (0 == currCount) {
        
        
        MOZ_ASSERT(mRecycleCallback == nullptr);
#ifdef DEBUG
        mRefCount = detail::DEAD;
#endif
        T* derived = static_cast<T*>(this);
        derived->Finalize();
        delete derived;
      } else if (1 == currCount && mRecycleCallback) {
        T* derived = static_cast<T*>(this);
        mRecycleCallback(derived, mClosure);
      }
    }

    typedef void (*RecycleCallback)(T* aObject, void* aClosure);
    



    void SetRecycleCallback(RecycleCallback aCallback, void* aClosure)
    {
      mRecycleCallback = aCallback;
      mClosure = aClosure;
    }
    void ClearRecycleCallback() { SetRecycleCallback(nullptr, nullptr); }

private:
    RecycleCallback mRecycleCallback;
    void *mClosure;
    Atomic<int> mRefCount;
};

}

#endif
