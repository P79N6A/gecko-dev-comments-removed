




#include "mozilla/BackgroundHangMonitor.h"
#include "mozilla/LinkedList.h"
#include "mozilla/Monitor.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ThreadLocal.h"

#include "prinrval.h"
#include "prthread.h"

#include <algorithm>

namespace mozilla {





class BackgroundHangManager : public AtomicRefCounted<BackgroundHangManager>
{
private:
  
  static void MonitorThread(void* aData)
  {
    PR_SetCurrentThreadName("BgHangManager");
    
    RefPtr<BackgroundHangManager>(
      static_cast<BackgroundHangManager*>(aData))->RunMonitorThread();
  }

  
  PRThread* mHangMonitorThread;
  
  bool mShutdown;

  BackgroundHangManager(const BackgroundHangManager&);
  BackgroundHangManager& operator=(const BackgroundHangManager&);
  void RunMonitorThread();

public:
  static StaticRefPtr<BackgroundHangManager> sInstance;

  
  Monitor mLock;
  
  PRIntervalTime mIntervalNow;
  
  LinkedList<BackgroundHangThread> mHangThreads;

  void Shutdown()
  {
    MonitorAutoLock autoLock(mLock);
    mShutdown = true;
    autoLock.Notify();
  }

  void Wakeup()
  {
    
    PR_Interrupt(mHangMonitorThread);
  }

