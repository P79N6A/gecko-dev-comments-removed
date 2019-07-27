





#ifndef mozilla_StaticMutex_h
#define mozilla_StaticMutex_h

#include "mozilla/Atomics.h"
#include "mozilla/Mutex.h"

namespace mozilla {















class StaticMutex
{
public:
  
  
  
#ifdef DEBUG
  StaticMutex()
  {
    MOZ_ASSERT(!mMutex);
  }
#endif

  void Lock()
  {
    Mutex()->Lock();
  }

  void Unlock()
  {
    Mutex()->Unlock();
  }

  void AssertCurrentThreadOwns()
  {
#ifdef DEBUG
    Mutex()->AssertCurrentThreadOwns();
#endif
  }

private:
  OffTheBooksMutex* Mutex()
  {
    if (mMutex) {
      return mMutex;
    }

    OffTheBooksMutex* mutex = new OffTheBooksMutex("StaticMutex");
    if (!mMutex.compareExchange(nullptr, mutex)) {
      delete mutex;
    }

    return mMutex;
  }

  Atomic<OffTheBooksMutex*> mMutex;


  
  
  
  
#ifdef DEBUG
  StaticMutex(StaticMutex& aOther);
#endif

  
  StaticMutex& operator=(StaticMutex* aRhs);
  static void* operator new(size_t) CPP_THROW_NEW;
  static void operator delete(void*);
};

typedef BaseAutoLock<StaticMutex> StaticMutexAutoLock;
typedef BaseAutoUnlock<StaticMutex> StaticMutexAutoUnlock;

} 

#endif
