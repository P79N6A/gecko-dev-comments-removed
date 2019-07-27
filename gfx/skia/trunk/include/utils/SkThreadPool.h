






#ifndef SkThreadPool_DEFINED
#define SkThreadPool_DEFINED

#include "SkCondVar.h"
#include "SkRunnable.h"
#include "SkTDArray.h"
#include "SkTInternalLList.h"
#include "SkThreadUtils.h"
#include "SkTypes.h"

#if defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_ANDROID)
#    include <unistd.h>
#endif


static inline int num_cores() {
#if defined(SK_BUILD_FOR_WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
#elif defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_ANDROID)
    return (int) sysconf(_SC_NPROCESSORS_ONLN);
#else
    return 1;
#endif
}

template <typename T>
class SkTThreadPool {
public:
    


    static const int kThreadPerCore = -1;
    explicit SkTThreadPool(int count);
    ~SkTThreadPool();

    




    void add(SkTRunnable<T>*);

    


    void addNext(SkTRunnable<T>*);

    


    void wait();

 private:
    struct LinkedRunnable {
        SkTRunnable<T>* fRunnable;  
        SK_DECLARE_INTERNAL_LLIST_INTERFACE(LinkedRunnable);
    };

    enum State {
        kRunning_State,  
        kWaiting_State,  
        kHalting_State,  
    };

    void addSomewhere(SkTRunnable<T>* r,
                      void (SkTInternalLList<LinkedRunnable>::*)(LinkedRunnable*));

    SkTInternalLList<LinkedRunnable> fQueue;
    SkCondVar                        fReady;
    SkTDArray<SkThread*>             fThreads;
    State                            fState;
    int                              fBusyThreads;

    static void Loop(void*);  
};

template <typename T>
SkTThreadPool<T>::SkTThreadPool(int count) : fState(kRunning_State), fBusyThreads(0) {
    if (count < 0) {
        count = num_cores();
    }
    
    for (int i = 0; i < count; i++) {
        SkThread* thread = SkNEW_ARGS(SkThread, (&SkTThreadPool::Loop, this));
        *fThreads.append() = thread;
        thread->start();
    }
}

template <typename T>
SkTThreadPool<T>::~SkTThreadPool() {
    if (kRunning_State == fState) {
        this->wait();
    }
}

namespace SkThreadPoolPrivate {

template <typename T>
struct ThreadLocal {
    void run(SkTRunnable<T>* r) { r->run(data); }
    T data;
};

template <>
struct ThreadLocal<void> {
    void run(SkTRunnable<void>* r) { r->run(); }
};

}  

template <typename T>
void SkTThreadPool<T>::addSomewhere(SkTRunnable<T>* r,
                                    void (SkTInternalLList<LinkedRunnable>::* f)(LinkedRunnable*)) {
    if (r == NULL) {
        return;
    }

    if (fThreads.isEmpty()) {
        SkThreadPoolPrivate::ThreadLocal<T> threadLocal;
        threadLocal.run(r);
        return;
    }

    LinkedRunnable* linkedRunnable = SkNEW(LinkedRunnable);
    linkedRunnable->fRunnable = r;
    fReady.lock();
    SkASSERT(fState != kHalting_State);  
    (fQueue.*f)(linkedRunnable);
    fReady.signal();
    fReady.unlock();
}

template <typename T>
void SkTThreadPool<T>::add(SkTRunnable<T>* r) {
    this->addSomewhere(r, &SkTInternalLList<LinkedRunnable>::addToTail);
}

template <typename T>
void SkTThreadPool<T>::addNext(SkTRunnable<T>* r) {
    this->addSomewhere(r, &SkTInternalLList<LinkedRunnable>::addToHead);
}


template <typename T>
void SkTThreadPool<T>::wait() {
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

template <typename T>
 void SkTThreadPool<T>::Loop(void* arg) {
    
    SkTThreadPool<T>* pool = static_cast<SkTThreadPool<T>*>(arg);
    SkThreadPoolPrivate::ThreadLocal<T> threadLocal;

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
        

        
        LinkedRunnable* r = pool->fQueue.head();

        pool->fQueue.remove(r);

        
        
        
        pool->fBusyThreads++;
        pool->fReady.unlock();

        
        threadLocal.run(r->fRunnable);
        SkDELETE(r);

        
        pool->fReady.lock();
        pool->fBusyThreads--;
        pool->fReady.unlock();
    }

    SkASSERT(false); 
}

typedef SkTThreadPool<void> SkThreadPool;

#endif
