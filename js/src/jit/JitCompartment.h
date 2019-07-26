





#ifndef jit_JitCompartment_h
#define jit_JitCompartment_h

#ifdef JS_ION

#include "mozilla/MemoryReporting.h"

#include "jsweakcache.h"

#include "jit/CompileInfo.h"
#include "jit/IonCode.h"
#include "jit/IonFrames.h"
#include "jit/shared/Assembler-shared.h"
#include "js/Value.h"
#include "vm/Stack.h"

namespace js {
namespace jit {

class FrameSizeClass;

enum EnterJitType {
    EnterJitBaseline = 0,
    EnterJitOptimized = 1
};

struct EnterJitData
{
    explicit EnterJitData(JSContext *cx)
      : scopeChain(cx),
        result(cx)
    {}

    uint8_t *jitcode;
    InterpreterFrame *osrFrame;

    void *calleeToken;

    Value *maxArgv;
    unsigned maxArgc;
    unsigned numActualArgs;
    unsigned osrNumStackValues;

    RootedObject scopeChain;
    RootedValue result;

    bool constructing;
};

typedef void (*EnterJitCode)(void *code, unsigned argc, Value *argv, InterpreterFrame *fp,
                             CalleeToken calleeToken, JSObject *scopeChain,
                             size_t numStackValues, Value *vp);

class IonBuilder;








class ICStubSpace
{
  protected:
    LifoAlloc allocator_;

    explicit ICStubSpace(size_t chunkSize)
      : allocator_(chunkSize)
    {}

  public:
    inline void *alloc(size_t size) {
        return allocator_.alloc(size);
    }

    JS_DECLARE_NEW_METHODS(allocate, alloc, inline)

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const {
        return allocator_.sizeOfExcludingThis(mallocSizeOf);
    }
};



struct OptimizedICStubSpace : public ICStubSpace
{
    static const size_t STUB_DEFAULT_CHUNK_SIZE = 4 * 1024;

  public:
    OptimizedICStubSpace()
      : ICStubSpace(STUB_DEFAULT_CHUNK_SIZE)
    {}

    void free() {
        allocator_.freeAll();
    }
};



struct FallbackICStubSpace : public ICStubSpace
{
    static const size_t STUB_DEFAULT_CHUNK_SIZE = 256;

  public:
    FallbackICStubSpace()
      : ICStubSpace(STUB_DEFAULT_CHUNK_SIZE)
    {}

    inline void adoptFrom(FallbackICStubSpace *other) {
        allocator_.steal(&(other->allocator_));
    }
};




class PatchableBackedge : public InlineListNode<PatchableBackedge>
{
    friend class JitRuntime;

    CodeLocationJump backedge;
    CodeLocationLabel loopHeader;
    CodeLocationLabel interruptCheck;

  public:
    PatchableBackedge(CodeLocationJump backedge,
                      CodeLocationLabel loopHeader,
                      CodeLocationLabel interruptCheck)
      : backedge(backedge), loopHeader(loopHeader), interruptCheck(interruptCheck)
    {}
};

class JitRuntime
{
    friend class JitCompartment;

    
    
    JSC::ExecutableAllocator *execAlloc_;

    
    
    
    
    
    JSC::ExecutableAllocator *ionAlloc_;

    
    JitCode *exceptionTail_;

    
    JitCode *bailoutTail_;

    
    JitCode *enterJIT_;

    
    JitCode *enterBaselineJIT_;

    
    Vector<JitCode*, 4, SystemAllocPolicy> bailoutTables_;

    
    JitCode *bailoutHandler_;

    
    
    JitCode *argumentsRectifier_;
    void *argumentsRectifierReturnAddr_;

    
    JitCode *parallelArgumentsRectifier_;

    
    JitCode *invalidator_;

    
    JitCode *valuePreBarrier_;
    JitCode *shapePreBarrier_;

    
    JitCode *debugTrapHandler_;

    
    JitCode *forkJoinGetSliceStub_;

    
    JitCode *baselineDebugModeOSRHandler_;
    void *baselineDebugModeOSRHandlerNoFrameRegPopAddr_;

    
    typedef WeakCache<const VMFunction *, JitCode *> VMWrapperMap;
    VMWrapperMap *functionWrappers_;

    
    
    
    uint8_t *osrTempData_;

    
    AutoFlushCache *flusher_;

    
    
    bool ionCodeProtected_;

    
    
    InlineList<PatchableBackedge> backedgeList_;

