





































#ifndef mozilla_ReentrantMonitor_h
#define mozilla_ReentrantMonitor_h

#include "prmon.h"

#include "mozilla/BlockingResourceBase.h"











namespace mozilla {








class NS_COM_GLUE ReentrantMonitor : BlockingResourceBase
{
public:
    



    ReentrantMonitor(const char* aName) :
        BlockingResourceBase(aName, eReentrantMonitor)
#ifdef DEBUG
        , mEntryCount(0)
#endif
    {
        MOZ_COUNT_CTOR(ReentrantMonitor);
        mReentrantMonitor = PR_NewMonitor();
        if (!mReentrantMonitor)
            NS_RUNTIMEABORT("Can't allocate mozilla::ReentrantMonitor");
    }

    


    ~ReentrantMonitor()
    {
        NS_ASSERTION(mReentrantMonitor,
                     "improperly constructed ReentrantMonitor or double free");
        PR_DestroyMonitor(mReentrantMonitor);
        mReentrantMonitor = 0;
        MOZ_COUNT_DTOR(ReentrantMonitor);
    }

#ifndef DEBUG
    



    void Enter()
    {
        PR_EnterMonitor(mReentrantMonitor);
    }

    



    void Exit()
    {
        PR_ExitMonitor(mReentrantMonitor);
    }

    


      
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT)
    {
        return PR_Wait(mReentrantMonitor, interval) == PR_SUCCESS ?
            NS_OK : NS_ERROR_FAILURE;
    }

#else
    void Enter();
    void Exit();
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT);

#endif  

    


      
    nsresult Notify()
    {
        return PR_Notify(mReentrantMonitor) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

    


      
    nsresult NotifyAll()
    {
        return PR_NotifyAll(mReentrantMonitor) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

#ifdef DEBUG
    



    void AssertCurrentThreadIn()
    {
        PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mReentrantMonitor);
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
    ReentrantMonitor();
    ReentrantMonitor(const ReentrantMonitor&);
    ReentrantMonitor& operator =(const ReentrantMonitor&);

    PRMonitor* mReentrantMonitor;
#ifdef DEBUG
    PRInt32 mEntryCount;
#endif
};








 
class NS_COM_GLUE NS_STACK_CLASS ReentrantMonitorAutoEnter
{
public:
    






    ReentrantMonitorAutoEnter(mozilla::ReentrantMonitor &aReentrantMonitor) :
        mReentrantMonitor(&aReentrantMonitor)
    {
        NS_ASSERTION(mReentrantMonitor, "null monitor");
        mReentrantMonitor->Enter();
    }
    
    ~ReentrantMonitorAutoEnter(void)
    {
        mReentrantMonitor->Exit();
    }
 
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT)
    {
       return mReentrantMonitor->Wait(interval);
    }

    nsresult Notify()
    {
        return mReentrantMonitor->Notify();
    }

    nsresult NotifyAll()
    {
        return mReentrantMonitor->NotifyAll();
    }

private:
    ReentrantMonitorAutoEnter();
    ReentrantMonitorAutoEnter(const ReentrantMonitorAutoEnter&);
    ReentrantMonitorAutoEnter& operator =(const ReentrantMonitorAutoEnter&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    mozilla::ReentrantMonitor* mReentrantMonitor;
};


} 


#endif 
