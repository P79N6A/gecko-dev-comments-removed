






#ifndef jsion_ion_compartment_h__
#define jsion_ion_compartment_h__

#include "IonCode.h"
#include "jsweakcache.h"
#include "js/Value.h"
#include "vm/Stack.h"
#include "IonFrames.h"

namespace js {
namespace ion {

class FrameSizeClass;

enum EnterJitType {
    EnterJitBaseline = 0,
    EnterJitOptimized = 1
};

typedef void (*EnterIonCode)(void *code, int argc, Value *argv, StackFrame *fp,
                             CalleeToken calleeToken, JSObject *evalScopeChain, Value *vp);

class IonActivation;
class IonBuilder;

typedef Vector<IonBuilder*, 0, SystemAllocPolicy> OffThreadCompilationVector;

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

    
    IonCode *invalidator_;

    
    IonCode *valuePreBarrier_;
    IonCode *shapePreBarrier_;

    
    IonCode *debugTrapHandler_;

    
    typedef WeakCache<const VMFunction *, IonCode *> VMWrapperMap;
    VMWrapperMap *functionWrappers_;

    
    
    
    uint8_t *osrTempData_;

  private:
    IonCode *generateEnterJIT(JSContext *cx, EnterJitType type);
    IonCode *generateArgumentsRectifier(JSContext *cx, void **returnAddrOut);
    IonCode *generateBailoutTable(JSContext *cx, uint32_t frameClass);
    IonCode *generateBailoutHandler(JSContext *cx);
    IonCode *generateInvalidator(JSContext *cx);
    IonCode *generatePreBarrier(JSContext *cx, MIRType type);
    IonCode *generateDebugTrapHandler(JSContext *cx);
    IonCode *generateVMWrapper(JSContext *cx, const VMFunction &f);

    IonCode *debugTrapHandler(JSContext *cx) {
        if (!debugTrapHandler_) {
            
            
            AutoEnterAtomsCompartment ac(cx);
            debugTrapHandler_ = generateDebugTrapHandler(cx);
        }
        return debugTrapHandler_;
    }

  public:
    IonRuntime();
    ~IonRuntime();
    bool initialize(JSContext *cx);

    uint8_t *allocateOsrTempData(size_t size);
    void freeOsrTempData();

    static void Mark(JSTracer *trc);
};

class IonCompartment
{
    friend class IonActivation;

    
    IonRuntime *rt;

    
    
    
    
    OffThreadCompilationVector finishedOffThreadCompilations_;

    
    AutoFlushCache *flusher_;

    
    typedef WeakValueCache<uint32_t, ReadBarriered<IonCode> > ICStubCodeMap;
    ICStubCodeMap *stubCodes_;

    
    
    void *baselineCallReturnAddr_;

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

    void mark(JSTracer *trc, JSCompartment *compartment);
    void sweep(FreeOp *fop);

    JSC::ExecutableAllocator *execAlloc() {
        return rt->execAlloc_;
    }

    IonCode *getGenericBailoutHandler() {
        return rt->bailoutHandler_;
    }

    IonCode *getBailoutTable(const FrameSizeClass &frameClass);

    IonCode *getArgumentsRectifier() {
        return rt->argumentsRectifier_;
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

    AutoFlushCache *flusher() {
        return flusher_;
    }
    void setFlusher(AutoFlushCache *fl) {
        if (!flusher_ || !fl)
            flusher_ = fl;
    }
};

class BailoutClosure;

class IonActivation
{
  private:
    JSContext *cx_;
    JSCompartment *compartment_;
    IonActivation *prev_;
    StackFrame *entryfp_;
    BailoutClosure *bailout_;
    uint8_t *prevIonTop_;
    JSContext *prevIonJSContext_;

    
    
    jsbytecode *prevpc_;

  public:
    IonActivation(JSContext *cx, StackFrame *fp);
    ~IonActivation();

    StackFrame *entryfp() const {
        return entryfp_;
    }
    IonActivation *prev() const {
        return prev_;
    }
    uint8_t *prevIonTop() const {
        return prevIonTop_;
    }
    jsbytecode *prevpc() const {
        JS_ASSERT_IF(entryfp_, entryfp_->callingIntoIon());
        return prevpc_;
    }
    void setEntryFp(StackFrame *fp) {
        JS_ASSERT_IF(fp, !entryfp_);
        entryfp_ = fp;
    }
    void setPrevPc(jsbytecode *pc) {
        JS_ASSERT_IF(pc, !prevpc_);
        prevpc_ = pc;
    }
    void setBailout(BailoutClosure *bailout) {
        JS_ASSERT(!bailout_);
        bailout_ = bailout;
    }
    BailoutClosure *maybeTakeBailout() {
        BailoutClosure *br = bailout_;
        bailout_ = NULL;
        return br;
    }
    BailoutClosure *takeBailout() {
        JS_ASSERT(bailout_);
        return maybeTakeBailout();
    }
    BailoutClosure *bailout() const {
        JS_ASSERT(bailout_);
        return bailout_;
    }
    JSCompartment *compartment() const {
        return compartment_;
    }
    bool empty() const {
        
        
        
        return !entryfp_ && !prevpc_;
    }

    static inline size_t offsetOfPrevPc() {
        return offsetof(IonActivation, prevpc_);
    }
    static inline size_t offsetOfEntryFp() {
        return offsetof(IonActivation, entryfp_);
    }
};


void InvalidateAll(FreeOp *fop, JSCompartment *comp);
void FinishInvalidation(FreeOp *fop, UnrootedScript script);

} 
} 

#endif 