  private:
    JitCode *generateExceptionTailStub(JSContext *cx);
    JitCode *generateBailoutTailStub(JSContext *cx);
    JitCode *generateEnterJIT(JSContext *cx, EnterJitType type);
    JitCode *generateArgumentsRectifier(JSContext *cx, ExecutionMode mode, void **returnAddrOut);
    JitCode *generateBailoutTable(JSContext *cx, uint32_t frameClass);
    JitCode *generateBailoutHandler(JSContext *cx);
    JitCode *generateInvalidator(JSContext *cx);
    JitCode *generatePreBarrier(JSContext *cx, MIRType type);
    JitCode *generateDebugTrapHandler(JSContext *cx);
    JitCode *generateForkJoinGetSliceStub(JSContext *cx);
    JitCode *generateBaselineDebugModeOSRHandler(JSContext *cx, uint32_t *noFrameRegPopOffsetOut);
    JitCode *generateVMWrapper(JSContext *cx, const VMFunction &f);

    JSC::ExecutableAllocator *createIonAlloc(JSContext *cx);

  public:
    JitRuntime();
    ~JitRuntime();
    bool initialize(JSContext *cx);

    uint8_t *allocateOsrTempData(size_t size);
    void freeOsrTempData();

    static void Mark(JSTracer *trc);

    AutoFlushCache *flusher() {
        return flusher_;
    }
    void setFlusher(AutoFlushCache *fl) {
        if (!flusher_ || !fl)
            flusher_ = fl;
    }

    JSC::ExecutableAllocator *execAlloc() const {
        return execAlloc_;
    }

    JSC::ExecutableAllocator *getIonAlloc(JSContext *cx) {
        JS_ASSERT(cx->runtime()->currentThreadOwnsInterruptLock());
        return ionAlloc_ ? ionAlloc_ : createIonAlloc(cx);
    }

    JSC::ExecutableAllocator *ionAlloc(JSRuntime *rt) {
        JS_ASSERT(rt->currentThreadOwnsInterruptLock());
        return ionAlloc_;
    }

    bool ionCodeProtected() {
        return ionCodeProtected_;
    }

    void addPatchableBackedge(PatchableBackedge *backedge) {
        backedgeList_.pushFront(backedge);
    }
    void removePatchableBackedge(PatchableBackedge *backedge) {
        backedgeList_.remove(backedge);
    }

    enum BackedgeTarget {
        BackedgeLoopHeader,
        BackedgeInterruptCheck
    };

    void ensureIonCodeProtected(JSRuntime *rt);
    void ensureIonCodeAccessible(JSRuntime *rt);
    void patchIonBackedges(JSRuntime *rt, BackedgeTarget target);

    bool handleAccessViolation(JSRuntime *rt, void *faultingAddress);

    JitCode *getVMWrapper(const VMFunction &f) const;
    JitCode *debugTrapHandler(JSContext *cx);
    JitCode *getBaselineDebugModeOSRHandler(JSContext *cx);
    void *getBaselineDebugModeOSRHandlerAddress(JSContext *cx, bool popFrameReg);

    JitCode *getGenericBailoutHandler() const {
        return bailoutHandler_;
    }

    JitCode *getExceptionTail() const {
        return exceptionTail_;
    }

    JitCode *getBailoutTail() const {
        return bailoutTail_;
    }

    JitCode *getBailoutTable(const FrameSizeClass &frameClass) const;

    JitCode *getArgumentsRectifier(ExecutionMode mode) const {
        switch (mode) {
          case SequentialExecution: return argumentsRectifier_;
          case ParallelExecution:   return parallelArgumentsRectifier_;
          default:                  MOZ_ASSUME_UNREACHABLE("No such execution mode");
        }
    }

    void *getArgumentsRectifierReturnAddr() const {
        return argumentsRectifierReturnAddr_;
    }

    JitCode *getInvalidationThunk() const {
        return invalidator_;
    }

    EnterJitCode enterIon() const {
        return enterJIT_->as<EnterJitCode>();
    }

    EnterJitCode enterBaseline() const {
        return enterBaselineJIT_->as<EnterJitCode>();
    }

    JitCode *valuePreBarrier() const {
        return valuePreBarrier_;
    }

    JitCode *shapePreBarrier() const {
        return shapePreBarrier_;
    }

    bool ensureForkJoinGetSliceStubExists(JSContext *cx);
    JitCode *forkJoinGetSliceStub() const {
        return forkJoinGetSliceStub_;
    }
};

class JitZone
{
    
    OptimizedICStubSpace optimizedStubSpace_;

  public:
    OptimizedICStubSpace *optimizedStubSpace() {
        return &optimizedStubSpace_;
    }
};

class JitCompartment
{
    friend class JitActivation;

    
    typedef WeakValueCache<uint32_t, ReadBarrieredJitCode> ICStubCodeMap;
    ICStubCodeMap *stubCodes_;

    
    
    void *baselineCallReturnFromIonAddr_;
    void *baselineGetPropReturnFromIonAddr_;
    void *baselineSetPropReturnFromIonAddr_;

    
    
    
    void *baselineCallReturnFromStubAddr_;
    void *baselineGetPropReturnFromStubAddr_;
    void *baselineSetPropReturnFromStubAddr_;

    
    
    
    
