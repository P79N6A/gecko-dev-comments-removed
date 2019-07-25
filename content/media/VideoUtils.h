





































#ifndef VideoUtils_h
#define VideoUtils_h

#include "mozilla/Monitor.h"















#define PR_INT64_MAX (~((PRInt64)(1) << 63))
#define PR_INT64_MIN (-PR_INT64_MAX - 1)
#define PR_UINT64_MAX (~(PRUint64)(0))

static PRBool MulOverflow32(PRUint32 a, PRUint32 b, PRUint32 &aResult)
{
  PRUint64 rl = static_cast<PRUint64>(a) * static_cast<PRUint64>(b);

  if (rl > PR_UINT32_MAX) {
    return PR_FALSE;
  }

  aResult = static_cast<PRUint32>(rl);

  return PR_TRUE;
}



namespace mozilla {







 
class NS_STACK_CLASS MonitorAutoExit
{
public:
    









    MonitorAutoExit(mozilla::Monitor &aMonitor) :
        mMonitor(&aMonitor)
    {
        NS_ASSERTION(mMonitor, "null monitor");
        mMonitor->AssertCurrentThreadIn();
        mMonitor->Exit();
    }
    
    ~MonitorAutoExit(void)
    {
        mMonitor->Enter();
    }
 
private:
    MonitorAutoExit();
    MonitorAutoExit(const MonitorAutoExit&);
    MonitorAutoExit& operator =(const MonitorAutoExit&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    mozilla::Monitor* mMonitor;
};

} 
#endif
