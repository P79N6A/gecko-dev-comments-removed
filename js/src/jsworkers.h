











#ifndef jsworkers_h___
#define jsworkers_h___

#include "jscntxt.h"
#include "jslock.h"

namespace js {

namespace ion {
  class IonBuilder;
}

#ifdef JS_THREADSAFE

struct WorkerThread;


struct WorkerThreadState
{
    
    WorkerThread *threads;
    size_t numThreads;

    enum CondVar {
        MAIN,
        WORKER
    };

    
    js::Vector<ion::IonBuilder*, 0, SystemAllocPolicy> ionWorklist;

    WorkerThreadState() { PodZero(this); }
    ~WorkerThreadState();

    bool init(JSRuntime *rt);

    void lock();
    void unlock();

#ifdef DEBUG
    bool isLocked();
#endif

    void wait(CondVar which, uint32_t timeoutMillis = 0);
    void notify(CondVar which);

  private:

    



    PRLock *workerLock;

#ifdef DEBUG
    PRThread *lockOwner;
#endif

    
    PRCondVar *mainWakeup;

    
    PRCondVar *helperWakeup;
};


struct WorkerThread
{
    JSRuntime *runtime;
    PRThread *thread;

    
    bool terminate;

    
    JSScript *ionScript;

    void destroy();

    static void ThreadMain(void *arg);
    void threadLoop();
};

#endif 







bool StartOffThreadIonCompile(JSContext *cx, ion::IonBuilder *builder);





void CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script);

class AutoLockWorkerThreadState
{
    JSRuntime *rt;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:

    AutoLockWorkerThreadState(JSRuntime *rt JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
#ifdef JS_THREADSAFE
        rt->workerThreadState->lock();
#endif
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoLockWorkerThreadState()
    {
#ifdef JS_THREADSAFE
        rt->workerThreadState->unlock();
#endif
    }
};

class AutoUnlockWorkerThreadState
{
    JSRuntime *rt;
    JS_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:

    AutoUnlockWorkerThreadState(JSRuntime *rt JS_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
#ifdef JS_THREADSAFE
        rt->workerThreadState->unlock();
#endif
        JS_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoUnlockWorkerThreadState()
    {
#ifdef JS_THREADSAFE
        rt->workerThreadState->lock();
#endif
    }
};

} 

#endif 
