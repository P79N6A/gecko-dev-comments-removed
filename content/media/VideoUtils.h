





































#ifndef nsOggHacks_h
#define nsOggHacks_h















#define PR_INT64_MAX (~((PRInt64)(1) << 63))
#define PR_INT64_MIN (-PR_INT64_MAX - 1)
#define PR_UINT64_MAX (~(PRUint64)(0))




namespace mozilla {







 
class MonitorAutoExit
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
