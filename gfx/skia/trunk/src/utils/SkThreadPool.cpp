






#include "SkRunnable.h"
#include "SkThreadPool.h"
#include "SkThreadUtils.h"
#include "SkTypes.h"

#if defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_ANDROID)
#include <unistd.h>
#endif


static int num_cores() {
#if defined(SK_BUILD_FOR_WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_ANDROID)
    return sysconf(_SC_NPROCESSORS_ONLN);
#else
    return 1;
#endif
}

SkThreadPool::SkThreadPool(int count)
: fState(kRunning_State), fBusyThreads(0) {
    if (count < 0) count = num_cores();
    
    for (int i = 0; i < count; i++) {
        SkThread* thread = SkNEW_ARGS(SkThread, (&SkThreadPool::Loop, this));
        *fThreads.append() = thread;
        thread->start();
    }
}

SkThreadPool::~SkThreadPool() {
    if (kRunning_State == fState) {
        this->wait();
    }
}

void SkThreadPool::wait() {
    fReady.lock();
    fState = kWaiting_State;
    fReady.broadcast();
    fReady.unlock();

    
    for (int i = 0; i < fThreads.count(); i++) {
        fThreads[i]->join();
        SkDELETE(fThreads[i]);
    }
    SkASSERT(fQueue.isEmpty());
}

 void SkThreadPool::Loop(void* arg) {
    
    SkThreadPool* pool = static_cast<SkThreadPool*>(arg);

    while (true) {
        
        pool->fReady.lock();
        while(pool->fQueue.isEmpty()) {
            
            
            if (kWaiting_State == pool->fState && pool->fBusyThreads == 0) {
                pool->fState = kHalting_State;
                pool->fReady.broadcast();
            }
            
            if (kHalting_State == pool->fState) {
                pool->fReady.unlock();
                return;
            }
            
            pool->fReady.wait();
        }
        

        
        LinkedRunnable* r = pool->fQueue.tail();

        pool->fQueue.remove(r);

        
        
        
        pool->fBusyThreads++;
        pool->fReady.unlock();

        
        r->fRunnable->run();
        SkDELETE(r);

        
        pool->fReady.lock();
        pool->fBusyThreads--;
        pool->fReady.unlock();
    }

    SkASSERT(false); 
}

void SkThreadPool::add(SkRunnable* r) {
    if (NULL == r) {
        return;
    }

    
    if (fThreads.isEmpty()) {
        return r->run();
    }

    
    fReady.lock();
    SkASSERT(fState != kHalting_State);  
    LinkedRunnable* linkedRunnable = SkNEW(LinkedRunnable);
    linkedRunnable->fRunnable = r;
    fQueue.addToHead(linkedRunnable);
    fReady.signal();
    fReady.unlock();
}
