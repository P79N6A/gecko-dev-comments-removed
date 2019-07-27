





#ifndef mozilla_CondVar_h
#define mozilla_CondVar_h

#include "prcvar.h"

#include "mozilla/BlockingResourceBase.h"
#include "mozilla/Mutex.h"

#ifdef MOZILLA_INTERNAL_API
#include "GeckoProfiler.h"
#endif 

namespace mozilla {







class CondVar : BlockingResourceBase
{
public:
  










  CondVar(Mutex& aLock, const char* aName)
    : BlockingResourceBase(aName, eCondVar)
    , mLock(&aLock)
  {
    MOZ_COUNT_CTOR(CondVar);
    
    mCvar = PR_NewCondVar(mLock->mLock);
    if (!mCvar) {
      NS_RUNTIMEABORT("Can't allocate mozilla::CondVar");
    }
  }

  



  ~CondVar()
  {
    NS_ASSERTION(mCvar && mLock,
                 "improperly constructed CondVar or double free");
    PR_DestroyCondVar(mCvar);
    mCvar = 0;
    mLock = 0;
    MOZ_COUNT_DTOR(CondVar);
  }

#ifndef DEBUG
  



  nsresult Wait(PRIntervalTime aInterval = PR_INTERVAL_NO_TIMEOUT)
  {

#ifdef MOZILLA_INTERNAL_API
    GeckoProfilerSleepRAII profiler_sleep;
#endif 
    
    return PR_WaitCondVar(mCvar, aInterval) == PR_SUCCESS ? NS_OK :
                                                            NS_ERROR_FAILURE;
  }
#else
  nsresult Wait(PRIntervalTime aInterval = PR_INTERVAL_NO_TIMEOUT);
#endif 

  



  nsresult Notify()
  {
    
    return PR_NotifyCondVar(mCvar) == PR_SUCCESS ? NS_OK : NS_ERROR_FAILURE;
  }

  



  nsresult NotifyAll()
  {
    
    return PR_NotifyAllCondVar(mCvar) == PR_SUCCESS ? NS_OK : NS_ERROR_FAILURE;
  }

#ifdef DEBUG
  



  void AssertCurrentThreadOwnsMutex()
  {
    mLock->AssertCurrentThreadOwns();
  }

  



  void AssertNotCurrentThreadOwnsMutex()
  {
    mLock->AssertNotCurrentThreadOwns();
  }

#else
  void AssertCurrentThreadOwnsMutex() {}
  void AssertNotCurrentThreadOwnsMutex() {}

#endif  

private:
  CondVar();
  CondVar(CondVar&);
  CondVar& operator=(CondVar&);

  Mutex* mLock;
  PRCondVar* mCvar;
};


} 


#endif  
