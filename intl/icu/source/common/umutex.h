
















#ifndef UMUTEX_H
#define UMUTEX_H

#include "unicode/utypes.h"
#include "unicode/uclean.h"
#include "putilimp.h"


#if defined(_MSC_VER) && _MSC_VER >= 1500
# include <intrin.h>
#endif


#if U_PLATFORM_HAS_WIN32_API
#if 0  






# define WIN32_LEAN_AND_MEAN
# define VC_EXTRALEAN
# define NOUSER
# define NOSERVICE
# define NOIME
# define NOMCX
# include <windows.h>
#endif  
#define U_WINDOWS_CRIT_SEC_SIZE 64
#endif  

#if U_PLATFORM_IS_DARWIN_BASED
#if defined(__STRICT_ANSI__)
#define UPRV_REMAP_INLINE
#define inline
#endif
#include <libkern/OSAtomic.h>
#define USE_MAC_OS_ATOMIC_INCREMENT 1
#if defined(UPRV_REMAP_INLINE)
#undef inline
#undef UPRV_REMAP_INLINE
#endif
#endif






#ifndef ANNOTATE_HAPPENS_BEFORE
# define ANNOTATE_HAPPENS_BEFORE(obj)
# define ANNOTATE_HAPPENS_AFTER(obj)
# define ANNOTATE_UNPROTECTED_READ(x) (x)
#endif

#ifndef UMTX_FULL_BARRIER
# if U_HAVE_GCC_ATOMICS
#  define UMTX_FULL_BARRIER __sync_synchronize();
# elif defined(_MSC_VER) && _MSC_VER >= 1500
    
#  define UMTX_FULL_BARRIER _ReadWriteBarrier();
# elif U_PLATFORM_IS_DARWIN_BASED
#  define UMTX_FULL_BARRIER OSMemoryBarrier();
# else
#  define UMTX_FULL_BARRIER \
    { \
        umtx_lock(NULL); \
        umtx_unlock(NULL); \
    }
# endif
#endif

#ifndef UMTX_ACQUIRE_BARRIER
# define UMTX_ACQUIRE_BARRIER UMTX_FULL_BARRIER
#endif

#ifndef UMTX_RELEASE_BARRIER
# define UMTX_RELEASE_BARRIER UMTX_FULL_BARRIER
#endif















#define UMTX_CHECK(pMutex, expression, result) \
    { \
        (result)=(expression); \
        UMTX_ACQUIRE_BARRIER; \
    }


































#if U_PLATFORM_HAS_WIN32_API





typedef struct U_INIT_ONCE {
    long               fState;
    void              *fContext;
} U_INIT_ONCE;
#define U_INIT_ONCE_STATIC_INIT {0, NULL}

typedef struct UMutex {
    U_INIT_ONCE       fInitOnce;
    UMTX              fUserMutex;
    UBool             fInitialized;  
      


    char              fCS[U_WINDOWS_CRIT_SEC_SIZE];
} UMutex;




#define U_MUTEX_INITIALIZER {U_INIT_ONCE_STATIC_INIT, NULL, FALSE}

#elif U_PLATFORM_IMPLEMENTS_POSIX
#include <pthread.h>

struct UMutex {
    pthread_mutex_t  fMutex;
    UMTX             fUserMutex;
    UBool            fInitialized;
};
#define U_MUTEX_INITIALIZER  {PTHREAD_MUTEX_INITIALIZER, NULL, FALSE}

#else

struct UMutex {
    void *fMutex;
};
#define U_MUTEX_INITIALIZER {NULL}
#error Unknown Platform.

#endif

#if (U_PLATFORM != U_PF_CYGWIN && U_PLATFORM != U_PF_MINGW) || defined(CYGWINMSVC)
typedef struct UMutex UMutex;
#endif
    





U_CAPI void U_EXPORT2 umtx_lock(UMutex* mutex); 





U_CAPI void U_EXPORT2 umtx_unlock (UMutex* mutex);










U_CAPI int32_t U_EXPORT2 umtx_atomic_inc(int32_t *);
U_CAPI int32_t U_EXPORT2 umtx_atomic_dec(int32_t *);

#endif 

