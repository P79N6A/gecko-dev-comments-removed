
















#ifndef UMUTEX_H
#define UMUTEX_H

#include "unicode/utypes.h"
#include "unicode/uclean.h"
#include "putilimp.h"





struct UMutex;
struct UConditionVar;

U_NAMESPACE_BEGIN
struct UInitOnce;
U_NAMESPACE_END


#define U_MUTEX_STR(s) #s
#define U_MUTEX_XSTR(s) U_MUTEX_STR(s)







#if defined (U_USER_ATOMICS_H)
#include U_MUTEX_XSTR(U_USER_ATOMICS_H)

#elif U_HAVE_STD_ATOMICS



#include <atomic>

U_NAMESPACE_BEGIN

typedef std::atomic<int32_t> u_atomic_int32_t;
#define ATOMIC_INT32_T_INITIALIZER(val) ATOMIC_VAR_INIT(val)

inline int32_t umtx_loadAcquire(u_atomic_int32_t &var) {
    return var.load(std::memory_order_acquire);
}

inline void umtx_storeRelease(u_atomic_int32_t &var, int32_t val) {
    var.store(val, std::memory_order_release);
}

inline int32_t umtx_atomic_inc(u_atomic_int32_t *var) {
    return var->fetch_add(1) + 1;
}

inline int32_t umtx_atomic_dec(u_atomic_int32_t *var) {
    return var->fetch_sub(1) - 1;
}
U_NAMESPACE_END

#elif U_PLATFORM_HAS_WIN32_API









# define WIN32_LEAN_AND_MEAN
# define VC_EXTRALEAN
# define NOUSER
# define NOSERVICE
# define NOIME
# define NOMCX
# ifndef NOMINMAX
# define NOMINMAX
# endif
# include <windows.h>

U_NAMESPACE_BEGIN
typedef volatile LONG u_atomic_int32_t;
#define ATOMIC_INT32_T_INITIALIZER(val) val

inline int32_t umtx_loadAcquire(u_atomic_int32_t &var) {
    return InterlockedCompareExchange(&var, 0, 0);
}

inline void umtx_storeRelease(u_atomic_int32_t &var, int32_t val) {
    InterlockedExchange(&var, val);
}


inline int32_t umtx_atomic_inc(u_atomic_int32_t *var) {
    return InterlockedIncrement(var);
}

inline int32_t umtx_atomic_dec(u_atomic_int32_t *var) {
    return InterlockedDecrement(var);
}
U_NAMESPACE_END


#elif U_HAVE_GCC_ATOMICS




U_NAMESPACE_BEGIN
typedef int32_t u_atomic_int32_t;
#define ATOMIC_INT32_T_INITIALIZER(val) val

inline int32_t umtx_loadAcquire(u_atomic_int32_t &var) {
    int32_t val = var;
    __sync_synchronize();
    return val;
}

inline void umtx_storeRelease(u_atomic_int32_t &var, int32_t val) {
    __sync_synchronize();
    var = val;
}

inline int32_t umtx_atomic_inc(u_atomic_int32_t *p)  {
   return __sync_add_and_fetch(p, 1);
}

inline int32_t umtx_atomic_dec(u_atomic_int32_t *p)  {
   return __sync_sub_and_fetch(p, 1);
}
U_NAMESPACE_END

#else






#define U_NO_PLATFORM_ATOMICS

U_NAMESPACE_BEGIN
typedef int32_t u_atomic_int32_t;
#define ATOMIC_INT32_T_INITIALIZER(val) val

U_COMMON_API int32_t U_EXPORT2 
umtx_loadAcquire(u_atomic_int32_t &var);

U_COMMON_API void U_EXPORT2 
umtx_storeRelease(u_atomic_int32_t &var, int32_t val);

U_COMMON_API int32_t U_EXPORT2 
umtx_atomic_inc(u_atomic_int32_t *p);

U_COMMON_API int32_t U_EXPORT2 
umtx_atomic_dec(u_atomic_int32_t *p);

U_NAMESPACE_END

#endif  










U_NAMESPACE_BEGIN

struct UInitOnce {
    u_atomic_int32_t   fState;
    UErrorCode       fErrCode;
    void reset() {fState = 0;};
    UBool isReset() {return umtx_loadAcquire(fState) == 0;};


};

