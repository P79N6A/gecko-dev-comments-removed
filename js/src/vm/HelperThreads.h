











#ifndef vm_HelperThreads_h
#define vm_HelperThreads_h

#include "mozilla/GuardObjects.h"
#include "mozilla/PodOperations.h"

#include "jscntxt.h"
#include "jslock.h"

#include "frontend/TokenStream.h"
#include "jit/Ion.h"

namespace js {

struct HelperThread;
struct AsmJSParallelTask;
struct ParseTask;
namespace jit {
  class IonBuilder;
}


class GlobalHelperThreadState
{
  public:
    
    
    size_t cpuCount;

    
    size_t threadCount;

    typedef Vector<jit::IonBuilder*, 0, SystemAllocPolicy> IonBuilderVector;
    typedef Vector<AsmJSParallelTask*, 0, SystemAllocPolicy> AsmJSParallelTaskVector;
    typedef Vector<ParseTask*, 0, SystemAllocPolicy> ParseTaskVector;
    typedef Vector<SourceCompressionTask*, 0, SystemAllocPolicy> SourceCompressionTaskVector;
    typedef Vector<GCHelperState *, 0, SystemAllocPolicy> GCHelperStateVector;
    typedef mozilla::LinkedList<jit::IonBuilder> IonBuilderList;

    
    HelperThread *threads;

  private:
    

    
    IonBuilderVector ionWorklist_, ionFinishedList_;

    
    IonBuilderList ionLazyLinkList_;

    
    
    
    
    
    
    AsmJSParallelTaskVector asmJSWorklist_, asmJSFinishedList_;

  public:
    
    
    mozilla::Atomic<bool> asmJSCompilationInProgress;

  private:
    
    ParseTaskVector parseWorklist_, parseFinishedList_;

    
    ParseTaskVector parseWaitingOnGC_;

    
    SourceCompressionTaskVector compressionWorklist_;

    
    GCHelperStateVector gcHelperWorklist_;

  public:
    size_t maxIonCompilationThreads() const {
        return 1;
    }
    size_t maxAsmJSCompilationThreads() const {
        if (cpuCount < 2)
            return 2;
        return cpuCount;
    }

    GlobalHelperThreadState();

    void ensureInitialized();
    void finish();

    void lock();
    void unlock();

#ifdef DEBUG
    bool isLocked();
#endif

    enum CondVar {
        
        CONSUMER,

        
        PRODUCER,

        
        
