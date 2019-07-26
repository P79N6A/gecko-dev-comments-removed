








#ifndef SkThread_DEFINED
#define SkThread_DEFINED

#include "SkTypes.h"
#include "SkThread_platform.h"



















class SkAutoMutexAcquire : SkNoncopyable {
public:
    explicit SkAutoMutexAcquire(SkBaseMutex& mutex) : fMutex(&mutex) {
        SkASSERT(fMutex != NULL);
        mutex.acquire();
    }

    SkAutoMutexAcquire(SkBaseMutex* mutex) : fMutex(mutex) {
        if (mutex) {
            mutex->acquire();
        }
    }

    

    ~SkAutoMutexAcquire() {
        if (fMutex) {
            fMutex->release();
        }
    }

    

    void release() {
        if (fMutex) {
            fMutex->release();
            fMutex = NULL;
        }
    }

private:
    SkBaseMutex* fMutex;
};

#endif