#define U_INITONCE_INITIALIZER {ATOMIC_INT32_T_INITIALIZER(0), U_ZERO_ERROR}


U_COMMON_API UBool U_EXPORT2 umtx_initImplPreInit(UInitOnce &);
U_COMMON_API void  U_EXPORT2 umtx_initImplPostInit(UInitOnce &);

template<class T> void umtx_initOnce(UInitOnce &uio, T *obj, void (T::*fp)()) {
    if (umtx_loadAcquire(uio.fState) == 2) {
        return;
    }
    if (umtx_initImplPreInit(uio)) {
        (obj->*fp)();
        umtx_initImplPostInit(uio);
    }
}




inline void umtx_initOnce(UInitOnce &uio, void (*fp)()) {
    if (umtx_loadAcquire(uio.fState) == 2) {
        return;
    }
    if (umtx_initImplPreInit(uio)) {
        (*fp)();
        umtx_initImplPostInit(uio);
    }
}



inline void umtx_initOnce(UInitOnce &uio, void (*fp)(UErrorCode &), UErrorCode &errCode) {
    if (U_FAILURE(errCode)) {
        return;
    }
    if (umtx_loadAcquire(uio.fState) != 2 && umtx_initImplPreInit(uio)) {
        
        (*fp)(errCode);
        uio.fErrCode = errCode;
        umtx_initImplPostInit(uio);
    } else {
        
        if (U_FAILURE(uio.fErrCode)) {
            errCode = uio.fErrCode;
        }
    }
}



template<class T> void umtx_initOnce(UInitOnce &uio, void (*fp)(T), T context) {
    if (umtx_loadAcquire(uio.fState) == 2) {
        return;
    }
    if (umtx_initImplPreInit(uio)) {
        (*fp)(context);
        umtx_initImplPostInit(uio);
    }
}



template<class T> void umtx_initOnce(UInitOnce &uio, void (*fp)(T, UErrorCode &), T context, UErrorCode &errCode) {
    if (U_FAILURE(errCode)) {
        return;
    }
    if (umtx_loadAcquire(uio.fState) != 2 && umtx_initImplPreInit(uio)) {
        
        (*fp)(context, errCode);
        uio.fErrCode = errCode;
        umtx_initImplPostInit(uio);
    } else {
        
        if (U_FAILURE(uio.fErrCode)) {
            errCode = uio.fErrCode;
        }
    }
}

U_NAMESPACE_END











#if defined(U_USER_MUTEX_H)

#include U_MUTEX_XSTR(U_USER_MUTEX_H)

#elif U_PLATFORM_HAS_WIN32_API
















# define WIN32_LEAN_AND_MEAN
# define VC_EXTRALEAN
# define NOUSER
# define NOSERVICE
# define NOIME
# define NOMCX
# ifndef NOMINMAX
# define NOMINMAX
# endif
# include <windows.h>


typedef struct UMutex {
    icu::UInitOnce    fInitOnce;
    CRITICAL_SECTION  fCS;
} UMutex;




#define U_MUTEX_INITIALIZER {U_INITONCE_INITIALIZER}

struct UConditionVar {
    HANDLE           fEntryGate;
    HANDLE           fExitGate;
    int32_t          fWaitCount;
};

#define U_CONDITION_INITIALIZER {NULL, NULL, 0}
    


#elif U_PLATFORM_IMPLEMENTS_POSIX





#include <pthread.h>

struct UMutex {
    pthread_mutex_t  fMutex;
};
typedef struct UMutex UMutex;
#define U_MUTEX_INITIALIZER  {PTHREAD_MUTEX_INITIALIZER}

struct UConditionVar {
    pthread_cond_t   fCondition;
};
#define U_CONDITION_INITIALIZER {PTHREAD_COND_INITIALIZER}

#else






#error Unknown Platform.

#endif
















U_INTERNAL void U_EXPORT2 umtx_lock(UMutex* mutex);





U_INTERNAL void U_EXPORT2 umtx_unlock (UMutex* mutex);










U_INTERNAL void U_EXPORT2 umtx_condWait(UConditionVar *cond, UMutex *mutex);









U_INTERNAL void U_EXPORT2 umtx_condBroadcast(UConditionVar *cond);





U_INTERNAL void U_EXPORT2 umtx_condSignal(UConditionVar *cond);

#endif 

