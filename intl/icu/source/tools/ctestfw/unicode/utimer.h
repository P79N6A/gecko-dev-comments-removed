






#ifndef _UTIMER_H
#define _UTIMER_H

#include "unicode/utypes.h"

#if U_PLATFORM_HAS_WIN32_API
#   define VC_EXTRALEAN
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   if U_PLATFORM == U_PF_OS390 && !defined(__UU)
#     define __UU
#   endif
#   include <time.h>
#   include <sys/time.h> 
#   include <unistd.h> 
#endif





































































































































typedef struct UTimer UTimer;

typedef void FuntionToBeTimed(void* param);


#if U_PLATFORM_HAS_WIN32_API

    struct UTimer{
        LARGE_INTEGER start;
        LARGE_INTEGER placeHolder;
    };      
        
static    int uprv_initFrequency(UTimer* timer)
    {
        return QueryPerformanceFrequency(&timer->placeHolder);
    }
static    void uprv_start(UTimer* timer)
    {
        QueryPerformanceCounter(&timer->start);
    }
static    double uprv_delta(UTimer* timer1, UTimer* timer2){
        return ((double)(timer2->start.QuadPart - timer1->start.QuadPart))/((double)timer1->placeHolder.QuadPart);
    }
static    UBool uprv_compareFrequency(UTimer* timer1, UTimer* timer2){
        return (timer1->placeHolder.QuadPart == timer2->placeHolder.QuadPart);
    }

#else

    struct UTimer{
        struct timeval start;
        struct timeval placeHolder;
    };
    
static    int32_t uprv_initFrequency(UTimer* )
    {
        return 0;
    }
static    void uprv_start(UTimer* timer)
    {
        gettimeofday(&timer->start, 0);
    }
static    double uprv_delta(UTimer* timer1, UTimer* timer2){
        double t1, t2;

        t1 =  (double)timer1->start.tv_sec + (double)timer1->start.tv_usec/(1000*1000);
        t2 =  (double)timer2->start.tv_sec + (double)timer2->start.tv_usec/(1000*1000);
        return (t2-t1);
    }
static    UBool uprv_compareFrequency(UTimer* , UTimer* ){
        return TRUE;
    }

#endif





static inline void U_EXPORT2
utimer_getTime(UTimer* timer){
    uprv_initFrequency(timer);
    uprv_start(timer);
}









static inline double U_EXPORT2
utimer_getDeltaSeconds(UTimer* timer1, UTimer* timer2){
    if(uprv_compareFrequency(timer1,timer2)){
        return uprv_delta(timer1,timer2);
    }
    
    return -1;
}







static inline double U_EXPORT2
utimer_getElapsedSeconds(UTimer* timer){
    UTimer temp;
    utimer_getTime(&temp);
    return uprv_delta(timer,&temp);
}










static inline double U_EXPORT2
utimer_loopUntilDone(double thresholdTimeVal,
                     int32_t* loopCount, 
                     FuntionToBeTimed fn, 
                     void* param){
    UTimer timer;
    double currentVal=0;
    *loopCount = 0;
    utimer_getTime(&timer);
    for(;currentVal<thresholdTimeVal;){
        fn(param);
        currentVal = utimer_getElapsedSeconds(&timer);
        (*loopCount)++;
    }
    return currentVal;
}

#endif

