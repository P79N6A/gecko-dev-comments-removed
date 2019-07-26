











#ifndef jsworkers_h___
#define jsworkers_h___

#include "mozilla/GuardObjects.h"

#include "jscntxt.h"
#include "jslock.h"

#include "ion/Ion.h"

namespace js {

namespace ion {
  class IonBuilder;
}

#if defined(JS_THREADSAFE) && defined(JS_ION)
# define JS_PARALLEL_COMPILATION

struct WorkerThread;
struct AsmJSParallelTask;


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

    
    js::Vector<AsmJSParallelTask*, 0, SystemAllocPolicy> asmJSWorklist;

    




    js::Vector<AsmJSParallelTask*, 0, SystemAllocPolicy> asmJSFinishedList;

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

    bool canStartAsmJSCompile();
    bool canStartIonCompile();

    uint32_t harvestFailedAsmJSJobs() {
        JS_ASSERT(isLocked());
        uint32_t n = numAsmJSFailedJobs;
        numAsmJSFailedJobs = 0;
        return n;
    }
    void noteAsmJSFailure(int32_t func) {
        
        JS_ASSERT(isLocked());
        if (asmJSFailedFunctionIndex < 0)
            asmJSFailedFunctionIndex = func;
        numAsmJSFailedJobs++;
    }
    bool asmJSWorkerFailed() const {
        return bool(numAsmJSFailedJobs);
    }
    void resetAsmJSFailureState() {
        numAsmJSFailedJobs = 0;
        asmJSFailedFunctionIndex = -1;
    }
    int32_t maybeGetAsmJSFailedFunctionIndex() const {
        return asmJSFailedFunctionIndex;
    }

  private:

    



    PRLock *workerLock;

# ifdef DEBUG
    PRThread *lockOwner;
# endif

    
    PRCondVar *mainWakeup;

    
    PRCondVar *helperWakeup;

    



    uint32_t numAsmJSFailedJobs;

    



    int32_t asmJSFailedFunctionIndex;
};


struct WorkerThread
{
    JSRuntime *runtime;

    mozilla::Maybe<PerThreadData> threadData;
    PRThread *thread;

    
    bool terminate;

    
    ion::IonBuilder *ionBuilder;

    
    AsmJSParallelTask *asmData;

    void destroy();

    void handleAsmJSWorkload(WorkerThreadState &state);
    void handleIonWorkload(WorkerThreadState &state);

    static void ThreadMain(void *arg);
    void threadLoop();
};

#endif 

inline bool
OffThreadCompilationEnabled(JSContext *cx)
{
#ifdef JS_PARALLEL_COMPILATION
    return ion::js_IonOptions.parallelCompilation
        && cx->runtime->useHelperThreads()
        && cx->runtime->helperThreadCount() != 0;
#else
    return false;
#endif
}




bool
EnsureParallelCompilationInitialized(JSRuntime *rt);


bool
StartOffThreadAsmJSCompile(JSContext *cx, AsmJSParallelTask *asmData);





bool
StartOffThreadIonCompile(JSContext *cx, ion::IonBuilder *builder);





void
CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script);

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
