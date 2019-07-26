





#ifndef ion_IonCompartment_h
#define ion_IonCompartment_h

#ifdef JS_ION

#include "mozilla/MemoryReporting.h"

#include "ion/IonCode.h"
#include "jsweakcache.h"
#include "js/Value.h"
#include "vm/Stack.h"
#include "ion/IonFrames.h"
#include "ion/CompileInfo.h"

namespace js {
namespace ion {

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
    const static size_t STUB_DEFAULT_CHUNK_SIZE = 4 * 1024;

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
    const static size_t STUB_DEFAULT_CHUNK_SIZE = 256;

  public:
    FallbackICStubSpace()
      : ICStubSpace(STUB_DEFAULT_CHUNK_SIZE)
    {}

    inline void adoptFrom(FallbackICStubSpace *other) {
        allocator_.steal(&(other->allocator_));
    }
};

class IonRuntime
{
    friend class IonCompartment;

    
    JSC::ExecutableAllocator *execAlloc_;

    
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

  private:
    IonCode *generateEnterJIT(JSContext *cx, EnterJitType type);
    IonCode *generateArgumentsRectifier(JSContext *cx, ExecutionMode mode, void **returnAddrOut);
    IonCode *generateBailoutTable(JSContext *cx, uint32_t frameClass);
    IonCode *generateBailoutHandler(JSContext *cx);
    IonCode *generateInvalidator(JSContext *cx);
    IonCode *generatePreBarrier(JSContext *cx, MIRType type);
    IonCode *generateDebugTrapHandler(JSContext *cx);
    IonCode *generateVMWrapper(JSContext *cx, const VMFunction &f);

    IonCode *debugTrapHandler(JSContext *cx);

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
};

class IonCompartment
{
    friend class JitActivation;

    
    IonRuntime *rt;

    
    
    
    
    OffThreadCompilationVector finishedOffThreadCompilations_;

    
    typedef WeakValueCache<uint32_t, ReadBarriered<IonCode> > ICStubCodeMap;
    ICStubCodeMap *stubCodes_;

    
    
    void *baselineCallReturnAddr_;

    
    OptimizedICStubSpace optimizedStubSpace_;

    
    
    
    
    ReadBarriered<IonCode> stringConcatStub_;
    ReadBarriered<IonCode> parallelStringConcatStub_;

    IonCode *generateStringConcatStub(JSContext *cx, ExecutionMode mode);

  public:
    IonCode *getVMWrapper(const VMFunction &f);

    OffThreadCompilationVector &finishedOffThreadCompilations() {
        return finishedOffThreadCompilations_;
    }

    IonCode *getStubCode(uint32_t key) {
        ICStubCodeMap::AddPtr p = stubCodes_->lookupForAdd(key);
        if (p)
            return p->value;
        return NULL;
    }
    bool putStubCode(uint32_t key, Handle<IonCode *> stubCode) {
        
        
        
        JS_ASSERT(!stubCodes_->has(key));
        ICStubCodeMap::AddPtr p = stubCodes_->lookupForAdd(key);
        return stubCodes_->add(p, key, stubCode.get());
    }
    void initBaselineCallReturnAddr(void *addr) {
        JS_ASSERT(baselineCallReturnAddr_ == NULL);
        baselineCallReturnAddr_ = addr;
    }
    void *baselineCallReturnAddr() {
        JS_ASSERT(baselineCallReturnAddr_ != NULL);
        return baselineCallReturnAddr_;
    }

    void toggleBaselineStubBarriers(bool enabled);

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

    IonCode *getGenericBailoutHandler() {
        return rt->bailoutHandler_;
    }

    IonCode *getBailoutTable(const FrameSizeClass &frameClass);

    IonCode *getArgumentsRectifier(ExecutionMode mode) {
        switch (mode) {
          case SequentialExecution: return rt->argumentsRectifier_;
          case ParallelExecution:   return rt->parallelArgumentsRectifier_;
          default:                  MOZ_ASSUME_NOT_REACHED("No such execution mode");
        }
    }

    void *getArgumentsRectifierReturnAddr() {
        return rt->argumentsRectifierReturnAddr_;
    }

    IonCode *getInvalidationThunk() {
        return rt->invalidator_;
    }

    EnterIonCode enterJIT() {
        return rt->enterJIT_->as<EnterIonCode>();
    }

    EnterIonCode enterBaselineJIT() {
        return rt->enterBaselineJIT_->as<EnterIonCode>();
    }

    IonCode *valuePreBarrier() {
        return rt->valuePreBarrier_;
    }

    IonCode *shapePreBarrier() {
        return rt->shapePreBarrier_;
    }

    IonCode *debugTrapHandler(JSContext *cx) {
        return rt->debugTrapHandler(cx);
    }

    IonCode *stringConcatStub(ExecutionMode mode) {
        switch (mode) {
          case SequentialExecution: return stringConcatStub_;
          case ParallelExecution:   return parallelStringConcatStub_;
          default:                  MOZ_ASSUME_NOT_REACHED("No such execution mode");
        }
    }

    AutoFlushCache *flusher() {
        return rt->flusher();
    }
    void setFlusher(AutoFlushCache *fl) {
        rt->setFlusher(fl);
    }
    OptimizedICStubSpace *optimizedStubSpace() {
        return &optimizedStubSpace_;
    }
};


void InvalidateAll(FreeOp *fop, JS::Zone *zone);
void FinishInvalidation(FreeOp *fop, JSScript *script);

} 
} 

#endif 

#endif 