  BackgroundHangManager();
  ~BackgroundHangManager();
};





class BackgroundHangThread : public RefCounted<BackgroundHangThread>
                           , public LinkedListElement<BackgroundHangThread>
{
private:
  static ThreadLocal<BackgroundHangThread*> sTlsKey;

  BackgroundHangThread(const BackgroundHangThread&);
  BackgroundHangThread& operator=(const BackgroundHangThread&);

  

  const RefPtr<BackgroundHangManager> mManager;
  
  const PRThread* mThreadID;

public:
  static BackgroundHangThread* FindThread();

  static void Startup()
  {
    

    if (!sTlsKey.init()) {}
  }

  
  const nsAutoCString mThreadName;
  
  const PRIntervalTime mTimeout;
  
  const PRIntervalTime mMaxTimeout;
  
  PRIntervalTime mInterval;
  
  bool mWaiting;

  BackgroundHangThread(const char* aName,
                       uint32_t aTimeoutMs,
                       uint32_t aMaxTimeoutMs);
  ~BackgroundHangThread();

  
  void ReportHang(PRIntervalTime aHangTime) const;
  
  void ReportPermaHang() const;
  
  void NotifyActivity();
  
  void NotifyWait()
  {
    NotifyActivity();
    mWaiting = true;
  }
};


StaticRefPtr<BackgroundHangManager> BackgroundHangManager::sInstance;

ThreadLocal<BackgroundHangThread*> BackgroundHangThread::sTlsKey;


BackgroundHangManager::BackgroundHangManager()
  : mShutdown(false)
  , mLock("BackgroundHangManager")
  , mIntervalNow(0)
{
  
  MonitorAutoLock autoLock(mLock);
  mHangMonitorThread = PR_CreateThread(
    PR_USER_THREAD, MonitorThread, this,
    PR_PRIORITY_LOW, PR_GLOBAL_THREAD, PR_UNJOINABLE_THREAD, 0);
}

BackgroundHangManager::~BackgroundHangManager()
{
  MOZ_ASSERT(mShutdown,
    "Destruction without Shutdown call");
  MOZ_ASSERT(mHangThreads.isEmpty(),
    "Destruction with outstanding monitors");
}

void
BackgroundHangManager::RunMonitorThread()
{
  
  MonitorAutoLock autoLock(mLock);

  





  PRIntervalTime systemTime = PR_IntervalNow();
  
  PRIntervalTime waitTime = PR_INTERVAL_NO_WAIT;
  PRIntervalTime permaHangTimeout = PR_INTERVAL_NO_WAIT;

  while (!mShutdown) {

    PR_ClearInterrupt();
    nsresult rv = autoLock.Wait(waitTime);

    PRIntervalTime newTime = PR_IntervalNow();
    PRIntervalTime systemInterval = newTime - systemTime;
    systemTime = newTime;

    


    if (MOZ_LIKELY(waitTime != PR_INTERVAL_NO_TIMEOUT &&
                   systemInterval < 2 * waitTime)) {
      mIntervalNow += systemInterval;
    }

    


    if (MOZ_LIKELY(systemInterval < permaHangTimeout &&
                   systemInterval >= waitTime &&
                   rv == NS_OK)) {
      permaHangTimeout -= systemInterval;
      continue;
    }

    





    waitTime = PR_INTERVAL_NO_TIMEOUT;
    permaHangTimeout = PR_INTERVAL_NO_TIMEOUT;

    
    PRIntervalTime intervalNow = mIntervalNow;

    
    for (BackgroundHangThread* currentThread = mHangThreads.getFirst();
         currentThread; currentThread = currentThread->getNext()) {

      if (currentThread->mWaiting) {
        
        continue;
      }
      PRIntervalTime hangTime = intervalNow - currentThread->mInterval;
      if (MOZ_UNLIKELY(hangTime >= currentThread->mMaxTimeout)) {
        
        currentThread->mWaiting = true;
        currentThread->ReportPermaHang();
        continue;
      }
      

      waitTime = std::min(waitTime, currentThread->mTimeout / 4);
      permaHangTimeout = std::min(
        permaHangTimeout, currentThread->mMaxTimeout - hangTime);
    }
  }

  

  while (!mHangThreads.isEmpty()) {
    autoLock.Wait(PR_INTERVAL_NO_TIMEOUT);
  }
}


BackgroundHangThread::BackgroundHangThread(const char* aName,
                                           uint32_t aTimeoutMs,
                                           uint32_t aMaxTimeoutMs)
  : mManager(BackgroundHangManager::sInstance)
  , mThreadID(PR_GetCurrentThread())
  , mThreadName(aName)
  , mTimeout(PR_MillisecondsToInterval(aTimeoutMs))
  , mMaxTimeout(PR_MillisecondsToInterval(aMaxTimeoutMs))
  , mInterval(mManager->mIntervalNow)
  , mWaiting(true)
{
  if (sTlsKey.initialized()) {
    sTlsKey.set(this);
  }
  
  MonitorAutoLock autoLock(mManager->mLock);
  
  mManager->mHangThreads.insertBack(this);
  
  autoLock.Notify();
}

BackgroundHangThread::~BackgroundHangThread()
{
  
  MonitorAutoLock autoLock(mManager->mLock);
  
  remove();
  
  autoLock.Notify();

  
  if (sTlsKey.initialized()) {
    sTlsKey.set(nullptr);
  }
}

void
BackgroundHangThread::ReportHang(PRIntervalTime aHangTime) const
{
  
  

  
}

void
BackgroundHangThread::ReportPermaHang() const
{
  
  

  
}

MOZ_ALWAYS_INLINE void
BackgroundHangThread::NotifyActivity()
{
  PRIntervalTime intervalNow = mManager->mIntervalNow;
  if (mWaiting) {
    mInterval = intervalNow;
    mWaiting = false;
    

    mManager->Wakeup();
  } else {
    PRIntervalTime duration = intervalNow - mInterval;
    if (MOZ_UNLIKELY(duration >= mTimeout)) {
      ReportHang(duration);
    }
    mInterval = intervalNow;
  }
}

BackgroundHangThread*
BackgroundHangThread::FindThread()
{
  if (sTlsKey.initialized()) {
    
    return sTlsKey.get();
  }
  
  RefPtr<BackgroundHangManager> manager(BackgroundHangManager::sInstance);
  MOZ_ASSERT(manager, "Creating BackgroundHangMonitor after shutdown");

  PRThread* threadID = PR_GetCurrentThread();
  
  MonitorAutoLock autoLock(manager->mLock);
  for (BackgroundHangThread* thread = manager->mHangThreads.getFirst();
       thread; thread = thread->getNext()) {
    if (thread->mThreadID == threadID) {
      return thread;
    }
  }
  
  return nullptr;
}


void
BackgroundHangMonitor::Startup()
{
  MOZ_ASSERT(!BackgroundHangManager::sInstance, "Already initialized");
  BackgroundHangThread::Startup();
  BackgroundHangManager::sInstance = new BackgroundHangManager();
}

void
BackgroundHangMonitor::Shutdown()
{
  MOZ_ASSERT(BackgroundHangManager::sInstance, "Not initialized");
  


  BackgroundHangManager::sInstance->Shutdown();
  BackgroundHangManager::sInstance = nullptr;
}

BackgroundHangMonitor::BackgroundHangMonitor(const char* aName,
                                             uint32_t aTimeoutMs,
                                             uint32_t aMaxTimeoutMs)
  : mThread(BackgroundHangThread::FindThread())
{
  if (!mThread) {
    mThread = new BackgroundHangThread(aName, aTimeoutMs, aMaxTimeoutMs);
  }
}

BackgroundHangMonitor::BackgroundHangMonitor()
  : mThread(BackgroundHangThread::FindThread())
{
  MOZ_ASSERT(mThread, "Thread not initialized for hang monitoring");
}

BackgroundHangMonitor::~BackgroundHangMonitor()
{
}

void
BackgroundHangMonitor::NotifyActivity()
{
  mThread->NotifyActivity();
}

void
BackgroundHangMonitor::NotifyWait()
{
  mThread->NotifyWait();
}

} 
