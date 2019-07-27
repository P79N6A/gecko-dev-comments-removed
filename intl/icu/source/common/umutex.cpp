



















#include "umutex.h"

#include "unicode/utypes.h"
#include "uassert.h"
#include "cmemory.h"



static UMutex   globalMutex = U_MUTEX_INITIALIZER;






#if defined(U_USER_MUTEX_CPP)

#include U_MUTEX_XSTR(U_USER_MUTEX_CPP)

#elif U_PLATFORM_HAS_WIN32_API










#if defined U_NO_PLATFORM_ATOMICS
#error ICU on Win32 requires support for low level atomic operations.

#endif











U_NAMESPACE_BEGIN

U_COMMON_API UBool U_EXPORT2 umtx_initImplPreInit(UInitOnce &uio) {
    for (;;) {
        int32_t previousState = InterlockedCompareExchange(
#if (U_PLATFORM == U_PF_MINGW) || (U_PLATFORM == U_PF_CYGWIN) || defined(__clang__)
           (LONG volatile *) 
#endif
            &uio.fState,  
            1,            
            0);           

        if (previousState == 0) {
            return true;   
                           
        } else if (previousState == 2) {
            
            
            
            return FALSE;
        } else {
            
            
            do {
                Sleep(1);
                previousState = umtx_loadAcquire(uio.fState);
            } while (previousState == 1);
        }
    }
}




U_COMMON_API void U_EXPORT2 umtx_initImplPostInit(UInitOnce &uio) {
    umtx_storeRelease(uio.fState, 2);
}

U_NAMESPACE_END

static void winMutexInit(CRITICAL_SECTION *cs) {
    InitializeCriticalSection(cs);
    return;
}

U_CAPI void  U_EXPORT2
umtx_lock(UMutex *mutex) {
    if (mutex == NULL) {
        mutex = &globalMutex;
    }
    CRITICAL_SECTION *cs = &mutex->fCS;
    umtx_initOnce(mutex->fInitOnce, winMutexInit, cs);
    EnterCriticalSection(cs);
}

U_CAPI void  U_EXPORT2
umtx_unlock(UMutex* mutex)
{
    if (mutex == NULL) {
        mutex = &globalMutex;
    }
    LeaveCriticalSection(&mutex->fCS);
}


U_CAPI void U_EXPORT2
umtx_condBroadcast(UConditionVar *condition) {
    
    
    
    if (condition->fWaitCount == 0) {
        return;
    }
    ResetEvent(condition->fExitGate);
    SetEvent(condition->fEntryGate);
}

U_CAPI void U_EXPORT2
umtx_condSignal(UConditionVar *condition) {
    
    
    
    
    U_ASSERT(FALSE);
}

U_CAPI void U_EXPORT2
umtx_condWait(UConditionVar *condition, UMutex *mutex) {
    if (condition->fEntryGate == NULL) {
        
        
        
        
        U_ASSERT(condition->fExitGate == NULL);
        condition->fEntryGate = CreateEvent(NULL,   
                                            TRUE,   
                                            FALSE,  
                                            NULL);  
        U_ASSERT(condition->fEntryGate != NULL);
        condition->fExitGate = CreateEvent(NULL, TRUE, TRUE, NULL);
        U_ASSERT(condition->fExitGate != NULL);
    }

    condition->fWaitCount++;
    umtx_unlock(mutex);
    WaitForSingleObject(condition->fEntryGate, INFINITE); 
    umtx_lock(mutex);
    condition->fWaitCount--;
    if (condition->fWaitCount == 0) {
        
        
        ResetEvent(condition->fEntryGate);
        SetEvent(condition->fExitGate);
    } else {
        umtx_unlock(mutex);
        WaitForSingleObject(condition->fExitGate, INFINITE);
        umtx_lock(mutex);
    }
}


#elif U_PLATFORM_IMPLEMENTS_POSIX







# include <pthread.h>





U_CAPI void  U_EXPORT2
umtx_lock(UMutex *mutex) {
    if (mutex == NULL) {
        mutex = &globalMutex;
    }
    int sysErr = pthread_mutex_lock(&mutex->fMutex);
    (void)sysErr;   
    U_ASSERT(sysErr == 0);
}


