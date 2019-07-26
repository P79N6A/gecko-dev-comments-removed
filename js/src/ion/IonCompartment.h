






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

typedef void (*EnterIonCode)(void *code, int argc, Value *argv, StackFrame *fp,
                             CalleeToken calleeToken, Value *vp);

class IonActivation;
class IonBuilder;

typedef Vector<IonBuilder*, 0, SystemAllocPolicy> OffThreadCompilationVector;

class IonRuntime
{
    friend class IonCompartment;

    
    JSC::ExecutableAllocator *execAlloc_;

    
    IonCode *enterJIT_;

    
    Vector<IonCode*, 4, SystemAllocPolicy> bailoutTables_;

    
    IonCode *bailoutHandler_;

    
    
    IonCode *argumentsRectifier_;

    
    IonCode *invalidator_;

    
    IonCode *valuePreBarrier_;
    IonCode *shapePreBarrier_;

    
    typedef WeakCache<const VMFunction *, IonCode *> VMWrapperMap;
    VMWrapperMap *functionWrappers_;

  private:
    IonCode *generateEnterJIT(JSContext *cx);
    IonCode *generateArgumentsRectifier(JSContext *cx);
    IonCode *generateBailoutTable(JSContext *cx, uint32_t frameClass);
    IonCode *generateBailoutHandler(JSContext *cx);
    IonCode *generateInvalidator(JSContext *cx);
    IonCode *generatePreBarrier(JSContext *cx, MIRType type);
    IonCode *generateVMWrapper(JSContext *cx, const VMFunction &f);

  public:
    IonRuntime();
    ~IonRuntime();
    bool initialize(JSContext *cx);

    static void Mark(JSTracer *trc);
};

class IonCompartment
{
    friend class IonActivation;

    
    IonRuntime *rt;

    
    
    
    
    OffThreadCompilationVector finishedOffThreadCompilations_;

    
    AutoFlushCache *flusher_;

  public:
    IonCode *getVMWrapper(const VMFunction &f);

    OffThreadCompilationVector &finishedOffThreadCompilations() {
        return finishedOffThreadCompilations_;
    }

  public:
    IonCompartment(IonRuntime *rt);

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

    IonCode *getInvalidationThunk() {
        return rt->invalidator_;
    }

    EnterIonCode enterJIT() {
        return rt->enterJIT_->as<EnterIonCode>();
    }

    IonCode *valuePreBarrier() {
        return rt->valuePreBarrier_;
    }
    
    IonCode *shapePreBarrier() {
        return rt->shapePreBarrier_;
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

