











#ifndef jsworkers_h
#define jsworkers_h

#include "mozilla/GuardObjects.h"
#include "mozilla/PodOperations.h"

#include "jscntxt.h"
#include "jslock.h"

#include "frontend/TokenStream.h"
#include "jit/Ion.h"

namespace js {

struct WorkerThread;
struct AsmJSParallelTask;
struct ParseTask;
namespace jit {
  class IonBuilder;
}

#ifdef JS_WORKER_THREADS


class WorkerThreadState
{
  public:
    
    WorkerThread *threads;
    size_t numThreads;

    enum CondVar {
        
        CONSUMER,

        
        PRODUCER
    };

    
    Vector<jit::IonBuilder*, 0, SystemAllocPolicy> ionWorklist;

    
    Vector<AsmJSParallelTask*, 0, SystemAllocPolicy> asmJSWorklist;

    




    Vector<AsmJSParallelTask*, 0, SystemAllocPolicy> asmJSFinishedList;

    



    mozilla::Atomic<uint32_t> asmJSCompilationInProgress;

    
    Vector<ParseTask*, 0, SystemAllocPolicy> parseWorklist, parseFinishedList;

    
    Vector<ParseTask*, 0, SystemAllocPolicy> parseWaitingOnGC;

    
    Vector<SourceCompressionTask *, 0, SystemAllocPolicy> compressionWorklist;

    WorkerThreadState(JSRuntime *rt) {
        mozilla::PodZero(this);
        runtime = rt;
    }
    ~WorkerThreadState();

    bool init();
    void cleanup();

    void lock();
    void unlock();

# ifdef DEBUG
    bool isLocked();
# endif

    void wait(CondVar which, uint32_t timeoutMillis = 0);
    void notifyAll(CondVar which);

    bool canStartAsmJSCompile();
    bool canStartIonCompile();
    bool canStartParseTask();
    bool canStartCompressionTask();

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
        asmJSFailedFunction = nullptr;
    }
    void *maybeAsmJSFailedFunction() const {
        return asmJSFailedFunction;
    }

    JSScript *finishParseTask(JSContext *maybecx, JSRuntime *rt, void *token);
    bool compressionInProgress(SourceCompressionTask *task);
    SourceCompressionTask *compressionTaskForSource(ScriptSource *ss);

  private:

    JSRuntime *runtime;

    



    PRLock *workerLock;

# ifdef DEBUG
    PRThread *lockOwner;
# endif

    
    PRCondVar *consumerWakeup;
    PRCondVar *producerWakeup;

    



    uint32_t numAsmJSFailedJobs;

    



    void *asmJSFailedFunction;
};


struct WorkerThread
{
    JSRuntime *runtime;

    mozilla::Maybe<PerThreadData> threadData;
    PRThread *thread;

    
    bool terminate;

    
    jit::IonBuilder *ionBuilder;

    
    AsmJSParallelTask *asmData;

    
    ParseTask *parseTask;

    
    SourceCompressionTask *compressionTask;

    bool idle() const {
        return !ionBuilder && !asmData && !parseTask && !compressionTask;
    }

    void destroy();

    void handleAsmJSWorkload(WorkerThreadState &state);
    void handleIonWorkload(WorkerThreadState &state);
    void handleParseWorkload(WorkerThreadState &state);
    void handleCompressionWorkload(WorkerThreadState &state);

    static void ThreadMain(void *arg);
    void threadLoop();
};

#endif 

inline bool
OffThreadIonCompilationEnabled(JSRuntime *rt)
{
#ifdef JS_WORKER_THREADS
    return rt->useHelperThreads()
        && rt->helperThreadCount() != 0
        && rt->useHelperThreadsForIonCompilation();
#else
    return false;
#endif
}




bool
EnsureWorkerThreadsInitialized(ExclusiveContext *cx);


bool
StartOffThreadAsmJSCompile(ExclusiveContext *cx, AsmJSParallelTask *asmData);





bool
StartOffThreadIonCompile(JSContext *cx, jit::IonBuilder *builder);





void
CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script);





bool
StartOffThreadParseScript(JSContext *cx, const ReadOnlyCompileOptions &options,
                          const jschar *chars, size_t length, HandleObject scopeChain,
                          JS::OffThreadCompileCallback callback, void *callbackData);





void
EnqueuePendingParseTasksAfterGC(JSRuntime *rt);


void
WaitForOffThreadParsingToFinish(JSRuntime *rt);


bool
StartOffThreadCompression(ExclusiveContext *cx, SourceCompressionTask *task);

class AutoLockWorkerThreadState
{
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

#ifdef JS_WORKER_THREADS
    WorkerThreadState &state;

  public:
    AutoLockWorkerThreadState(WorkerThreadState &state
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : state(state)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        state.lock();
    }

    ~AutoLockWorkerThreadState() {
        state.unlock();
    }
#else
  public:
    AutoLockWorkerThreadState(WorkerThreadState &state
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
#endif
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

#ifdef JS_ION
struct AsmJSParallelTask
{
    LifoAlloc lifo;         
    void *func;             
    jit::MIRGenerator *mir; 
    jit::LIRGraph *lir;     
    unsigned compileTime;

    AsmJSParallelTask(size_t defaultChunkSize)
      : lifo(defaultChunkSize), func(nullptr), mir(nullptr), lir(nullptr), compileTime(0)
    { }

    void init(void *func, jit::MIRGenerator *mir) {
        this->func = func;
        this->mir = mir;
        this->lir = nullptr;
    }
};
#endif

struct ParseTask
{
    ExclusiveContext *cx;
    OwningCompileOptions options;
    const jschar *chars;
    size_t length;
    LifoAlloc alloc;

    
    
    
    JSObject *scopeChain;

    
    JS::OffThreadCompileCallback callback;
    void *callbackData;

    
    
    
    JSScript *script;

    
    
    Vector<frontend::CompileError *> errors;

    ParseTask(ExclusiveContext *cx, JSContext *initCx,
              const jschar *chars, size_t length, JSObject *scopeChain,
              JS::OffThreadCompileCallback callback, void *callbackData);
    bool init(JSContext *cx, const ReadOnlyCompileOptions &options);

    ~ParseTask();
};




struct SourceCompressionTask
{
    friend class ScriptSource;

#ifdef JS_WORKER_THREADS
    
    WorkerThread *workerThread;
#endif

  private:
    
    ExclusiveContext *cx;

    ScriptSource *ss;
    const jschar *chars;
    bool oom;

    
    
#ifdef JS_THREADSAFE
    mozilla::Atomic<int32_t, mozilla::Relaxed> abort_;
#else
    int32_t abort_;
#endif

  public:
    explicit SourceCompressionTask(ExclusiveContext *cx)
      : cx(cx), ss(nullptr), chars(nullptr), oom(false), abort_(0)
    {
#ifdef JS_WORKER_THREADS
        workerThread = nullptr;
#endif
    }

    ~SourceCompressionTask()
    {
        complete();
    }

    bool compress();
    bool complete();
    void abort() { abort_ = 1; }
    bool active() const { return !!ss; }
    ScriptSource *source() { return ss; }
    const jschar *uncompressedChars() { return chars; }
    void setOOM() { oom = true; }
};

} 

#endif 
