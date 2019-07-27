





#ifndef jit_JitCompartment_h
#define jit_JitCompartment_h

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
class JitcodeGlobalTable;








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

    
    
    ExecutableAllocator *execAlloc_;

    
    
    
    
    
    ExecutableAllocator *ionAlloc_;

    
    JitCode *exceptionTail_;

    
    JitCode *bailoutTail_;

    
    JitCode *enterJIT_;

    
    JitCode *enterBaselineJIT_;

    
    Vector<JitCode*, 4, SystemAllocPolicy> bailoutTables_;

    
    JitCode *bailoutHandler_;

    
    JitCode *parallelBailoutHandler_;

    
    
    JitCode *argumentsRectifier_;
    void *argumentsRectifierReturnAddr_;

    
    JitCode *parallelArgumentsRectifier_;

    
    JitCode *invalidator_;

    
    JitCode *valuePreBarrier_;
    JitCode *shapePreBarrier_;
    JitCode *typeObjectPreBarrier_;

    
    JitCode *mallocStub_;
    JitCode *freeStub_;

    
    JitCode *lazyLinkStub_;

    
    JitCode *debugTrapHandler_;

    
    JitCode *forkJoinGetSliceStub_;

    
    JitCode *baselineDebugModeOSRHandler_;
    void *baselineDebugModeOSRHandlerNoFrameRegPopAddr_;

    
    typedef WeakCache<const VMFunction *, JitCode *> VMWrapperMap;
    VMWrapperMap *functionWrappers_;

    
    
    
    uint8_t *osrTempData_;

    
    
    bool ionCodeProtected_;

    
    
    InlineList<PatchableBackedge> backedgeList_;

    
    
    
    
    
    
    
    
    
    
    
    
    js::Value ionReturnOverride_;

    
    JitcodeGlobalTable *jitcodeGlobalTable_;

  private:
    JitCode *generateLazyLinkStub(JSContext *cx);
    JitCode *generateExceptionTailStub(JSContext *cx);
    JitCode *generateBailoutTailStub(JSContext *cx);
    JitCode *generateEnterJIT(JSContext *cx, EnterJitType type);
    JitCode *generateArgumentsRectifier(JSContext *cx, ExecutionMode mode, void **returnAddrOut);
    JitCode *generateBailoutTable(JSContext *cx, uint32_t frameClass);
    JitCode *generateBailoutHandler(JSContext *cx, ExecutionMode mode);
    JitCode *generateInvalidator(JSContext *cx);
    JitCode *generatePreBarrier(JSContext *cx, MIRType type);
    JitCode *generateMallocStub(JSContext *cx);
    JitCode *generateFreeStub(JSContext *cx);
    JitCode *generateDebugTrapHandler(JSContext *cx);
    JitCode *generateForkJoinGetSliceStub(JSContext *cx);
    JitCode *generateBaselineDebugModeOSRHandler(JSContext *cx, uint32_t *noFrameRegPopOffsetOut);
    JitCode *generateVMWrapper(JSContext *cx, const VMFunction &f);

    ExecutableAllocator *createIonAlloc(JSContext *cx);

  public:
    JitRuntime();
    ~JitRuntime();
    bool initialize(JSContext *cx);

    uint8_t *allocateOsrTempData(size_t size);
    void freeOsrTempData();

    static void Mark(JSTracer *trc);

    ExecutableAllocator *execAlloc() const {
        return execAlloc_;
    }

    ExecutableAllocator *getIonAlloc(JSContext *cx) {
        JS_ASSERT(cx->runtime()->currentThreadOwnsInterruptLock());
        return ionAlloc_ ? ionAlloc_ : createIonAlloc(cx);
    }

    ExecutableAllocator *ionAlloc(JSRuntime *rt) {
        JS_ASSERT(rt->currentThreadOwnsInterruptLock());
        return ionAlloc_;
    }

    bool hasIonAlloc() const {
        return !!ionAlloc_;
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

    JitCode *getGenericBailoutHandler(ExecutionMode mode) const {
        switch (mode) {
          case SequentialExecution: return bailoutHandler_;
          case ParallelExecution:   return parallelBailoutHandler_;
          default:                  MOZ_CRASH("No such execution mode");
        }
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
          default:                  MOZ_CRASH("No such execution mode");
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

    JitCode *preBarrier(MIRType type) const {
        switch (type) {
          case MIRType_Value: return valuePreBarrier_;
          case MIRType_Shape: return shapePreBarrier_;
          case MIRType_TypeObject: return typeObjectPreBarrier_;
          default: MOZ_CRASH();
        }
    }

    JitCode *mallocStub() const {
        return mallocStub_;
    }

    JitCode *freeStub() const {
        return freeStub_;
    }

    JitCode *lazyLinkStub() const {
        return lazyLinkStub_;
    }

    bool ensureForkJoinGetSliceStubExists(JSContext *cx);
    JitCode *forkJoinGetSliceStub() const {
        return forkJoinGetSliceStub_;
    }

    bool hasIonReturnOverride() const {
        return !ionReturnOverride_.isMagic(JS_ARG_POISON);
    }
    js::Value takeIonReturnOverride() {
        js::Value v = ionReturnOverride_;
        ionReturnOverride_ = js::MagicValue(JS_ARG_POISON);
        return v;
    }
    void setIonReturnOverride(const js::Value &v) {
        JS_ASSERT(!hasIonReturnOverride());
        JS_ASSERT(!v.isMagic());
        ionReturnOverride_ = v;
    }

    bool hasJitcodeGlobalTable() const {
        return jitcodeGlobalTable_ != nullptr;
    }

    JitcodeGlobalTable *getJitcodeGlobalTable() {
        JS_ASSERT(hasJitcodeGlobalTable());
        return jitcodeGlobalTable_;
    }

    bool isNativeToBytecodeMapEnabled(JSRuntime *rt) {
#ifdef DEBUG
        return true;
#else 
        return rt->spsProfiler.enabled();
#endif 
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

    
    
    void *baselineCallReturnAddr_;
    void *baselineGetPropReturnAddr_;
    void *baselineSetPropReturnAddr_;

    
    
    
    
    
    JitCode *stringConcatStub_;
    JitCode *parallelStringConcatStub_;

    
    
    
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
    void initBaselineCallReturnAddr(void *addr) {
        JS_ASSERT(baselineCallReturnAddr_ == nullptr);
        baselineCallReturnAddr_ = addr;
    }
    void *baselineCallReturnAddr() {
        JS_ASSERT(baselineCallReturnAddr_ != nullptr);
        return baselineCallReturnAddr_;
    }
    void initBaselineGetPropReturnAddr(void *addr) {
        JS_ASSERT(baselineGetPropReturnAddr_ == nullptr);
        baselineGetPropReturnAddr_ = addr;
    }
    void *baselineGetPropReturnAddr() {
        JS_ASSERT(baselineGetPropReturnAddr_ != nullptr);
        return baselineGetPropReturnAddr_;
    }
    void initBaselineSetPropReturnAddr(void *addr) {
        JS_ASSERT(baselineSetPropReturnAddr_ == nullptr);
        baselineSetPropReturnAddr_ = addr;
    }
    void *baselineSetPropReturnAddr() {
        JS_ASSERT(baselineSetPropReturnAddr_ != nullptr);
        return baselineSetPropReturnAddr_;
    }

    bool notifyOfActiveParallelEntryScript(JSContext *cx, HandleScript script);
    bool hasRecentParallelActivity() const;

    void toggleBaselineStubBarriers(bool enabled);

    ExecutableAllocator *createIonAlloc();

  public:
    JitCompartment();
    ~JitCompartment();

    bool initialize(JSContext *cx);

    
    bool ensureIonStubsExist(JSContext *cx);

    void mark(JSTracer *trc, JSCompartment *compartment);
    void sweep(FreeOp *fop, JSCompartment *compartment);

    JitCode *stringConcatStubNoBarrier(ExecutionMode mode) const {
        switch (mode) {
          case SequentialExecution: return stringConcatStub_;
          case ParallelExecution:   return parallelStringConcatStub_;
          default:                  MOZ_CRASH("No such execution mode");
        }
    }
};


void InvalidateAll(FreeOp *fop, JS::Zone *zone);
template <ExecutionMode mode>
void FinishInvalidation(FreeOp *fop, JSScript *script);



#ifdef XP_WIN
const unsigned WINDOWS_BIG_FRAME_TOUCH_INCREMENT = 4096 - 1;
#endif

} 
} 

#endif 
