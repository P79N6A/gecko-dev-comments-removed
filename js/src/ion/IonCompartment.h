








































#ifndef jsion_ion_compartment_h__
#define jsion_ion_compartment_h__

#include "IonCode.h"
#include "jsval.h"
#include "jsweakcache.h"
#include "vm/Stack.h"
#include "IonFrames.h"

namespace js {
namespace ion {

class FrameSizeClass;

typedef void (*EnterIonCode)(void *code, int argc, Value *argv, StackFrame *fp,
                             CalleeToken calleeToken, Value *vp);

class IonActivation;

class IonCompartment
{
    typedef WeakCache<const VMFunction *, ReadBarriered<IonCode> > VMWrapperMap;

    friend class IonActivation;

    
    JSC::ExecutableAllocator *execAlloc_;

    
    ReadBarriered<IonCode> enterJIT_;

    
    js::Vector<ReadBarriered<IonCode>, 4, SystemAllocPolicy> bailoutTables_;

    
    ReadBarriered<IonCode> bailoutHandler_;

    
    
    ReadBarriered<IonCode> argumentsRectifier_;

    
    ReadBarriered<IonCode> invalidator_;

    
    ReadBarriered<IonCode> preBarrier_;

    
    VMWrapperMap *functionWrappers_;

  private:
    IonCode *generateEnterJIT(JSContext *cx);
    IonCode *generateReturnError(JSContext *cx);
    IonCode *generateArgumentsRectifier(JSContext *cx);
    IonCode *generateBailoutTable(JSContext *cx, uint32 frameClass);
    IonCode *generateBailoutHandler(JSContext *cx);
    IonCode *generateInvalidator(JSContext *cx);
    IonCode *generatePreBarrier(JSContext *cx);

  public:
    IonCode *generateVMWrapper(JSContext *cx, const VMFunction &f);

  public:
    bool initialize(JSContext *cx);
    IonCompartment();
    ~IonCompartment();

    void mark(JSTracer *trc, JSCompartment *compartment);
    void sweep(JSContext *cx);

    JSC::ExecutableAllocator *execAlloc() {
        return execAlloc_;
    }

    IonCode *getBailoutTable(JSContext *cx, const FrameSizeClass &frameClass);
    IonCode *getGenericBailoutHandler(JSContext *cx) {
        if (!bailoutHandler_) {
            bailoutHandler_ = generateBailoutHandler(cx);
            if (!bailoutHandler_)
                return NULL;
        }
        return bailoutHandler_;
    }

    
    IonCode *getBailoutTable(const FrameSizeClass &frameClass);

    
    IonCode *getArgumentsRectifier(JSContext *cx) {
        if (!argumentsRectifier_) {
            argumentsRectifier_ = generateArgumentsRectifier(cx);
            if (!argumentsRectifier_)
                return NULL;
        }
        return argumentsRectifier_;
    }

    IonCode *getOrCreateInvalidationThunk(JSContext *cx) {
        if (!invalidator_) {
            invalidator_ = generateInvalidator(cx);
            if (!invalidator_)
                return NULL;
        }
        return invalidator_;
    }

    EnterIonCode enterJITInfallible() {
        JS_ASSERT(enterJIT_);
        return enterJIT_.get()->as<EnterIonCode>();
    }

    EnterIonCode enterJIT(JSContext *cx) {
        if (!enterJIT_) {
            enterJIT_ = generateEnterJIT(cx);
            if (!enterJIT_)
                return NULL;
        }
        return enterJIT_.get()->as<EnterIonCode>();
    }

    IonCode *preBarrier(JSContext *cx) {
        if (!preBarrier_) {
            preBarrier_ = generatePreBarrier(cx);
            if (!preBarrier_)
                return NULL;
        }
        return preBarrier_;
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
    uint8 *prevIonTop_;
    JSContext *prevIonJSContext_;
    JSObject *savedEnumerators_;

  public:
    IonActivation(JSContext *cx, StackFrame *fp);
    ~IonActivation();

    StackFrame *entryfp() const {
        return entryfp_;
    }
    IonActivation *prev() const {
        return prev_;
    }
    uint8 *prevIonTop() const {
        return prevIonTop_;
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
    JSObject *savedEnumerators() const {
        return savedEnumerators_;
    }
    void updateSavedEnumerators(JSObject *obj) {
        savedEnumerators_ = obj;
    }
    static inline size_t offsetOfSavedEnumerators() {
        return offsetof(IonActivation, savedEnumerators_);
    }
};

} 
} 

#endif 

