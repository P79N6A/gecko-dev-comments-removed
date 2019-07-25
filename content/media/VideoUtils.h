





































#ifndef VideoUtils_h
#define VideoUtils_h

#include "mozilla/ReentrantMonitor.h"















#define PR_INT64_MAX (~((PRInt64)(1) << 63))
#define PR_INT64_MIN (-PR_INT64_MAX - 1)
#define PR_UINT64_MAX (~(PRUint64)(0))



namespace mozilla {







 
class NS_STACK_CLASS ReentrantMonitorAutoExit
{
public:
    








    ReentrantMonitorAutoExit(ReentrantMonitor& aReentrantMonitor) :
        mReentrantMonitor(&aReentrantMonitor)
    {
        NS_ASSERTION(mReentrantMonitor, "null monitor");
        mReentrantMonitor->AssertCurrentThreadIn();
        mReentrantMonitor->Exit();
    }
    
    ~ReentrantMonitorAutoExit(void)
    {
        mReentrantMonitor->Enter();
    }
 
private:
    ReentrantMonitorAutoExit();
    ReentrantMonitorAutoExit(const ReentrantMonitorAutoExit&);
    ReentrantMonitorAutoExit& operator =(const ReentrantMonitorAutoExit&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    ReentrantMonitor* mReentrantMonitor;
};

} 



PRBool AddOverflow32(PRUint32 a, PRUint32 b, PRUint32& aResult);
 



PRBool MulOverflow32(PRUint32 a, PRUint32 b, PRUint32& aResult);



PRBool AddOverflow(PRInt64 a, PRInt64 b, PRInt64& aResult);




PRBool MulOverflow(PRInt64 a, PRInt64 b, PRInt64& aResult);





PRBool SamplesToUsecs(PRInt64 aSamples, PRUint32 aRate, PRInt64& aOutUsecs);





PRBool UsecsToSamples(PRInt64 aUsecs, PRUint32 aRate, PRInt64& aOutSamples);


#define USECS_PER_S 1000000


#define USECS_PER_MS 1000

#endif
