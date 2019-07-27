






#ifndef SkMutex_pthread_DEFINED
#define SkMutex_pthread_DEFINED



#include <errno.h>
#include <pthread.h>



SkDEBUGCODE(static const pthread_t kNoOwner = 0;)





struct SkBaseMutex {
    void acquire() {
        SkASSERT(0 == pthread_equal(fOwner, pthread_self()));  
        pthread_mutex_lock(&fMutex);
        SkDEBUGCODE(fOwner = pthread_self();)
    }
    void release() {
        this->assertHeld();
        SkDEBUGCODE(fOwner = kNoOwner;)
        pthread_mutex_unlock(&fMutex);
    }
    void assertHeld() {
        SkASSERT(0 != pthread_equal(fOwner, pthread_self()));
    }

    pthread_mutex_t fMutex;
    SkDEBUGCODE(pthread_t fOwner;)
};



class SkMutex : public SkBaseMutex {
public:
    SkMutex() {
        SkDEBUGCODE(int status = )pthread_mutex_init(&fMutex, NULL);
        SkDEBUGCODE(
            if (status != 0) {
                print_pthread_error(status);
                SkASSERT(0 == status);
            }
            fOwner = kNoOwner;
        )
    }

    ~SkMutex() {
        SkDEBUGCODE(int status = )pthread_mutex_destroy(&fMutex);
        SkDEBUGCODE(
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

#define SK_BASE_MUTEX_INIT { PTHREAD_MUTEX_INITIALIZER, SkDEBUGCODE(0) }









#define SK_DECLARE_STATIC_MUTEX(name) namespace {} static SkBaseMutex name = SK_BASE_MUTEX_INIT

#endif
