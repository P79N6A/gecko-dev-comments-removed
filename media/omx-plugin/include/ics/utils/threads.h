















#ifndef _LIBS_UTILS_THREADS_H
#define _LIBS_UTILS_THREADS_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>
#include <system/graphics.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
#endif




#ifdef __cplusplus
extern "C" {
#endif

typedef void* android_thread_id_t;

typedef int (*android_thread_func_t)(void*);

enum {
    













    ANDROID_PRIORITY_LOWEST         =  19,

    
    ANDROID_PRIORITY_BACKGROUND     =  10,
    
    
    ANDROID_PRIORITY_NORMAL         =   0,
    
    
    ANDROID_PRIORITY_FOREGROUND     =  -2,

    
    ANDROID_PRIORITY_DISPLAY        =  -4,
    
    
    ANDROID_PRIORITY_URGENT_DISPLAY =  HAL_PRIORITY_URGENT_DISPLAY,
    
    
    ANDROID_PRIORITY_AUDIO          = -16,
    
    
    ANDROID_PRIORITY_URGENT_AUDIO   = -19,

    

    ANDROID_PRIORITY_HIGHEST        = -20,

    ANDROID_PRIORITY_DEFAULT        = ANDROID_PRIORITY_NORMAL,
    ANDROID_PRIORITY_MORE_FAVORABLE = -1,
    ANDROID_PRIORITY_LESS_FAVORABLE = +1,
};

enum {
    ANDROID_TGROUP_DEFAULT          = 0,
    ANDROID_TGROUP_BG_NONINTERACT   = 1,
    ANDROID_TGROUP_FG_BOOST         = 2,
    ANDROID_TGROUP_MAX              = ANDROID_TGROUP_FG_BOOST,
};


extern int androidCreateThread(android_thread_func_t, void *);


extern int androidCreateThreadEtc(android_thread_func_t entryFunction,
                                  void *userData,
                                  const char* threadName,
                                  int32_t threadPriority,
                                  size_t threadStackSize,
                                  android_thread_id_t *threadId);


extern android_thread_id_t androidGetThreadId();



extern int androidCreateRawThreadEtc(android_thread_func_t entryFunction,
                                     void *userData,
                                     const char* threadName,
                                     int32_t threadPriority,
                                     size_t threadStackSize,
                                     android_thread_id_t *threadId);



typedef int (*android_create_thread_fn)(android_thread_func_t entryFunction,
                                        void *userData,
                                        const char* threadName,
                                        int32_t threadPriority,
                                        size_t threadStackSize,
                                        android_thread_id_t *threadId);

extern void androidSetCreateThreadFunc(android_create_thread_fn func);





extern pid_t androidGetTid();





extern int androidSetThreadSchedulingGroup(pid_t tid, int grp);





extern int androidSetThreadPriority(pid_t tid, int prio);



extern int androidGetThreadPriority(pid_t tid);






extern int androidGetThreadSchedulingGroup(pid_t tid);

#ifdef __cplusplus
}
#endif




#ifdef __cplusplus

#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>

namespace android {

typedef android_thread_id_t thread_id_t;

typedef android_thread_func_t thread_func_t;

enum {
    PRIORITY_LOWEST         = ANDROID_PRIORITY_LOWEST,
    PRIORITY_BACKGROUND     = ANDROID_PRIORITY_BACKGROUND,
    PRIORITY_NORMAL         = ANDROID_PRIORITY_NORMAL,
    PRIORITY_FOREGROUND     = ANDROID_PRIORITY_FOREGROUND,
    PRIORITY_DISPLAY        = ANDROID_PRIORITY_DISPLAY,
    PRIORITY_URGENT_DISPLAY = ANDROID_PRIORITY_URGENT_DISPLAY,
    PRIORITY_AUDIO          = ANDROID_PRIORITY_AUDIO,
    PRIORITY_URGENT_AUDIO   = ANDROID_PRIORITY_URGENT_AUDIO,
    PRIORITY_HIGHEST        = ANDROID_PRIORITY_HIGHEST,
    PRIORITY_DEFAULT        = ANDROID_PRIORITY_DEFAULT,
    PRIORITY_MORE_FAVORABLE = ANDROID_PRIORITY_MORE_FAVORABLE,
    PRIORITY_LESS_FAVORABLE = ANDROID_PRIORITY_LESS_FAVORABLE,
};


inline bool createThread(thread_func_t f, void *a) {
    return androidCreateThread(f, a) ? true : false;
}


inline bool createThreadEtc(thread_func_t entryFunction,
                            void *userData,
                            const char* threadName = "android:unnamed_thread",
                            int32_t threadPriority = PRIORITY_DEFAULT,
                            size_t threadStackSize = 0,
                            thread_id_t *threadId = 0)
{
    return androidCreateThreadEtc(entryFunction, userData, threadName,
        threadPriority, threadStackSize, threadId) ? true : false;
}


inline thread_id_t getThreadId() {
    return androidGetThreadId();
}









class Mutex {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };
    
                Mutex();
                Mutex(const char* name);
                Mutex(int type, const char* name = NULL);
                ~Mutex();

    
    status_t    lock();
    void        unlock();

    
    status_t    tryLock();

    
    
