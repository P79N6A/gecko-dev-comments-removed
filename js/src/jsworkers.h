











#ifndef jsworkers_h___
#define jsworkers_h___

#include "mozilla/GuardObjects.h"

#include "jscntxt.h"
#include "jslock.h"

namespace js {

namespace ion {
  class IonBuilder;
}

#if defined(JS_THREADSAFE) && defined(JS_ION)
# define JS_PARALLEL_COMPILATION

struct WorkerThread;


class WorkerThreadState
{
  public:
    
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

# ifdef DEBUG
    bool isLocked();
# endif

    void wait(CondVar which, uint32_t timeoutMillis = 0);
    void notify(CondVar which);
    void notifyAll(CondVar which);

    bool canStartIonCompile();

  private:

    



    PRLock *workerLock;

# ifdef DEBUG
    PRThread *lockOwner;
# endif

    
    PRCondVar *mainWakeup;

    
    PRCondVar *helperWakeup;
};


struct WorkerThread
{
    JSRuntime *runtime;

    mozilla::Maybe<PerThreadData> threadData;
    PRThread *thread;

    
    bool terminate;

    
    ion::IonBuilder *ionBuilder;

    void destroy();

    static void ThreadMain(void *arg);
    void threadLoop();
};

#endif 







bool
StartOffThreadIonCompile(JSContext *cx, ion::IonBuilder *builder);





void
CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script);


bool
OffThreadCompilationAvailable(JSContext *cx);

class AutoLockWorkerThreadState
{
    JSRuntime *rt;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:

    AutoLockWorkerThreadState(JSRuntime *rt
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef JS_PARALLEL_COMPILATION
        JS_ASSERT(rt->workerThreadState);
        rt->workerThreadState->lock();
#else
        (void)this->rt;
#endif
    }

    ~AutoLockWorkerThreadState()
    {
#ifdef JS_PARALLEL_COMPILATION
        rt->workerThreadState->unlock();
#endif
    }
};

class AutoUnlockWorkerThreadState
{
    JSRuntime *rt;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:

    AutoUnlockWorkerThreadState(JSRuntime *rt
                                MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : rt(rt)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef JS_PARALLEL_COMPILATION
        JS_ASSERT(rt->workerThreadState);
        rt->workerThreadState->unlock();
#else
        (void)this->rt;
#endif
    }

    ~AutoUnlockWorkerThreadState()
    {
#ifdef JS_PARALLEL_COMPILATION
        rt->workerThreadState->lock();
#endif
    }
};

} 

#endif 
