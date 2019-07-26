






#include "SkThread.h"
#include "SkTLS.h"

#include <pthread.h>
#include <errno.h>

#ifndef SK_BUILD_FOR_ANDROID










#if (__GNUC__ == 4 && __GNUC_MINOR__ >= 1) || __GNUC__ > 4
    #if (defined(__x86_64) || defined(__i386__))
        #define GCC_INTRINSIC
    #endif
#endif

#if defined(GCC_INTRINSIC)

int32_t sk_atomic_inc(int32_t* addr)
{
    return __sync_fetch_and_add(addr, 1);
}

int32_t sk_atomic_add(int32_t* addr, int32_t inc)
{
    return __sync_fetch_and_add(addr, inc);
}

int32_t sk_atomic_dec(int32_t* addr)
{
    return __sync_fetch_and_add(addr, -1);
}
void sk_membar_aquire__after_atomic_dec() { }

int32_t sk_atomic_conditional_inc(int32_t* addr)
{
    int32_t value = *addr;

    while (true) {
        if (value == 0) {
            return 0;
        }

        int32_t before = __sync_val_compare_and_swap(addr, value, value + 1);

        if (before == value) {
            return value;
        } else {
            value = before;
        }
    }
}
void sk_membar_aquire__after_atomic_conditional_inc() { }

#else

SkMutex gAtomicMutex;

int32_t sk_atomic_inc(int32_t* addr)
{
    SkAutoMutexAcquire ac(gAtomicMutex);

    int32_t value = *addr;
    *addr = value + 1;
    return value;
}

int32_t sk_atomic_add(int32_t* addr, int32_t inc)
{
    SkAutoMutexAcquire ac(gAtomicMutex);

    int32_t value = *addr;
    *addr = value + inc;
    return value;
}

int32_t sk_atomic_dec(int32_t* addr)
{
    SkAutoMutexAcquire ac(gAtomicMutex);

    int32_t value = *addr;
    *addr = value - 1;
    return value;
}
void sk_membar_aquire__after_atomic_dec() { }

int32_t sk_atomic_conditional_inc(int32_t* addr)
{
    SkAutoMutexAcquire ac(gAtomicMutex);

    int32_t value = *addr;
    if (value != 0) ++*addr;
    return value;
}
void sk_membar_aquire__after_atomic_conditional_inc() { }

#endif

#endif 



static void print_pthread_error(int status) {
    switch (status) {
    case 0: 
        break;
    case EINVAL:
        SkDebugf("pthread error [%d] EINVAL\n", status);
        break;
    case EBUSY:
        SkDebugf("pthread error [%d] EBUSY\n", status);
        break;
    default:
        SkDebugf("pthread error [%d] unknown\n", status);
        break;
    }
}

#ifdef SK_USE_POSIX_THREADS

SkMutex::SkMutex() {
    int status;

    status = pthread_mutex_init(&fMutex, NULL);
    if (status != 0) {
        print_pthread_error(status);
        SkASSERT(0 == status);
    }
}

SkMutex::~SkMutex() {
    int status = pthread_mutex_destroy(&fMutex);

    
    if (status != 0) {
        print_pthread_error(status);
        SkASSERT(0 == status);
    }
}

#else 

SkMutex::SkMutex() {
    if (sizeof(pthread_mutex_t) > sizeof(fStorage)) {
        SkDEBUGF(("pthread mutex size = %d\n", sizeof(pthread_mutex_t)));
        SkDEBUGFAIL("mutex storage is too small");
    }

    int status;
    pthread_mutexattr_t attr;

    status = pthread_mutexattr_init(&attr);
    print_pthread_error(status);
    SkASSERT(0 == status);

    status = pthread_mutex_init((pthread_mutex_t*)fStorage, &attr);
    print_pthread_error(status);
    SkASSERT(0 == status);
}

SkMutex::~SkMutex() {
    int status = pthread_mutex_destroy((pthread_mutex_t*)fStorage);
#if 0
    
    if (!fIsGlobal) {
        print_pthread_error(status);
        SkASSERT(0 == status);
    }
#endif
}

void SkMutex::acquire() {
    int status = pthread_mutex_lock((pthread_mutex_t*)fStorage);
    print_pthread_error(status);
    SkASSERT(0 == status);
}

void SkMutex::release() {
    int status = pthread_mutex_unlock((pthread_mutex_t*)fStorage);
    print_pthread_error(status);
    SkASSERT(0 == status);
}

#endif 



static pthread_key_t gSkTLSKey;
static pthread_once_t gSkTLSKey_Once = PTHREAD_ONCE_INIT;

static void sk_tls_make_key() {
    (void)pthread_key_create(&gSkTLSKey, SkTLS::Destructor);
}

void* SkTLS::PlatformGetSpecific(bool forceCreateTheSlot) {
    
    
    

    (void)pthread_once(&gSkTLSKey_Once, sk_tls_make_key);
    return pthread_getspecific(gSkTLSKey);
}

void SkTLS::PlatformSetSpecific(void* ptr) {
    (void)pthread_setspecific(gSkTLSKey, ptr);
}

