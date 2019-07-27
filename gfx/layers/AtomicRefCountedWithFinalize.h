




#ifndef MOZILLA_ATOMICREFCOUNTEDWITHFINALIZE_H_
#define MOZILLA_ATOMICREFCOUNTEDWITHFINALIZE_H_

#include "mozilla/RefPtr.h"
#include "mozilla/Likely.h"
#include "MainThreadUtils.h"
#include "base/message_loop.h"
#include "base/task.h"
#include "mozilla/gfx/Logging.h"

#define ADDREF_MANUALLY(obj)  (obj)->AddRefManually(__FUNCTION__, __FILE__, __LINE__)
#define RELEASE_MANUALLY(obj)  (obj)->ReleaseManually(__FUNCTION__, __FILE__, __LINE__)

namespace mozilla {

template<class U>
class StaticRefPtr;

namespace gl {
template<typename T>
class RefSet;

template<typename T>
class RefQueue;
}

template<typename T>
class AtomicRefCountedWithFinalize
{
protected:
    AtomicRefCountedWithFinalize()
      : mRecycleCallback(nullptr)
      , mRefCount(0)
      , mMessageLoopToPostDestructionTo(nullptr)
#ifdef DEBUG
      , mSpew(false)
      , mManualAddRefs(0)
      , mManualReleases(0)
#endif
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
    
    template<typename U>
    friend class RefPtr;

    template<class U>
    friend class ::mozilla::StaticRefPtr;

    template<class U>
    friend class ::nsRefPtr;

    template<class U>
    friend struct ::RunnableMethodTraits;

    template<typename U>
    friend class ::mozilla::gl::RefSet;

    template<typename U>
    friend class ::mozilla::gl::RefQueue;

    

    void AddRefManually(const char* funcName, const char* fileName, uint32_t lineNum) {
#ifdef DEBUG
      uint32_t count = ++mManualAddRefs;
      if (mSpew) {
        printf_stderr("AddRefManually() #%u in %s at %s:%u\n", count, funcName,
                      fileName, lineNum);
      }
#else
      (void)funcName;
      (void)fileName;
      (void)lineNum;
#endif
      AddRef();
    }

    void ReleaseManually(const char* funcName, const char* fileName, uint32_t lineNum) {
#ifdef DEBUG
      uint32_t count = ++mManualReleases;
      if (mSpew) {
        printf_stderr("ReleaseManually() #%u in %s at %s:%u\n", count, funcName,
                      fileName, lineNum);
      }
#else
      (void)funcName;
      (void)fileName;
      (void)lineNum;
#endif
      Release();
    }

private:
    void AddRef() {
      MOZ_ASSERT(mRefCount >= 0, "AddRef() during/after Finalize()/dtor.");
      ++mRefCount;
    }

    void Release() {
      MOZ_ASSERT(mRefCount > 0, "Release() during/after Finalize()/dtor.");
      
      
      
      
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

        MOZ_ASSERT(mManualAddRefs == mManualReleases);

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

public:
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
#ifdef DEBUG
public:
    bool mSpew;
private:
    Atomic<uint32_t> mManualAddRefs;
    Atomic<uint32_t> mManualReleases;
#endif
};

} 

#endif