    ReadBarrieredJitCode stringConcatStub_;
    ReadBarrieredJitCode parallelStringConcatStub_;

    
    
    
    typedef HashSet<PreBarrieredScript> ScriptSet;
    ScriptSet *activeParallelEntryScripts_;

    JitCode *generateStringConcatStub(JSContext *cx, ExecutionMode mode);

  public:
    JitCode *getStubCode(uint32_t key) {
        ICStubCodeMap::AddPtr p = stubCodes_->lookupForAdd(key);
        if (p)
            return p->value();
        return nullptr;
    }
    bool putStubCode(uint32_t key, Handle<JitCode *> stubCode) {
        
        
        
        JS_ASSERT(!stubCodes_->has(key));
        ICStubCodeMap::AddPtr p = stubCodes_->lookupForAdd(key);
        return stubCodes_->add(p, key, stubCode.get());
    }
    void initBaselineCallReturnFromIonAddr(void *addr) {
        JS_ASSERT(baselineCallReturnFromIonAddr_ == nullptr);
        baselineCallReturnFromIonAddr_ = addr;
    }
    void *baselineCallReturnFromIonAddr() {
        JS_ASSERT(baselineCallReturnFromIonAddr_ != nullptr);
        return baselineCallReturnFromIonAddr_;
    }
    void initBaselineGetPropReturnFromIonAddr(void *addr) {
        JS_ASSERT(baselineGetPropReturnFromIonAddr_ == nullptr);
        baselineGetPropReturnFromIonAddr_ = addr;
    }
    void *baselineGetPropReturnFromIonAddr() {
        JS_ASSERT(baselineGetPropReturnFromIonAddr_ != nullptr);
        return baselineGetPropReturnFromIonAddr_;
    }
    void initBaselineSetPropReturnFromIonAddr(void *addr) {
        JS_ASSERT(baselineSetPropReturnFromIonAddr_ == nullptr);
        baselineSetPropReturnFromIonAddr_ = addr;
    }
    void *baselineSetPropReturnFromIonAddr() {
        JS_ASSERT(baselineSetPropReturnFromIonAddr_ != nullptr);
        return baselineSetPropReturnFromIonAddr_;
    }

    void initBaselineCallReturnFromStubAddr(void *addr) {
        MOZ_ASSERT(baselineCallReturnFromStubAddr_ == nullptr);
        baselineCallReturnFromStubAddr_ = addr;;
    }
    void *baselineCallReturnFromStubAddr() {
        JS_ASSERT(baselineCallReturnFromStubAddr_ != nullptr);
        return baselineCallReturnFromStubAddr_;
    }
    void initBaselineGetPropReturnFromStubAddr(void *addr) {
        JS_ASSERT(baselineGetPropReturnFromStubAddr_ == nullptr);
        baselineGetPropReturnFromStubAddr_ = addr;
    }
    void *baselineGetPropReturnFromStubAddr() {
        JS_ASSERT(baselineGetPropReturnFromStubAddr_ != nullptr);
        return baselineGetPropReturnFromStubAddr_;
    }
    void initBaselineSetPropReturnFromStubAddr(void *addr) {
        JS_ASSERT(baselineSetPropReturnFromStubAddr_ == nullptr);
        baselineSetPropReturnFromStubAddr_ = addr;
    }
    void *baselineSetPropReturnFromStubAddr() {
        JS_ASSERT(baselineSetPropReturnFromStubAddr_ != nullptr);
        return baselineSetPropReturnFromStubAddr_;
    }

    bool notifyOfActiveParallelEntryScript(JSContext *cx, HandleScript script);

    void toggleBaselineStubBarriers(bool enabled);

    JSC::ExecutableAllocator *createIonAlloc();

  public:
    JitCompartment();
    ~JitCompartment();

    bool initialize(JSContext *cx);

    
    bool ensureIonStubsExist(JSContext *cx);

    void mark(JSTracer *trc, JSCompartment *compartment);
    void sweep(FreeOp *fop);

    JitCode *stringConcatStub(ExecutionMode mode) const {
        switch (mode) {
          case SequentialExecution: return stringConcatStub_;
          case ParallelExecution:   return parallelStringConcatStub_;
          default:                  MOZ_ASSUME_UNREACHABLE("No such execution mode");
        }
    }
};


void InvalidateAll(FreeOp *fop, JS::Zone *zone);
template <ExecutionMode mode>
void FinishInvalidation(FreeOp *fop, JSScript *script);

inline bool
ShouldPreserveParallelJITCode(JSRuntime *rt, JSScript *script, bool increase = false)
{
    IonScript *parallelIon = script->parallelIonScript();
    uint32_t age = increase ? parallelIon->increaseParallelAge() : parallelIon->parallelAge();
    return age < jit::IonScript::MAX_PARALLEL_AGE && !rt->gcShouldCleanUpEverything;
}



#ifdef XP_WIN
const unsigned WINDOWS_BIG_FRAME_TOUCH_INCREMENT = 4096 - 1;
#endif

} 
} 

#endif 

#endif 
