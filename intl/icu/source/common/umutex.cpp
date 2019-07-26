



















#include "unicode/utypes.h"
#include "uassert.h"
#include "ucln_cmn.h"






#if U_PLATFORM_HAS_WIN32_API
    
#   undef POSIX
#elif U_PLATFORM_IMPLEMENTS_POSIX
#   define POSIX
#else
#   undef POSIX
#endif

#if defined(POSIX)
# include <pthread.h> 
#endif 

#if U_PLATFORM_HAS_WIN32_API
# define WIN32_LEAN_AND_MEAN
# define VC_EXTRALEAN
# define NOUSER
# define NOSERVICE
# define NOIME
# define NOMCX
# include <windows.h>
#endif

#include "umutex.h"
#include "cmemory.h"

#if U_PLATFORM_HAS_WIN32_API
#define SYNC_COMPARE_AND_SWAP(dest, oldval, newval) \
            InterlockedCompareExchangePointer(dest, newval, oldval)

#elif defined(POSIX)
#if (U_HAVE_GCC_ATOMICS == 1)
#define SYNC_COMPARE_AND_SWAP(dest, oldval, newval) \
            __sync_val_compare_and_swap(dest, oldval, newval)
#else
#define SYNC_COMPARE_AND_SWAP(dest, oldval, newval) \
            mutexed_compare_and_swap(dest, newval, oldval)
#endif

#else   

#define SYNC_COMPARE_AND_SWAP(dest, oldval, newval) \
            mutexed_compare_and_swap(dest, newval, oldval)
#endif

static void *mutexed_compare_and_swap(void **dest, void *newval, void *oldval);


static UMutex   globalMutex = U_MUTEX_INITIALIZER;



static UMutex   implMutex = U_MUTEX_INITIALIZER;      










static const int MUTEX_LIST_LIMIT = 100;
static UMutex *gMutexList[MUTEX_LIST_LIMIT];
static int gMutexListSize = 0;







static UMtxInitFn    *pMutexInitFn    = NULL;
static UMtxFn        *pMutexDestroyFn = NULL;
static UMtxFn        *pMutexLockFn    = NULL;
static UMtxFn        *pMutexUnlockFn  = NULL;
static const void    *gMutexContext   = NULL;




static void usrMutexCleanup() {
    if (pMutexDestroyFn != NULL) {
        for (int i = 0; i < gMutexListSize; i++) {
            UMutex *m = gMutexList[i];
            U_ASSERT(m->fInitialized);
            (*pMutexDestroyFn)(gMutexContext, &m->fUserMutex);
            m->fInitialized = FALSE;
        }
        (*pMutexDestroyFn)(gMutexContext, &globalMutex.fUserMutex);
        (*pMutexDestroyFn)(gMutexContext, &implMutex.fUserMutex);
    }
    gMutexListSize  = 0;
    pMutexInitFn    = NULL;
    pMutexDestroyFn = NULL;
    pMutexLockFn    = NULL;
    pMutexUnlockFn  = NULL;
    gMutexContext   = NULL;
}










static void usrMutexLock(UMutex *mutex) {
    UErrorCode status = U_ZERO_ERROR;
    if (!(mutex == &implMutex || mutex == &globalMutex)) {
        umtx_lock(&implMutex);
        if (!mutex->fInitialized) {
            (*pMutexInitFn)(gMutexContext, &mutex->fUserMutex, &status);
            U_ASSERT(U_SUCCESS(status));
            mutex->fInitialized = TRUE;
            U_ASSERT(gMutexListSize < MUTEX_LIST_LIMIT);
            if (gMutexListSize < MUTEX_LIST_LIMIT) {
                gMutexList[gMutexListSize] = mutex;
                ++gMutexListSize;
            }
        }
        umtx_unlock(&implMutex);
    }
    (*pMutexLockFn)(gMutexContext, &mutex->fUserMutex);
}
        


#if defined(POSIX)








