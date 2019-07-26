






#ifndef jsmonitor_h___
#define jsmonitor_h___

#include <stdlib.h>
#include "mozilla/Util.h"
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
    Monitor();
    ~Monitor();

    bool init();
};

class AutoLockMonitor
{
private:
    Monitor &monitor;

public:
    AutoLockMonitor(Monitor &monitor) : monitor(monitor) {
#ifdef JS_THREADSAFE
        PR_Lock(monitor.lock_);
#endif
    }

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
    Monitor &monitor;

  public:
    AutoUnlockMonitor(Monitor &monitor) : monitor(monitor) {
#ifdef JS_THREADSAFE
        PR_Unlock(monitor.lock_);
#endif
    }
    ~AutoUnlockMonitor() {
#ifdef JS_THREADSAFE
        PR_Lock(monitor.lock_);
#endif
    }
};

}

#endif 
