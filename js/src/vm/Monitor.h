





#ifndef vm_Monitor_h
#define vm_Monitor_h

#include "mozilla/DebugOnly.h"

#include <stddef.h>

#include "jslock.h"

#include "js/Utility.h"

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
      : lock_(nullptr),
        condVar_(nullptr)
    { }

    ~Monitor() {
        if (lock_)
            PR_DestroyLock(lock_);
        if (condVar_)
            PR_DestroyCondVar(condVar_);
    }

    bool init();
};

class AutoLockMonitor
{
  private:
    Monitor &monitor;

  public:
    explicit AutoLockMonitor(Monitor &monitor)
      : monitor(monitor)
    {
        PR_Lock(monitor.lock_);
    }

    ~AutoLockMonitor() {
        PR_Unlock(monitor.lock_);
    }

    bool isFor(Monitor &other) const {
        return monitor.lock_ == other.lock_;
    }

    void wait(PRCondVar *condVar) {
        mozilla::DebugOnly<PRStatus> status =
          PR_WaitCondVar(condVar, PR_INTERVAL_NO_TIMEOUT);
        MOZ_ASSERT(status == PR_SUCCESS);
    }

    void wait() {
        wait(monitor.condVar_);
    }

    void notify(PRCondVar *condVar) {
        mozilla::DebugOnly<PRStatus> status = PR_NotifyCondVar(condVar);
        MOZ_ASSERT(status == PR_SUCCESS);
    }

    void notify() {
        notify(monitor.condVar_);
    }

    void notifyAll(PRCondVar *condVar) {
        mozilla::DebugOnly<PRStatus> status = PR_NotifyAllCondVar(monitor.condVar_);
        MOZ_ASSERT(status == PR_SUCCESS);
    }

    void notifyAll() {
        notifyAll(monitor.condVar_);
    }
};

class AutoUnlockMonitor
{
  private:
    Monitor &monitor;

  public:
    explicit AutoUnlockMonitor(Monitor &monitor)
      : monitor(monitor)
    {
        PR_Unlock(monitor.lock_);
    }

    ~AutoUnlockMonitor() {
        PR_Lock(monitor.lock_);
    }

    bool isFor(Monitor &other) const {
        return monitor.lock_ == other.lock_;
    }
};

} 

#endif 