U_CAPI void  U_EXPORT2
umtx_lock(UMutex *mutex) {
    if (mutex == NULL) {
        mutex = &globalMutex;
    }
    if (pMutexLockFn) {
        usrMutexLock(mutex);
    } else {
        #if U_DEBUG
            
            int sysErr = pthread_mutex_lock(&mutex->fMutex);
            U_ASSERT(sysErr == 0);
        #else
            pthread_mutex_lock(&mutex->fMutex);
        #endif
    }
}


U_CAPI void  U_EXPORT2
umtx_unlock(UMutex* mutex)
{
    if (mutex == NULL) {
        mutex = &globalMutex;
    }
    if (pMutexUnlockFn) {
        (*pMutexUnlockFn)(gMutexContext, &mutex->fUserMutex);
    } else {
        #if U_DEBUG
            
            int sysErr = pthread_mutex_unlock(&mutex->fMutex);
            U_ASSERT(sysErr == 0);
        #else
            pthread_mutex_unlock(&mutex->fMutex);
        #endif
    }
}

#elif U_PLATFORM_HAS_WIN32_API











typedef UBool (*U_PINIT_ONCE_FN) (
  U_INIT_ONCE     *initOnce,
  void            *parameter,
  void            **context
);

UBool u_InitOnceExecuteOnce(
  U_INIT_ONCE     *initOnce,
  U_PINIT_ONCE_FN initFn,
  void            *parameter,
  void            **context) {
      for (;;) {
          long previousState = InterlockedCompareExchange( 
              &initOnce->fState,  
              1,                  
              0);                 
          if (previousState == 2) {
              
              if (context != NULL) {
                  *context = initOnce->fContext;
              }
              return TRUE;
          }
          if (previousState == 1) {
              
              
              Sleep(1);
              continue;
          }
           
          
          U_ASSERT(previousState == 0);
          U_ASSERT(initOnce->fState == 1);
          UBool success = (*initFn)(initOnce, parameter, &initOnce->fContext);
          U_ASSERT(success); 

          
          
          
          previousState = InterlockedCompareExchange(&initOnce->fState, 2, 1);
          U_ASSERT(previousState == 1);
          
      }
};

static UBool winMutexInit(U_INIT_ONCE *initOnce, void *param, void **context) {
    UMutex *mutex = static_cast<UMutex *>(param);
    U_ASSERT(sizeof(CRITICAL_SECTION) <= sizeof(mutex->fCS));
    InitializeCriticalSection((CRITICAL_SECTION *)mutex->fCS);
    return TRUE;
}




U_CAPI void  U_EXPORT2
umtx_lock(UMutex *mutex) {
    if (mutex == NULL) {
        mutex = &globalMutex;
    }
    if (pMutexLockFn) {
        usrMutexLock(mutex);
    } else {
        u_InitOnceExecuteOnce(&mutex->fInitOnce, winMutexInit, mutex, NULL);
        EnterCriticalSection((CRITICAL_SECTION *)mutex->fCS);
    }
}

U_CAPI void  U_EXPORT2
umtx_unlock(UMutex* mutex)
{
    if (mutex == NULL) {
        mutex = &globalMutex;
    }
    if (pMutexUnlockFn) {
        (*pMutexUnlockFn)(gMutexContext, &mutex->fUserMutex);
    } else {
        LeaveCriticalSection((CRITICAL_SECTION *)mutex->fCS);
    }
}

#endif  


U_CAPI void U_EXPORT2 
u_setMutexFunctions(const void *context, UMtxInitFn *i, UMtxFn *d, UMtxFn *l, UMtxFn *u,
                    UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }

    
    if (i==NULL || d==NULL || l==NULL || u==NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    
    if (cmemory_inUse()) {
        *status = U_INVALID_STATE_ERROR;
        return;
    }

    
    
    
    
    

    usrMutexCleanup();
    
    
    pMutexInitFn    = i;
    pMutexDestroyFn = d;
    pMutexLockFn    = l;
    pMutexUnlockFn  = u;
    gMutexContext   = context;
    gMutexListSize  = 0;

    


    (*pMutexInitFn)(gMutexContext, &globalMutex.fUserMutex, status);
    globalMutex.fInitialized = TRUE;
    (*pMutexInitFn)(gMutexContext, &implMutex.fUserMutex, status);
    implMutex.fInitialized = TRUE;
}






