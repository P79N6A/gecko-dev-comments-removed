






#ifndef Monitor_h__
#define Monitor_h__

#include <stdlib.h>

#include "mozilla/DebugOnly.h"

#include "js/Utility.h"
#include "jslock.h"

namespace js {






class Monitor
{
  protected:
    friend class AutoLockMonitor;
    friend class AutoUnlockMonitor;

    PRLock *lock_;
    PRCondVar *condVar_;

  public:
    Monitor()
      : lock_(NULL),
        condVar_(NULL)
    { }

    ~Monitor() {
#ifdef JS_THREADSAFE
        if (lock_)
            PR_DestroyLock(lock_);
        if (condVar_)
            PR_DestroyCondVar(condVar_);
#endif
    }

    bool init();
};

class AutoLockMonitor
{
  private:
#ifdef JS_THREADSAFE
    Monitor &monitor;
#endif

  public:
    AutoLockMonitor(Monitor &monitor)
#ifdef JS_THREADSAFE
      : monitor(monitor)
    {
        PR_Lock(monitor.lock_);
    }
#else
    {}
#endif

    ~AutoLockMonitor() {
#ifdef JS_THREADSAFE
        PR_Unlock(monitor.lock_);
#endif
    }

    void wait() {
#ifdef JS_THREADSAFE
        mozilla::DebugOnly<PRStatus> status =
          PR_WaitCondVar(monitor.condVar_, PR_INTERVAL_NO_TIMEOUT);
        JS_ASSERT(status == PR_SUCCESS);
#endif
    }

    void notify() {
#ifdef JS_THREADSAFE
        PR_NotifyCondVar(monitor.condVar_);
#endif
    }

    void notifyAll() {
#ifdef JS_THREADSAFE
        PR_NotifyAllCondVar(monitor.condVar_);
#endif
    }
};

class AutoUnlockMonitor
{
  private:
#ifdef JS_THREADSAFE
    Monitor &monitor;
#endif

  public:
    AutoUnlockMonitor(Monitor &monitor)
#ifdef JS_THREADSAFE
      : monitor(monitor)
    {
        PR_Unlock(monitor.lock_);
    }
#else
    {}
#endif

    ~AutoUnlockMonitor() {
#ifdef JS_THREADSAFE
        PR_Lock(monitor.lock_);
#endif
    }
};

} 

#endif 
