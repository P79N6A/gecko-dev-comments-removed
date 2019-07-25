








































#ifndef jsion_ion_compartment_h__
#define jsion_ion_compartment_h__

#include "IonCode.h"
#include "jsval.h"
#include "vm/Stack.h"
#include "IonFrames.h"

namespace js {
namespace ion {

class FrameSizeClass;

typedef JSBool (*EnterIonCode)(void *code, int argc, Value *argv, Value *vp,
                               CalleeToken calleeToken);

class IonActivation;

class IonCompartment
{
    friend class IonActivation;

    JSC::ExecutableAllocator *execAlloc_;

    
    IonActivation *active_;

    
    IonCode *enterJIT_;

    
    js::Vector<IonCode *, 4, SystemAllocPolicy> bailoutTables_;

    
    IonCode *bailoutHandler_;

    
    IonCode *returnError_;

    
    
    IonCode *argumentsRectifier_;

    
    VMWrapperMap *functionWrappers_;

    
    IonCFrame *topCFrame_;

  private:
    IonCode *generateEnterJIT(JSContext *cx);
    IonCode *generateReturnError(JSContext *cx);
    IonCode *generateArgumentsRectifier(JSContext *cx);
    IonCode *generateBailoutTable(JSContext *cx, uint32 frameClass);
    IonCode *generateBailoutHandler(JSContext *cx);

  public:
    IonCode *generateCWrapper(JSContext *cx, const VMFunction &f);

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

    EnterIonCode enterJIT(JSContext *cx) {
        if (!enterJIT_) {
            enterJIT_ = generateEnterJIT(cx);
            if (!enterJIT_)
                return NULL;
        }
        if (!returnError_) {
            returnError_ = generateReturnError(cx);
            if (!returnError_)
                return NULL;
        }
        return enterJIT_->as<EnterIonCode>();
    }
    IonCode *returnError() const {
        JS_ASSERT(returnError_);
        return returnError_;
    }

    IonActivation *activation() const {
        return active_;
    }

    IonCFrame *topCFrame() const {
        return topCFrame_;
    }
};

class BailoutClosure;

class IonActivation
{
    JSContext *cx_;
    IonActivation *prev_;
    StackFrame *entryfp_;
    FrameRegs &oldFrameRegs_;
    BailoutClosure *bailout_;

  public:
    IonActivation(JSContext *cx, StackFrame *fp);
    ~IonActivation();
    StackFrame *entryfp() const {
        return entryfp_;
    }
    IonActivation *prev() const {
        return prev_;
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
    FrameRegs &oldFrameRegs() {
        return oldFrameRegs_;
    }
};

} 
} 

#endif 