U_CAPI void  U_EXPORT2
umtx_unlock(UMutex* mutex)
{
    if (mutex == NULL) {
        mutex = &globalMutex;
    }
    int sysErr = pthread_mutex_unlock(&mutex->fMutex);
    (void)sysErr;   
    U_ASSERT(sysErr == 0);
}


U_CAPI void U_EXPORT2
umtx_condWait(UConditionVar *cond, UMutex *mutex) {
    if (mutex == NULL) {
        mutex = &globalMutex;
    }
    int sysErr = pthread_cond_wait(&cond->fCondition, &mutex->fMutex);
    (void)sysErr;
    U_ASSERT(sysErr == 0);
}

U_CAPI void U_EXPORT2
umtx_condBroadcast(UConditionVar *cond) {
    int sysErr = pthread_cond_broadcast(&cond->fCondition);
    (void)sysErr;
    U_ASSERT(sysErr == 0);
}

U_CAPI void U_EXPORT2
umtx_condSignal(UConditionVar *cond) {
    int sysErr = pthread_cond_signal(&cond->fCondition);
    (void)sysErr;
    U_ASSERT(sysErr == 0);
}



U_NAMESPACE_BEGIN

static pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t initCondition = PTHREAD_COND_INITIALIZER;










U_COMMON_API UBool U_EXPORT2
umtx_initImplPreInit(UInitOnce &uio) {
    pthread_mutex_lock(&initMutex);
    int32_t state = uio.fState;
    if (state == 0) {
        umtx_storeRelease(uio.fState, 1);
        pthread_mutex_unlock(&initMutex);
        return TRUE;   
    } else {
        while (uio.fState == 1) {
            
            
            pthread_cond_wait(&initCondition, &initMutex);
        }
        pthread_mutex_unlock(&initMutex);
        U_ASSERT(uio.fState == 2);
        return FALSE;
    }
}









U_COMMON_API void U_EXPORT2
umtx_initImplPostInit(UInitOnce &uio) {
    pthread_mutex_lock(&initMutex);
    umtx_storeRelease(uio.fState, 2);
    pthread_cond_broadcast(&initCondition);
    pthread_mutex_unlock(&initMutex);
}

U_NAMESPACE_END



#else  

#error Unknown Platform

#endif  












#if defined U_NO_PLATFORM_ATOMICS
static UMutex   gIncDecMutex = U_MUTEX_INITIALIZER;

U_NAMESPACE_BEGIN

U_COMMON_API int32_t U_EXPORT2
umtx_atomic_inc(u_atomic_int32_t *p)  {
    int32_t retVal;
    umtx_lock(&gIncDecMutex);
    retVal = ++(*p);
    umtx_unlock(&gIncDecMutex);
    return retVal;
}


U_COMMON_API int32_t U_EXPORT2
umtx_atomic_dec(u_atomic_int32_t *p) {
    int32_t retVal;
    umtx_lock(&gIncDecMutex);
    retVal = --(*p);
    umtx_unlock(&gIncDecMutex);
    return retVal;
}

U_COMMON_API int32_t U_EXPORT2
umtx_loadAcquire(u_atomic_int32_t &var) {
    int32_t val = var;
    umtx_lock(&gIncDecMutex);
    umtx_unlock(&gIncDecMutex);
    return val;
}

U_COMMON_API void U_EXPORT2
umtx_storeRelease(u_atomic_int32_t &var, int32_t val) {
    umtx_lock(&gIncDecMutex);
    umtx_unlock(&gIncDecMutex);
    var = val;
}

U_NAMESPACE_END
#endif







U_DEPRECATED void U_EXPORT2
u_setMutexFunctions(const void * , UMtxInitFn *, UMtxFn *,
                    UMtxFn *,  UMtxFn *, UErrorCode *status) {
    if (U_SUCCESS(*status)) {
        *status = U_UNSUPPORTED_ERROR;
    }
    return;
}



U_DEPRECATED void U_EXPORT2
u_setAtomicIncDecFunctions(const void * , UMtxAtomicFn *, UMtxAtomicFn *,
                           UErrorCode *status) {
    if (U_SUCCESS(*status)) {
        *status = U_UNSUPPORTED_ERROR;
    }
    return;
}
