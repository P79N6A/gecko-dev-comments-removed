






#ifndef SkMutex_pthread_DEFINED
#define SkMutex_pthread_DEFINED



#ifdef SK_DEBUG_PTHREAD_MUTEX
#include "SkTypes.h"
#define SkDEBUGCODE_PTHREAD_MUTEX(code) code
#else
#define SkDEBUGCODE_PTHREAD_MUTEX(code)
#ifndef SkDebugf
    void SkDebugf(const char format[], ...);
#endif
#endif

#include <errno.h>
#include <pthread.h>





struct SkBaseMutex {
    void acquire() { pthread_mutex_lock(&fMutex); }
    void release() { pthread_mutex_unlock(&fMutex); }
    pthread_mutex_t fMutex;
};



class SkMutex : public SkBaseMutex {
public:
    SkMutex() {
        SkDEBUGCODE_PTHREAD_MUTEX(int status = )pthread_mutex_init(&fMutex, NULL);
        SkDEBUGCODE_PTHREAD_MUTEX(
            if (status != 0) {
                print_pthread_error(status);
                SkASSERT(0 == status);
            }
        )
    }

    ~SkMutex() {
        SkDEBUGCODE_PTHREAD_MUTEX(int status = )pthread_mutex_destroy(&fMutex);
        SkDEBUGCODE_PTHREAD_MUTEX(
            if (status != 0) {
                print_pthread_error(status);
                SkASSERT(0 == status);
            }
        )
    }

private:
    SkMutex(const SkMutex&);
    SkMutex& operator=(const SkMutex&);

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
};


#define SK_DECLARE_STATIC_MUTEX(name) static SkBaseMutex name = { PTHREAD_MUTEX_INITIALIZER }


#define SK_DECLARE_GLOBAL_MUTEX(name) SkBaseMutex name = { PTHREAD_MUTEX_INITIALIZER }

#endif
