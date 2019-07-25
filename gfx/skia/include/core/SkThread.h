








#ifndef SkThread_DEFINED
#define SkThread_DEFINED

#include "SkTypes.h"
#include "SkThread_platform.h"

















class SkAutoMutexAcquire : SkNoncopyable {
public:
    explicit SkAutoMutexAcquire(SkMutex& mutex) : fMutex(&mutex)
    {
        SkASSERT(fMutex != NULL);
        mutex.acquire();
    }
    

    ~SkAutoMutexAcquire()
    {
        if (fMutex)
            fMutex->release();
    }
    

    void release()
    {
        if (fMutex)
        {
            fMutex->release();
            fMutex = NULL;
        }
    }
        
private:
    SkMutex* fMutex;
};

#endif
