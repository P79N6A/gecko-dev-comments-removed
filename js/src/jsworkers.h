











#ifndef jsworkers_h
#define jsworkers_h

#include "mozilla/GuardObjects.h"
#include "mozilla/PodOperations.h"

#include "jscntxt.h"
#include "jslock.h"

#include "ion/Ion.h"

namespace js {

struct AsmJSParallelTask;

namespace ion {
  class IonBuilder;
}

#if defined(JS_THREADSAFE) && defined(JS_ION)
# define JS_WORKER_THREADS

struct WorkerThread;
struct AsmJSParallelTask;
struct ParseTask;


class WorkerThreadState
{
  public:
    
    WorkerThread *threads;
    size_t numThreads;

    



    volatile size_t shouldPause;

    
    uint32_t numPaused;

    enum CondVar {
        MAIN,
        WORKER
    };

    
    Vector<ion::IonBuilder*, 0, SystemAllocPolicy> ionWorklist;

    
    Vector<AsmJSParallelTask*, 0, SystemAllocPolicy> asmJSWorklist;

    




    Vector<AsmJSParallelTask*, 0, SystemAllocPolicy> asmJSFinishedList;

    
    Vector<ParseTask*, 0, SystemAllocPolicy> parseWorklist, parseFinishedList;

    WorkerThreadState() { mozilla::PodZero(this); }
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
    void noteAsmJSFailure(void *func) {
        
        JS_ASSERT(isLocked());
        if (!asmJSFailedFunction)
            asmJSFailedFunction = func;
        numAsmJSFailedJobs++;
    }
    bool asmJSWorkerFailed() const {
        return bool(numAsmJSFailedJobs);
    }
    void resetAsmJSFailureState() {
        numAsmJSFailedJobs = 0;
        asmJSFailedFunction = NULL;
    }
    void *maybeAsmJSFailedFunction() const {
        return asmJSFailedFunction;
    }

  private:

    



    PRLock *workerLock;

# ifdef DEBUG
    PRThread *lockOwner;
# endif

    
    PRCondVar *mainWakeup;

    
    PRCondVar *helperWakeup;

    



    uint32_t numAsmJSFailedJobs;

    



    void *asmJSFailedFunction;
};


struct WorkerThread
{
    JSRuntime *runtime;

    mozilla::Maybe<PerThreadData> threadData;
    PRThread *thread;

    
    bool terminate;

    
    ion::IonBuilder *ionBuilder;

    
    AsmJSParallelTask *asmData;

    
    ParseTask *parseTask;

    bool idle() const {
        return !ionBuilder && !asmData && !parseTask;
    }

    void pause();
    void destroy();

    void handleAsmJSWorkload(WorkerThreadState &state);
    void handleIonWorkload(WorkerThreadState &state);
    void handleParseWorkload(WorkerThreadState &state);

    static void ThreadMain(void *arg);
    void threadLoop();
};

#endif 

inline bool
OffThreadCompilationEnabled(JSContext *cx)
{
#ifdef JS_WORKER_THREADSj
    return ion::js_IonOptions.parallelCompilation
        && cx->runtime()->useHelperThreads()
        && cx->runtime()->helperThreadCount() != 0;
#else
    return false;
#endif
}




bool
EnsureWorkerThreadsInitialized(JSRuntime *rt);


bool
StartOffThreadAsmJSCompile(JSContext *cx, AsmJSParallelTask *asmData);





bool
StartOffThreadIonCompile(JSContext *cx, ion::IonBuilder *builder);





void
CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script);





bool
StartOffThreadParseScript(JSContext *cx, const CompileOptions &options,
                          const jschar *chars, size_t length);


void
WaitForOffThreadParsingToFinish(JSRuntime *rt);

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
#ifdef JS_WORKER_THREADS
        JS_ASSERT(rt->workerThreadState);
        rt->workerThreadState->lock();
#else
        (void)this->rt;
#endif
    }

    ~AutoLockWorkerThreadState()
    {
#ifdef JS_WORKER_THREADS
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
#ifdef JS_WORKER_THREADS
        JS_ASSERT(rt->workerThreadState);
        rt->workerThreadState->unlock();
#else
        (void)this->rt;
#endif
    }

    ~AutoUnlockWorkerThreadState()
    {
#ifdef JS_WORKER_THREADS
        rt->workerThreadState->lock();
#endif
    }
};


class AutoPauseWorkersForGC
{
    JSRuntime *runtime;
    bool needsUnpause;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    AutoPauseWorkersForGC(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
    ~AutoPauseWorkersForGC();
};


void
PauseOffThreadParsing();


void
ResumeOffThreadParsing();

struct ParseTask
{
    JSRuntime *runtime;
    ExclusiveContext *cx;
    CompileOptions options;
    const jschar *chars;
    size_t length;
    LifoAlloc alloc;

    JSScript *script;

    ParseTask(JSRuntime *rt, ExclusiveContext *cx, const CompileOptions &options,
              const jschar *chars, size_t length);
    ~ParseTask();
};

} 

#endif 
