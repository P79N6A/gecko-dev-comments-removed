








#include "SkEvent.h"
#include "utils/threads.h"
#include <stdio.h>

using namespace android;

Mutex gEventQMutex;
Condition gEventQCondition;

void SkEvent::SignalNonEmptyQueue()
{
    gEventQCondition.broadcast();
}



#ifdef FMS_ARCH_ANDROID_ARM



void SkEvent::SignalQueueTimer(SkMSec delay)
{
}

void SkEvent_start_timer_thread()
{
}

void SkEvent_stop_timer_thread()
{
}

#else

#include <pthread.h>
#include <errno.h>

static pthread_t        gTimerThread;
static pthread_mutex_t  gTimerMutex;
static pthread_cond_t   gTimerCond;
static timespec         gTimeSpec;

static void* timer_event_thread_proc(void*)
{
    for (;;)
    {
        int status;
        
        pthread_mutex_lock(&gTimerMutex);

        timespec spec = gTimeSpec;
        
        
        gTimeSpec.tv_sec = 0;
        gTimeSpec.tv_nsec = 0;

        if (spec.tv_sec == 0 && spec.tv_nsec == 0)
            status = pthread_cond_wait(&gTimerCond, &gTimerMutex);
        else
            status = pthread_cond_timedwait(&gTimerCond, &gTimerMutex, &spec);
        
        if (status == 0)    
        {
            pthread_mutex_unlock(&gTimerMutex);
        }
        else
        {
            SkASSERT(status == ETIMEDOUT);  
            
            
            gEventQCondition.broadcast();
        }
    }
    return 0;
}

#define kThousand   (1000)
#define kMillion    (kThousand * kThousand)
#define kBillion    (kThousand * kThousand * kThousand)

void SkEvent::SignalQueueTimer(SkMSec delay)
{
    pthread_mutex_lock(&gTimerMutex);

    if (delay)
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);

        
        if (tv.tv_usec >= kMillion)
        {
            tv.tv_sec += tv.tv_usec / kMillion;
            tv.tv_usec %= kMillion;
        }

        
        gTimeSpec.tv_nsec   = (tv.tv_usec + (delay % kThousand) * kThousand) * kThousand;
        gTimeSpec.tv_sec    = (tv.tv_sec + (delay / kThousand) * kThousand) * kThousand;
        
        
        if ((unsigned long)gTimeSpec.tv_nsec >= kBillion)
        {
            gTimeSpec.tv_nsec -= kBillion;
            gTimeSpec.tv_sec += 1;
            SkASSERT((unsigned long)gTimeSpec.tv_nsec < kBillion);
        }

    
    }
    else    
    {
        gTimeSpec.tv_nsec = 0;
        gTimeSpec.tv_sec = 0;
    }

    pthread_mutex_unlock(&gTimerMutex);
    pthread_cond_signal(&gTimerCond);
}

void SkEvent_start_timer_thread()
{
    int             status;
    pthread_attr_t  attr;
    
    status = pthread_attr_init(&attr);
    SkASSERT(status == 0);
    status = pthread_create(&gTimerThread, &attr, timer_event_thread_proc, 0);
    SkASSERT(status == 0);
}

void SkEvent_stop_timer_thread()
{
    int status = pthread_cancel(gTimerThread);
    SkASSERT(status == 0);
}

#endif
