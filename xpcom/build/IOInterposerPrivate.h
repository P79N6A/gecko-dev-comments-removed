





#ifndef xpcom_build_IOInterposerPrivate_h
#define xpcom_build_IOInterposerPrivate_h





#include <prcvar.h>
#include <prlock.h>

namespace mozilla {
namespace IOInterposer {










class Monitor
{
public:
  Monitor()
    : mLock(PR_NewLock())
    , mCondVar(PR_NewCondVar(mLock))
  {
  }

  ~Monitor()
  {
    PR_DestroyCondVar(mCondVar);
    mCondVar = nullptr;
    PR_DestroyLock(mLock);
    mLock = nullptr;
  }

  void Lock()
  {
    PR_Lock(mLock);
  }

  void Unlock()
  {
    PR_Unlock(mLock);
  }

  bool Wait(PRIntervalTime aTimeout = PR_INTERVAL_NO_TIMEOUT)
  {
    return PR_WaitCondVar(mCondVar, aTimeout) == PR_SUCCESS;
  }

  bool Notify()
  {
    return PR_NotifyCondVar(mCondVar) == PR_SUCCESS;
  }

private:
  PRLock*    mLock;
  PRCondVar* mCondVar;
};

class MonitorAutoLock
{
public:
  explicit MonitorAutoLock(Monitor &aMonitor)
    : mMonitor(aMonitor)
  {
    mMonitor.Lock();
  }

  ~MonitorAutoLock()
  {
    mMonitor.Unlock();
  }

  bool Wait(PRIntervalTime aTimeout = PR_INTERVAL_NO_TIMEOUT)
  {
    return mMonitor.Wait(aTimeout);
  }

  bool Notify()
  {
    return mMonitor.Notify();
  }

private:
  Monitor&  mMonitor;
};

class MonitorAutoUnlock
{
public:
  explicit MonitorAutoUnlock(Monitor &aMonitor)
    : mMonitor(aMonitor)
  {
    mMonitor.Unlock();
  }

  ~MonitorAutoUnlock()
  {
    mMonitor.Lock();
  }

private:
  Monitor&  mMonitor;
};

class Mutex
{
public:
  Mutex()
    : mPRLock(PR_NewLock())
  {
  }

  ~Mutex()
  {
    PR_DestroyLock(mPRLock);
    mPRLock = nullptr;
  }

  void Lock()
  {
    PR_Lock(mPRLock);
  }

  void Unlock()
  {
    PR_Unlock(mPRLock);
  }

private:
  PRLock*   mPRLock;
};

class AutoLock
{
public:
  explicit AutoLock(Mutex& aLock)
    : mLock(aLock)
  {
    mLock.Lock();
  }

  ~AutoLock()
  {
    mLock.Unlock();
  }

private:
  Mutex&     mLock;
};

} 
} 

#endif 

