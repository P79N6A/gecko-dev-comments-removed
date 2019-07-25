







































#ifndef mozilla_Monitor_h
#define mozilla_Monitor_h

#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"

namespace mozilla {










class NS_COM_GLUE Monitor
{
public:
    Monitor(const char* aName) :
        mMutex(aName),
        mCondVar(mMutex, "[Monitor.mCondVar]")
    {}

    ~Monitor() {}

    void Lock()
    {
        mMutex.Lock();
    }

    void Unlock()
    {
        mMutex.Unlock();
    }

    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT)
    {
        return mCondVar.Wait(interval);
    }

    nsresult Notify()
    {
        return mCondVar.Notify();
    }

    nsresult NotifyAll()
    {
        return mCondVar.NotifyAll();
    }

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
    Monitor& operator =(const Monitor&);

    Mutex mMutex;
    CondVar mCondVar;
};








class NS_COM_GLUE NS_STACK_CLASS MonitorAutoLock
{
public:
    MonitorAutoLock(Monitor& aMonitor) :
        mMonitor(&aMonitor)
    {
        mMonitor->Lock();
    }
    
    ~MonitorAutoLock()
    {
        mMonitor->Unlock();
    }
 
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT)
    {
       return mMonitor->Wait(interval);
    }

    nsresult Notify()
    {
        return mMonitor->Notify();
    }

    nsresult NotifyAll()
    {
        return mMonitor->NotifyAll();
    }

private:
    MonitorAutoLock();
    MonitorAutoLock(const MonitorAutoLock&);
    MonitorAutoLock& operator =(const MonitorAutoLock&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    Monitor* mMonitor;
};








class NS_COM_GLUE NS_STACK_CLASS MonitorAutoUnlock
{
public:
    MonitorAutoUnlock(Monitor& aMonitor) :
        mMonitor(&aMonitor)
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
    MonitorAutoUnlock& operator =(const MonitorAutoUnlock&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    Monitor* mMonitor;
};

} 

#endif 
