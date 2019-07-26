











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

#ifdef JS_THREADSAFE


class GlobalWorkerThreadState
{
  public:
    
    
    size_t cpuCount;

    
    size_t threadCount;

    typedef Vector<jit::IonBuilder*, 0, SystemAllocPolicy> IonBuilderVector;
    typedef Vector<AsmJSParallelTask*, 0, SystemAllocPolicy> AsmJSParallelTaskVector;
    typedef Vector<ParseTask*, 0, SystemAllocPolicy> ParseTaskVector;
    typedef Vector<SourceCompressionTask*, 0, SystemAllocPolicy> SourceCompressionTaskVector;

    
    WorkerThread *threads;

  private:
    

    
    IonBuilderVector ionWorklist_, ionFinishedList_;

    
    
    
    
    
    
    AsmJSParallelTaskVector asmJSWorklist_, asmJSFinishedList_;

  public:
    
    
    mozilla::Atomic<bool> asmJSCompilationInProgress;

  private:
    
    ParseTaskVector parseWorklist_, parseFinishedList_;

    
    ParseTaskVector parseWaitingOnGC_;

    
    SourceCompressionTaskVector compressionWorklist_;

  public:
    GlobalWorkerThreadState();

    void ensureInitialized();
    void finish();

    void lock();
    void unlock();

# ifdef DEBUG
    bool isLocked();
# endif

    enum CondVar {
        
        CONSUMER,

        
        PRODUCER
    };

    void wait(CondVar which, uint32_t timeoutMillis = 0);
    void notifyAll(CondVar which);
    void notifyOne(CondVar which);

    
    template <typename T>
    void remove(T &vector, size_t *index)
    {
        vector[(*index)--] = vector.back();
        vector.popBack();
    }

    IonBuilderVector &ionWorklist() {
        JS_ASSERT(isLocked());
        return ionWorklist_;
    }
    IonBuilderVector &ionFinishedList() {
        JS_ASSERT(isLocked());
        return ionFinishedList_;
    }

    AsmJSParallelTaskVector &asmJSWorklist() {
        JS_ASSERT(isLocked());
        return asmJSWorklist_;
    }
    AsmJSParallelTaskVector &asmJSFinishedList() {
        JS_ASSERT(isLocked());
        return asmJSFinishedList_;
    }

    ParseTaskVector &parseWorklist() {
        JS_ASSERT(isLocked());
        return parseWorklist_;
    }
    ParseTaskVector &parseFinishedList() {
        JS_ASSERT(isLocked());
        return parseFinishedList_;
    }
    ParseTaskVector &parseWaitingOnGC() {
        JS_ASSERT(isLocked());
        return parseWaitingOnGC_;
    }

    SourceCompressionTaskVector &compressionWorklist() {
        JS_ASSERT(isLocked());
        return compressionWorklist_;
    }

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

    



    PRLock *workerLock;

# ifdef DEBUG
    PRThread *lockOwner;
# endif

    
    PRCondVar *consumerWakeup;
    PRCondVar *producerWakeup;

    



    uint32_t numAsmJSFailedJobs;

    



    void *asmJSFailedFunction;
};

static inline GlobalWorkerThreadState &
WorkerThreadState()
{
    extern GlobalWorkerThreadState gWorkerThreadState;
    return gWorkerThreadState;
}


struct WorkerThread
{
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

    void handleAsmJSWorkload();
    void handleIonWorkload();
    void handleParseWorkload();
    void handleCompressionWorkload();

    static void ThreadMain(void *arg);
    void threadLoop();
};

#endif 




void
EnsureWorkerThreadsInitialized(ExclusiveContext *cx);



void
SetFakeCPUCount(size_t count);

#ifdef JS_ION


bool
StartOffThreadAsmJSCompile(ExclusiveContext *cx, AsmJSParallelTask *asmData);





bool
StartOffThreadIonCompile(JSContext *cx, jit::IonBuilder *builder);

