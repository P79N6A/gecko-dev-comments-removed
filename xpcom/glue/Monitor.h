





#ifndef mozilla_Monitor_h
#define mozilla_Monitor_h

#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"

namespace mozilla {










class Monitor
{
public:
  explicit Monitor(const char* aName)
    : mMutex(aName)
    , mCondVar(mMutex, "[Monitor.mCondVar]")
  {
  }

  ~Monitor() {}

  void Lock() { mMutex.Lock(); }
  void Unlock() { mMutex.Unlock(); }

  nsresult Wait(PRIntervalTime aInterval = PR_INTERVAL_NO_TIMEOUT)
  {
    return mCondVar.Wait(aInterval);
  }

  nsresult Notify() { return mCondVar.Notify(); }
  nsresult NotifyAll() { return mCondVar.NotifyAll(); }

  void AssertCurrentThreadOwns() const
  {
    mMutex.AssertCurrentThreadOwns();
  }

  void AssertNotCurrentThreadOwns() const
  {
    mMutex.AssertNotCurrentThreadOwns();
  }

private:
  Monitor();
  Monitor(const Monitor&);
  Monitor& operator=(const Monitor&);

  Mutex mMutex;
  CondVar mCondVar;
};








class MOZ_STACK_CLASS MonitorAutoLock
{
public:
  explicit MonitorAutoLock(Monitor& aMonitor)
    : mMonitor(&aMonitor)
  {
    mMonitor->Lock();
  }

  ~MonitorAutoLock()
  {
    mMonitor->Unlock();
  }

  nsresult Wait(PRIntervalTime aInterval = PR_INTERVAL_NO_TIMEOUT)
  {
    return mMonitor->Wait(aInterval);
  }

  nsresult Notify() { return mMonitor->Notify(); }
  nsresult NotifyAll() { return mMonitor->NotifyAll(); }

private:
  MonitorAutoLock();
  MonitorAutoLock(const MonitorAutoLock&);
  MonitorAutoLock& operator=(const MonitorAutoLock&);
  static void* operator new(size_t) CPP_THROW_NEW;

  Monitor* mMonitor;
};








class MOZ_STACK_CLASS MonitorAutoUnlock
{
public:
  explicit MonitorAutoUnlock(Monitor& aMonitor)
    : mMonitor(&aMonitor)
  {
    mMonitor->Unlock();
  }

  ~MonitorAutoUnlock()
  {
    mMonitor->Lock();
  }

private:
  MonitorAutoUnlock();
  MonitorAutoUnlock(const MonitorAutoUnlock&);
  MonitorAutoUnlock& operator=(const MonitorAutoUnlock&);
  static void* operator new(size_t) CPP_THROW_NEW;

  Monitor* mMonitor;
};

} 

#endif 
