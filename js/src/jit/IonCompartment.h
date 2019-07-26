





#ifndef jit_IonCompartment_h
#define jit_IonCompartment_h

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
    StackFrame *osrFrame;

    void *calleeToken;

    Value *maxArgv;
    unsigned maxArgc;
    unsigned numActualArgs;
    unsigned osrNumStackValues;

    RootedObject scopeChain;
    RootedValue result;

    bool constructing;
};

typedef void (*EnterIonCode)(void *code, unsigned argc, Value *argv, StackFrame *fp,
                             CalleeToken calleeToken, JSObject *scopeChain,
                             size_t numStackValues, Value *vp);

class IonBuilder;

typedef Vector<IonBuilder*, 0, SystemAllocPolicy> OffThreadCompilationVector;








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
    friend class IonRuntime;

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

class IonRuntime
{
    friend class IonCompartment;

    
    
    JSC::ExecutableAllocator *execAlloc_;

    
    
    
    
    
    JSC::ExecutableAllocator *ionAlloc_;

    
    IonCode *exceptionTail_;

    
    IonCode *bailoutTail_;

    
    IonCode *enterJIT_;

    
    IonCode *enterBaselineJIT_;

    
    Vector<IonCode*, 4, SystemAllocPolicy> bailoutTables_;

    
    IonCode *bailoutHandler_;

    
    
    IonCode *argumentsRectifier_;
    void *argumentsRectifierReturnAddr_;

    
    IonCode *parallelArgumentsRectifier_;

    
    IonCode *invalidator_;

    
    IonCode *valuePreBarrier_;
    IonCode *shapePreBarrier_;

    
    IonCode *debugTrapHandler_;

    
    typedef WeakCache<const VMFunction *, IonCode *> VMWrapperMap;
    VMWrapperMap *functionWrappers_;

    
    
    
    uint8_t *osrTempData_;

    
    AutoFlushCache *flusher_;

    
    
    bool ionCodeProtected_;

    
    
    InlineList<PatchableBackedge> backedgeList_;

  private:
    IonCode *generateExceptionTailStub(JSContext *cx);
    IonCode *generateBailoutTailStub(JSContext *cx);
    IonCode *generateEnterJIT(JSContext *cx, EnterJitType type);
    IonCode *generateArgumentsRectifier(JSContext *cx, ExecutionMode mode, void **returnAddrOut);
    IonCode *generateBailoutTable(JSContext *cx, uint32_t frameClass);
    IonCode *generateBailoutHandler(JSContext *cx);
    IonCode *generateInvalidator(JSContext *cx);
    IonCode *generatePreBarrier(JSContext *cx, MIRType type);
    IonCode *generateDebugTrapHandler(JSContext *cx);
    IonCode *generateVMWrapper(JSContext *cx, const VMFunction &f);

    JSC::ExecutableAllocator *createIonAlloc(JSContext *cx);

  public:
    IonRuntime();
    ~IonRuntime();
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

    JSC::ExecutableAllocator *getIonAlloc(JSContext *cx) {
        JS_ASSERT(cx->runtime()->currentThreadOwnsOperationCallbackLock());
        return ionAlloc_ ? ionAlloc_ : createIonAlloc(cx);
    }

    JSC::ExecutableAllocator *ionAlloc(JSRuntime *rt) {
        JS_ASSERT(rt->currentThreadOwnsOperationCallbackLock());
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

    IonCode *getVMWrapper(const VMFunction &f);
    IonCode *debugTrapHandler(JSContext *cx);

    IonCode *getGenericBailoutHandler() const {
        return bailoutHandler_;
    }

    IonCode *getExceptionTail() const {
        return exceptionTail_;
    }

    IonCode *getBailoutTail() const {
        return bailoutTail_;
    }

    IonCode *getBailoutTable(const FrameSizeClass &frameClass);

    IonCode *getArgumentsRectifier(ExecutionMode mode) const {
        switch (mode) {
          case SequentialExecution: return argumentsRectifier_;
          case ParallelExecution:   return parallelArgumentsRectifier_;
          default:                  MOZ_ASSUME_UNREACHABLE("No such execution mode");
        }
    }

    void *getArgumentsRectifierReturnAddr() const {
        return argumentsRectifierReturnAddr_;
    }

    IonCode *getInvalidationThunk() const {
        return invalidator_;
    }

    EnterIonCode enterIon() const {
        return enterJIT_->as<EnterIonCode>();
    }

    EnterIonCode enterBaseline() const {
        return enterBaselineJIT_->as<EnterIonCode>();
    }

    IonCode *valuePreBarrier() const {
        return valuePreBarrier_;
    }

    IonCode *shapePreBarrier() const {
        return shapePreBarrier_;
    }
};

class IonCompartment
{
    friend class JitActivation;

    
    IonRuntime *rt;

    
    
    
    
    OffThreadCompilationVector finishedOffThreadCompilations_;

    
    typedef WeakValueCache<uint32_t, ReadBarriered<IonCode> > ICStubCodeMap;
    ICStubCodeMap *stubCodes_;

    
    
    void *baselineCallReturnAddr_;
    void *baselineGetPropReturnAddr_;
    void *baselineSetPropReturnAddr_;

    
    OptimizedICStubSpace optimizedStubSpace_;

    
    
    
    
    ReadBarriered<IonCode> stringConcatStub_;
    ReadBarriered<IonCode> parallelStringConcatStub_;

    IonCode *generateStringConcatStub(JSContext *cx, ExecutionMode mode);

  public:
    OffThreadCompilationVector &finishedOffThreadCompilations() {
        return finishedOffThreadCompilations_;
    }

    IonCode *getStubCode(uint32_t key) {
        ICStubCodeMap::AddPtr p = stubCodes_->lookupForAdd(key);
        if (p)
            return p->value;
        return nullptr;
    }
    bool putStubCode(uint32_t key, Handle<IonCode *> stubCode) {
        
        
        
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

    void toggleBaselineStubBarriers(bool enabled);

    JSC::ExecutableAllocator *createIonAlloc();

  public:
    IonCompartment(IonRuntime *rt);
    ~IonCompartment();

    bool initialize(JSContext *cx);

    
    bool ensureIonStubsExist(JSContext *cx);

    void mark(JSTracer *trc, JSCompartment *compartment);
    void sweep(FreeOp *fop);

    JSC::ExecutableAllocator *execAlloc() {
        return rt->execAlloc_;
    }

    IonCode *stringConcatStub(ExecutionMode mode) {
        switch (mode) {
          case SequentialExecution: return stringConcatStub_;
          case ParallelExecution:   return parallelStringConcatStub_;
          default:                  MOZ_ASSUME_UNREACHABLE("No such execution mode");
        }
    }

    OptimizedICStubSpace *optimizedStubSpace() {
        return &optimizedStubSpace_;
    }
};


void InvalidateAll(FreeOp *fop, JS::Zone *zone);
void FinishInvalidation(FreeOp *fop, JSScript *script);



#ifdef XP_WIN
const unsigned WINDOWS_BIG_FRAME_TOUCH_INCREMENT = 4096 - 1;
#endif

} 
} 

#endif 

#endif 