        PAUSE
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
    IonBuilderList &ionLazyLinkList() {
        JS_ASSERT(isLocked());
        return ionLazyLinkList_;
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

    GCHelperStateVector &gcHelperWorklist() {
        JS_ASSERT(isLocked());
        return gcHelperWorklist_;
    }

    bool canStartAsmJSCompile();
    bool canStartIonCompile();
    bool canStartParseTask();
    bool canStartCompressionTask();
    bool canStartGCHelperTask();

    
    
    bool pendingIonCompileHasSufficientPriority();

    jit::IonBuilder *highestPriorityPendingIonCompile(bool remove = false);
    HelperThread *lowestPriorityUnpausedIonCompileAtThreshold();
    HelperThread *highestPriorityPausedIonCompile();

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
    bool asmJSFailed() const {
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

    



    PRLock *helperLock;
#ifdef DEBUG
    PRThread *lockOwner;
#endif

    
    PRCondVar *consumerWakeup;
    PRCondVar *producerWakeup;
    PRCondVar *pauseWakeup;

    PRCondVar *whichWakeup(CondVar which) {
        switch (which) {
          case CONSUMER: return consumerWakeup;
          case PRODUCER: return producerWakeup;
          case PAUSE: return pauseWakeup;
          default: MOZ_CRASH();
        }
    }

    



    uint32_t numAsmJSFailedJobs;

    



    void *asmJSFailedFunction;
};

static inline GlobalHelperThreadState &
HelperThreadState()
{
    extern GlobalHelperThreadState gHelperThreadState;
    return gHelperThreadState;
}


struct HelperThread
{
    mozilla::Maybe<PerThreadData> threadData;
    PRThread *thread;

    



    bool terminate;

    




    mozilla::Atomic<bool, mozilla::Relaxed> pause;

    
    jit::IonBuilder *ionBuilder;

    
    AsmJSParallelTask *asmData;

    
    ParseTask *parseTask;

    
    SourceCompressionTask *compressionTask;

    
    GCHelperState *gcHelperState;

    bool idle() const {
        return !ionBuilder && !asmData && !parseTask && !compressionTask && !gcHelperState;
    }

    void destroy();

    void handleAsmJSWorkload();
    void handleIonWorkload();
    void handleParseWorkload();
    void handleCompressionWorkload();
    void handleGCHelperWorkload();

    static void ThreadMain(void *arg);
    void threadLoop();
};




void
EnsureHelperThreadsInitialized(ExclusiveContext *cx);



void
SetFakeCPUCount(size_t count);


void
PauseCurrentHelperThread();


bool
StartOffThreadAsmJSCompile(ExclusiveContext *cx, AsmJSParallelTask *asmData);





bool
StartOffThreadIonCompile(JSContext *cx, jit::IonBuilder *builder);





void
CancelOffThreadIonCompile(JSCompartment *compartment, JSScript *script);


void
CancelOffThreadParses(JSRuntime *runtime);





bool
StartOffThreadParseScript(JSContext *cx, const ReadOnlyCompileOptions &options,
                          const char16_t *chars, size_t length,
                          JS::OffThreadCompileCallback callback, void *callbackData);





void
EnqueuePendingParseTasksAfterGC(JSRuntime *rt);


bool
StartOffThreadCompression(ExclusiveContext *cx, SourceCompressionTask *task);

class AutoLockHelperThreadState
{
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:
    explicit AutoLockHelperThreadState(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        HelperThreadState().lock();
    }

    ~AutoLockHelperThreadState() {
        HelperThreadState().unlock();
    }
};

class AutoUnlockHelperThreadState
{
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER

  public:

    explicit AutoUnlockHelperThreadState(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        HelperThreadState().unlock();
    }

    ~AutoUnlockHelperThreadState()
    {
        HelperThreadState().lock();
    }
};

struct AsmJSParallelTask
{
    JSRuntime *runtime;     
    LifoAlloc lifo;         
    void *func;             
    jit::MIRGenerator *mir; 
    jit::LIRGraph *lir;     
    unsigned compileTime;

    explicit AsmJSParallelTask(size_t defaultChunkSize)
      : runtime(nullptr), lifo(defaultChunkSize), func(nullptr), mir(nullptr), lir(nullptr), compileTime(0)
    { }

    void init(JSRuntime *rt, void *func, jit::MIRGenerator *mir) {
        this->runtime = rt;
        this->func = func;
        this->mir = mir;
        this->lir = nullptr;
    }
};

struct ParseTask
{
    ExclusiveContext *cx;
    OwningCompileOptions options;
    const char16_t *chars;
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
              JSContext *initCx, const char16_t *chars, size_t length,
              JS::OffThreadCompileCallback callback, void *callbackData);
    bool init(JSContext *cx, const ReadOnlyCompileOptions &options);

    void activate(JSRuntime *rt);
    bool finish(JSContext *cx);

    bool runtimeMatches(JSRuntime *rt) {
        return exclusiveContextGlobal->runtimeFromAnyThread() == rt;
    }

    ~ParseTask();
};



extern bool
OffThreadParsingMustWaitForGC(JSRuntime *rt);




struct SourceCompressionTask
{
    friend class ScriptSource;
    friend struct HelperThread;

    
    HelperThread *helperThread;

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
    HashNumber compressedHash;

  public:
    explicit SourceCompressionTask(ExclusiveContext *cx)
      : helperThread(nullptr), cx(cx), ss(nullptr), abort_(false),
        result(OOM), compressed(nullptr), compressedBytes(0), compressedHash(0)
    {}

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
