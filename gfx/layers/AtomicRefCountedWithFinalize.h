




#ifndef MOZILLA_ATOMICREFCOUNTEDWITHFINALIZE_H_
#define MOZILLA_ATOMICREFCOUNTEDWITHFINALIZE_H_

#include "mozilla/RefPtr.h"
#include "mozilla/Likely.h"
#include "MainThreadUtils.h"
#include "base/message_loop.h"
#include "base/task.h"
#include "mozilla/gfx/Logging.h"

namespace mozilla {

template<typename T>
class AtomicRefCountedWithFinalize
{
  protected:
    AtomicRefCountedWithFinalize()
      : mRecycleCallback(nullptr)
      , mRefCount(0)
      , mMessageLoopToPostDestructionTo(nullptr)
    {}

    ~AtomicRefCountedWithFinalize() {
      if (mRefCount >= 0) {
        gfxCriticalError() << "Deleting referenced object? " << mRefCount;
      }
    }

    void SetMessageLoopToPostDestructionTo(MessageLoop* l) {
      MOZ_ASSERT(NS_IsMainThread());
      mMessageLoopToPostDestructionTo = l;
    }

    static void DestroyToBeCalledOnMainThread(T* ptr) {
      MOZ_ASSERT(NS_IsMainThread());
      delete ptr;
    }

  public:
    void AddRef() {
      ++mRefCount;
    }

    void Release() {
      
      
      
      
      RecycleCallback recycleCallback = mRecycleCallback;
      int currCount = --mRefCount;
      if (currCount < 0) {
        gfxCriticalError() << "Invalid reference count release" << currCount;
        ++mRefCount;
        return;
      }

      if (0 == currCount) {
        mRefCount = detail::DEAD;
        MOZ_ASSERT(IsDead());

        
        
        if (mRecycleCallback) {
          gfxCriticalError() << "About to release with valid callback";
          mRecycleCallback = nullptr;
        }

        T* derived = static_cast<T*>(this);
        derived->Finalize();
        if (MOZ_LIKELY(!mMessageLoopToPostDestructionTo)) {
          delete derived;
        } else {
          if (MOZ_LIKELY(NS_IsMainThread())) {
            delete derived;
          } else {
            mMessageLoopToPostDestructionTo->PostTask(
              FROM_HERE,
              NewRunnableFunction(&DestroyToBeCalledOnMainThread, derived));
          }
        }
      } else if (1 == currCount && recycleCallback) {
        
        
        
        MOZ_ASSERT(!IsDead());
        T* derived = static_cast<T*>(this);
        recycleCallback(derived, mClosure);
      }
    }

    typedef void (*RecycleCallback)(T* aObject, void* aClosure);
    



    void SetRecycleCallback(RecycleCallback aCallback, void* aClosure)
    {
      MOZ_ASSERT(!IsDead());
      mRecycleCallback = aCallback;
      mClosure = aClosure;
    }
    void ClearRecycleCallback()
    {
      MOZ_ASSERT(!IsDead());
      SetRecycleCallback(nullptr, nullptr);
    }

    bool HasRecycleCallback() const
    {
      MOZ_ASSERT(!IsDead());
      return !!mRecycleCallback;
    }

    bool IsDead() const
    {
      return mRefCount < 0;
    }

private:
    RecycleCallback mRecycleCallback;
    void *mClosure;
    Atomic<int> mRefCount;
    MessageLoop *mMessageLoopToPostDestructionTo;
};

}

#endif
