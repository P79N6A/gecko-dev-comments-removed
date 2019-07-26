






#include "SkThreadPool.h"
#include "SkRunnable.h"
#include "SkThreadUtils.h"

SkThreadPool::SkThreadPool(const int count)
: fDone(false) {
    
    for (int i = 0; i < count; i++) {
        SkThread* thread = SkNEW_ARGS(SkThread, (&SkThreadPool::Loop, this));
        *fThreads.append() = thread;
        thread->start();
    }
}

SkThreadPool::~SkThreadPool() {
    fDone = true;
    fReady.lock();
    fReady.broadcast();
    fReady.unlock();

    
    for (int i = 0; i < fThreads.count(); i++) {
        fThreads[i]->join();
        SkDELETE(fThreads[i]);
    }
}

 void SkThreadPool::Loop(void* arg) {
    
    SkThreadPool* pool = static_cast<SkThreadPool*>(arg);

    while (true) {
        
        pool->fReady.lock();
        while(pool->fQueue.isEmpty()) {
            
            if (pool->fDone) {
                pool->fReady.unlock();
                return;
            }
            
            pool->fReady.wait();
        }
        

        
        LinkedRunnable* r = pool->fQueue.tail();

        pool->fQueue.remove(r);

        
        
        
        pool->fReady.unlock();

        
        r->fRunnable->run();
        SkDELETE(r);
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
    LinkedRunnable* linkedRunnable = SkNEW(LinkedRunnable);
    linkedRunnable->fRunnable = r;
    fQueue.addToHead(linkedRunnable);
    fReady.signal();
    fReady.unlock();
}
