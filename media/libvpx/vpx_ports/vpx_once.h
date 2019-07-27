









#ifndef VPX_PORTS_VPX_ONCE_H_
#define VPX_PORTS_VPX_ONCE_H_

#include "vpx_config.h"

#if CONFIG_MULTITHREAD && defined(_WIN32)
#include <windows.h>
#include <stdlib.h>
static void once(void (*func)(void))
{
    static CRITICAL_SECTION *lock;
    static LONG waiters;
    static int done;
    void *lock_ptr = &lock;

    



    if(done)
        return;

    InterlockedIncrement(&waiters);

    



    {
        
        CRITICAL_SECTION *new_lock = malloc(sizeof(CRITICAL_SECTION));
        InitializeCriticalSection(new_lock);
        if (InterlockedCompareExchangePointer(lock_ptr, new_lock, NULL) != NULL)
        {
            DeleteCriticalSection(new_lock);
            free(new_lock);
        }
    }

    



    EnterCriticalSection(lock);

    if (!done)
    {
        func();
        done = 1;
    }

    LeaveCriticalSection(lock);

    


    if(!InterlockedDecrement(&waiters))
    {
        DeleteCriticalSection(lock);
        free(lock);
        lock = NULL;
    }
}


#elif CONFIG_MULTITHREAD && HAVE_PTHREAD_H
#include <pthread.h>
static void once(void (*func)(void))
{
    static pthread_once_t lock = PTHREAD_ONCE_INIT;
    pthread_once(&lock, func);
}


#else





static void once(void (*func)(void))
{
    static int done;

    if(!done)
    {
        func();
        done = 1;
    }
}
#endif

#endif  
