






#ifndef jsmonitor_h___
#define jsmonitor_h___

#include <stdlib.h>
#include "mozilla/Util.h"
#include "js/Utility.h"
#include "prlock.h"
#include "prcvar.h"

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
        PR_Lock(monitor.lock_);
    }

    ~AutoLockMonitor() {
        PR_Unlock(monitor.lock_);
    }

    void wait() {
        mozilla::DebugOnly<PRStatus> status =
          PR_WaitCondVar(monitor.condVar_, PR_INTERVAL_NO_TIMEOUT);
        JS_ASSERT(status == PR_SUCCESS);
    }

    void notify() {
        PR_NotifyCondVar(monitor.condVar_);
    }

    void notifyAll() {
        PR_NotifyAllCondVar(monitor.condVar_);
    }
};

class AutoUnlockMonitor
{
  private:
    Monitor &monitor;

  public:
    AutoUnlockMonitor(Monitor &monitor) : monitor(monitor) { PR_Unlock(monitor.lock_); }
    ~AutoUnlockMonitor() { PR_Lock(monitor.lock_); }
};

}

#endif 