#endif 





void
CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script);


void
CancelOffThreadParses(JSRuntime *runtime);





bool
StartOffThreadParseScript(JSContext *cx, const ReadOnlyCompileOptions &options,
                          const jschar *chars, size_t length,
                          JS::OffThreadCompileCallback callback, void *callbackData);





void
EnqueuePendingParseTasksAfterGC(JSRuntime *rt);


bool
StartOffThreadCompression(ExclusiveContext *cx, SourceCompressionTask *task);

class AutoLockWorkerThreadState
{
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

#ifdef JS_THREADSAFE
  public:
    AutoLockWorkerThreadState(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        WorkerThreadState().lock();
    }

    ~AutoLockWorkerThreadState() {
        WorkerThreadState().unlock();
    }
#else
  public:
    AutoLockWorkerThreadState(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
#endif
};

class AutoUnlockWorkerThreadState
{
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:

    AutoUnlockWorkerThreadState(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
#ifdef JS_THREADSAFE
        WorkerThreadState().unlock();
#endif
    }

    ~AutoUnlockWorkerThreadState()
    {
#ifdef JS_THREADSAFE
        WorkerThreadState().lock();
#endif
    }
};

#ifdef JS_ION
struct AsmJSParallelTask
{
    JSRuntime *runtime;     
    LifoAlloc lifo;         
    void *func;             
    jit::MIRGenerator *mir; 
    jit::LIRGraph *lir;     
    unsigned compileTime;

    AsmJSParallelTask(size_t defaultChunkSize)
      : runtime(nullptr), lifo(defaultChunkSize), func(nullptr), mir(nullptr), lir(nullptr), compileTime(0)
    { }

    void init(JSRuntime *rt, void *func, jit::MIRGenerator *mir) {
        this->runtime = rt;
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

    
    PersistentRootedObject exclusiveContextGlobal;

    
    
    
    
    
    PersistentRootedObject optionsElement;
    PersistentRootedScript optionsIntroductionScript;

    
    JS::OffThreadCompileCallback callback;
    void *callbackData;

    
    
    
    JSScript *script;

    
    
    Vector<frontend::CompileError *> errors;
    bool overRecursed;

    ParseTask(ExclusiveContext *cx, JSObject *exclusiveContextGlobal,
              JSContext *initCx, const jschar *chars, size_t length,
              JS::OffThreadCompileCallback callback, void *callbackData);
    bool init(JSContext *cx, const ReadOnlyCompileOptions &options);

    void activate(JSRuntime *rt);
    void finish();

    bool runtimeMatches(JSRuntime *rt) {
        return exclusiveContextGlobal->runtimeFromAnyThread() == rt;
    }

    ~ParseTask();
};

#ifdef JS_THREADSAFE


extern bool
OffThreadParsingMustWaitForGC(JSRuntime *rt);
#endif




struct SourceCompressionTask
{
    friend class ScriptSource;
    friend class WorkerThread;

#ifdef JS_THREADSAFE
    
    WorkerThread *workerThread;
#endif

  private:
    
    ExclusiveContext *cx;

    ScriptSource *ss;

    
    
    mozilla::Atomic<bool, mozilla::Relaxed> abort_;

    
    enum ResultType {
        OOM,
        Aborted,
        Success
    } result;
    void *compressed;
    size_t compressedBytes;

  public:
    explicit SourceCompressionTask(ExclusiveContext *cx)
      : cx(cx), ss(nullptr), abort_(false),
        result(OOM), compressed(nullptr), compressedBytes(0)
    {
#ifdef JS_THREADSAFE
        workerThread = nullptr;
#endif
    }

    ~SourceCompressionTask()
    {
        complete();
    }

    ResultType work();
    bool complete();
    void abort() { abort_ = true; }
    bool active() const { return !!ss; }
    ScriptSource *source() { return ss; }
};

} 

#endif 