static void *mutexed_compare_and_swap(void **dest, void *newval, void *oldval) {
    umtx_lock(&implMutex);
    void *temp = *dest;
    if (temp == oldval) {
        *dest = newval;
    }
    umtx_unlock(&implMutex);
    
    return temp;
}












static UMtxAtomicFn  *pIncFn = NULL;
static UMtxAtomicFn  *pDecFn = NULL;
static const void *gIncDecContext  = NULL;

#if defined (POSIX) && (U_HAVE_GCC_ATOMICS == 0)
static UMutex   gIncDecMutex = U_MUTEX_INITIALIZER;
#endif

U_CAPI int32_t U_EXPORT2
umtx_atomic_inc(int32_t *p)  {
    int32_t retVal;
    if (pIncFn) {
        retVal = (*pIncFn)(gIncDecContext, p);
    } else {
        #if U_PLATFORM_HAS_WIN32_API
            retVal = InterlockedIncrement((LONG*)p);
        #elif defined(USE_MAC_OS_ATOMIC_INCREMENT)
            retVal = OSAtomicIncrement32Barrier(p);
        #elif (U_HAVE_GCC_ATOMICS == 1)
            retVal = __sync_add_and_fetch(p, 1);
        #elif defined (POSIX)
            umtx_lock(&gIncDecMutex);
            retVal = ++(*p);
            umtx_unlock(&gIncDecMutex);
        #else
            
            retVal = ++(*p);
        #endif
    }
    return retVal;
}

U_CAPI int32_t U_EXPORT2
umtx_atomic_dec(int32_t *p) {
    int32_t retVal;
    if (pDecFn) {
        retVal = (*pDecFn)(gIncDecContext, p);
    } else {
        #if U_PLATFORM_HAS_WIN32_API
            retVal = InterlockedDecrement((LONG*)p);
        #elif defined(USE_MAC_OS_ATOMIC_INCREMENT)
            retVal = OSAtomicDecrement32Barrier(p);
        #elif (U_HAVE_GCC_ATOMICS == 1)
            retVal = __sync_sub_and_fetch(p, 1);
        #elif defined (POSIX)
            umtx_lock(&gIncDecMutex);
            retVal = --(*p);
            umtx_unlock(&gIncDecMutex);
        #else
            
            retVal = --(*p);
        #endif
    }
    return retVal;
}



U_CAPI void U_EXPORT2
u_setAtomicIncDecFunctions(const void *context, UMtxAtomicFn *ip, UMtxAtomicFn *dp,
                                UErrorCode *status) {
    if (U_FAILURE(*status)) {
        return;
    }
    
    if (ip==NULL || dp==NULL) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    if (cmemory_inUse()) {
        *status = U_INVALID_STATE_ERROR;
        return;
    }

    pIncFn = ip;
    pDecFn = dp;
    gIncDecContext = context;

#if U_DEBUG
    {
        int32_t   testInt = 0;
        U_ASSERT(umtx_atomic_inc(&testInt) == 1);     
        U_ASSERT(testInt == 1);
        U_ASSERT(umtx_atomic_dec(&testInt) == 0);
        U_ASSERT(testInt == 0);
    }
#endif
}







U_CFUNC UBool umtx_cleanup(void) {
    

    void *pv = &globalMutex;
    mutexed_compare_and_swap(&pv, NULL, NULL);
    usrMutexCleanup();
           
    pIncFn          = NULL;
    pDecFn          = NULL;
    gIncDecContext  = NULL;

    return TRUE;
}
