





































#ifndef mozilla_Monitor_h
#define mozilla_Monitor_h

#include "prmon.h"

#include "mozilla/BlockingResourceBase.h"











namespace mozilla {








class NS_COM_GLUE Monitor : BlockingResourceBase
{
public:
    






    Monitor(const char* aName) :
        BlockingResourceBase(aName, eMonitor)
#ifdef DEBUG
        , mEntryCount(0)
#endif
    {
        mMonitor = PR_NewMonitor();
        if (!mMonitor)
            NS_RUNTIMEABORT("Can't allocate mozilla::Monitor");
    }

    


    ~Monitor()
    {
        NS_ASSERTION(mMonitor,
                     "improperly constructed Monitor or double free");
        PR_DestroyMonitor(mMonitor);
        mMonitor = 0;
    }

#ifndef DEBUG
    



    void Enter()
    {
        PR_EnterMonitor(mMonitor);
    }

    



    void Exit()
    {
        PR_ExitMonitor(mMonitor);
    }

    


      
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT)
    {
        return PR_Wait(mMonitor, interval) == PR_SUCCESS ?
            NS_OK : NS_ERROR_FAILURE;
    }

#else
    void Enter();
    void Exit();
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT);

#endif  

    


      
    nsresult Notify()
    {
        return PR_Notify(mMonitor) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

    


      
    nsresult NotifyAll()
    {
        return PR_NotifyAll(mMonitor) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

#ifdef DEBUG
    



    void AssertCurrentThreadIn()
    {
        PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);
    }

    



    void AssertNotCurrentThreadIn()
    {
        
    }

#else
    void AssertCurrentThreadIn()
    {
    }
    void AssertNotCurrentThreadIn()
    {
    }

#endif  

private:
    Monitor();
    Monitor(const Monitor&);
    Monitor& operator =(const Monitor&);

    PRMonitor* mMonitor;
#ifdef DEBUG
    PRInt32 mEntryCount;
#endif
};








 
class NS_COM_GLUE NS_STACK_CLASS MonitorAutoEnter
{
public:
    







    MonitorAutoEnter(mozilla::Monitor &aMonitor) :
        mMonitor(&aMonitor)
    {
        NS_ASSERTION(mMonitor, "null monitor");
        mMonitor->Enter();
    }
    
    ~MonitorAutoEnter(void)
    {
        mMonitor->Exit();
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
        return mMonitor->Notify();
    }

private:
    MonitorAutoEnter();
    MonitorAutoEnter(const MonitorAutoEnter&);
    MonitorAutoEnter& operator =(const MonitorAutoEnter&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    mozilla::Monitor* mMonitor;
};


} 


#endif 
