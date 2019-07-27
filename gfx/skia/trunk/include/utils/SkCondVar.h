






#ifndef SkCondVar_DEFINED
#define SkCondVar_DEFINED




#include "SkTypes.h"

#ifdef SK_USE_POSIX_THREADS
#include <pthread.h>
#elif defined(SK_BUILD_FOR_WIN32)
#include <windows.h>
#else



#warning "Thread safety class SkCondVar has no implementation!"
#endif








class SkCondVar {
public:
    SkCondVar();
    ~SkCondVar();

    


    void lock();

    


    void unlock();

    




    void wait();

    



    void signal();

    



    void broadcast();

private:
#ifdef SK_USE_POSIX_THREADS
    pthread_mutex_t  fMutex;
    pthread_cond_t   fCond;
#elif defined(SK_BUILD_FOR_WIN32)
    CRITICAL_SECTION   fCriticalSection;
    CONDITION_VARIABLE fCondition;
#endif
};

#endif
