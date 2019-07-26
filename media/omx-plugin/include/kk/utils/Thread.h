















#ifndef _LIBS_UTILS_THREAD_H
#define _LIBS_UTILS_THREAD_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
#endif

#include <utils/Condition.h>
#include <utils/Errors.h>
#include <utils/Mutex.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>
#include <utils/ThreadDefs.h>


namespace android {


class Thread : virtual public RefBase
{
public:
    
    
                        Thread(bool canCallJava = true);
    virtual             ~Thread();

    
    virtual status_t    run(    const char* name = 0,
                                int32_t priority = PRIORITY_DEFAULT,
                                size_t stack = 0);
    
    
    
    
    virtual void        requestExit();

    
    virtual status_t    readyToRun();
    
    
    
    
    
            status_t    requestExitAndWait();

    
    
            status_t    join();

    
            bool        isRunning() const;

#ifdef HAVE_ANDROID_OS
    
    
            pid_t       getTid() const;
#endif

protected:
    
            bool        exitPending() const;
    
private:
    
    
    
    
    
    virtual bool        threadLoop() = 0;

private:
    Thread& operator=(const Thread&);
    static  int             _threadLoop(void* user);
    const   bool            mCanCallJava;
    
            thread_id_t     mThread;
    mutable Mutex           mLock;
            Condition       mThreadExitedCondition;
            status_t        mStatus;
    
    volatile bool           mExitPending;
    volatile bool           mRunning;
            sp<Thread>      mHoldSelf;
#ifdef HAVE_ANDROID_OS
    
    
            pid_t           mTid;
#endif
};


}; 


#endif 