    class Autolock {
    public:
        inline Autolock(Mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline Autolock(Mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~Autolock() { mLock.unlock(); }
    private:
        Mutex& mLock;
    };

private:
    friend class Condition;
    
    
                Mutex(const Mutex&);
    Mutex&      operator = (const Mutex&);
    
#if defined(HAVE_PTHREADS)
    pthread_mutex_t mMutex;
#else
    void    _init();
    void*   mState;
#endif
};

#if defined(HAVE_PTHREADS)

inline Mutex::Mutex() {
    pthread_mutex_init(&mMutex, NULL);
}
inline Mutex::Mutex(const char* name) {
    pthread_mutex_init(&mMutex, NULL);
}
inline Mutex::Mutex(int type, const char* name) {
    if (type == SHARED) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&mMutex, NULL);
    }
}
inline Mutex::~Mutex() {
    pthread_mutex_destroy(&mMutex);
}
inline status_t Mutex::lock() {
    return -pthread_mutex_lock(&mMutex);
}
inline void Mutex::unlock() {
    pthread_mutex_unlock(&mMutex);
}
inline status_t Mutex::tryLock() {
    return -pthread_mutex_trylock(&mMutex);
}

#endif 






 
typedef Mutex::Autolock AutoMutex;



#if defined(HAVE_PTHREADS)







class RWLock {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

                RWLock();
                RWLock(const char* name);
                RWLock(int type, const char* name = NULL);
                ~RWLock();

    status_t    readLock();
    status_t    tryReadLock();
    status_t    writeLock();
    status_t    tryWriteLock();
    void        unlock();

    class AutoRLock {
    public:
        inline AutoRLock(RWLock& rwlock) : mLock(rwlock)  { mLock.readLock(); }
        inline ~AutoRLock() { mLock.unlock(); }
    private:
        RWLock& mLock;
    };

    class AutoWLock {
    public:
        inline AutoWLock(RWLock& rwlock) : mLock(rwlock)  { mLock.writeLock(); }
        inline ~AutoWLock() { mLock.unlock(); }
    private:
        RWLock& mLock;
    };

private:
    
                RWLock(const RWLock&);
   RWLock&      operator = (const RWLock&);

   pthread_rwlock_t mRWLock;
};

inline RWLock::RWLock() {
    pthread_rwlock_init(&mRWLock, NULL);
}
inline RWLock::RWLock(const char* name) {
    pthread_rwlock_init(&mRWLock, NULL);
}
inline RWLock::RWLock(int type, const char* name) {
    if (type == SHARED) {
        pthread_rwlockattr_t attr;
        pthread_rwlockattr_init(&attr);
        pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_rwlock_init(&mRWLock, &attr);
        pthread_rwlockattr_destroy(&attr);
    } else {
        pthread_rwlock_init(&mRWLock, NULL);
    }
}
inline RWLock::~RWLock() {
    pthread_rwlock_destroy(&mRWLock);
}
inline status_t RWLock::readLock() {
    return -pthread_rwlock_rdlock(&mRWLock);
}
inline status_t RWLock::tryReadLock() {
    return -pthread_rwlock_tryrdlock(&mRWLock);
}
inline status_t RWLock::writeLock() {
    return -pthread_rwlock_wrlock(&mRWLock);
}
inline status_t RWLock::tryWriteLock() {
    return -pthread_rwlock_trywrlock(&mRWLock);
}
inline void RWLock::unlock() {
    pthread_rwlock_unlock(&mRWLock);
}

#endif











class Condition {
public:
    enum {
        PRIVATE = 0,
        SHARED = 1
    };

    Condition();
    Condition(int type);
    ~Condition();
    
    status_t wait(Mutex& mutex);
    
    status_t waitRelative(Mutex& mutex, nsecs_t reltime);
    
    void signal();
    
    void broadcast();

private:
#if defined(HAVE_PTHREADS)
    pthread_cond_t mCond;
#else
    void*   mState;
#endif
};

#if defined(HAVE_PTHREADS)

inline Condition::Condition() {
    pthread_cond_init(&mCond, NULL);
}
inline Condition::Condition(int type) {
    if (type == SHARED) {
        pthread_condattr_t attr;
        pthread_condattr_init(&attr);
        pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_cond_init(&mCond, &attr);
        pthread_condattr_destroy(&attr);
    } else {
        pthread_cond_init(&mCond, NULL);
    }
}
inline Condition::~Condition() {
    pthread_cond_destroy(&mCond);
}
inline status_t Condition::wait(Mutex& mutex) {
    return -pthread_cond_wait(&mCond, &mutex.mMutex);
}
inline status_t Condition::waitRelative(Mutex& mutex, nsecs_t reltime) {
#if defined(HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE)
    struct timespec ts;
    ts.tv_sec  = reltime/1000000000;
    ts.tv_nsec = reltime%1000000000;
    return -pthread_cond_timedwait_relative_np(&mCond, &mutex.mMutex, &ts);
#else 
    struct timespec ts;
#if defined(HAVE_POSIX_CLOCKS)
    clock_gettime(CLOCK_REALTIME, &ts);
#else 
    
    struct timeval t;
    gettimeofday(&t, NULL);
    ts.tv_sec = t.tv_sec;
    ts.tv_nsec= t.tv_usec*1000;
#endif 
    ts.tv_sec += reltime/1000000000;
    ts.tv_nsec+= reltime%1000000000;
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec  += 1;
    }
    return -pthread_cond_timedwait(&mCond, &mutex.mMutex, &ts);
#endif 
}
inline void Condition::signal() {
    pthread_cond_signal(&mCond);
}
inline void Condition::broadcast() {
    pthread_cond_broadcast(&mCond);
}

#endif 







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
#if HAVE_ANDROID_OS
            int             mTid;
#endif
};


}; 

#endif

#endif
