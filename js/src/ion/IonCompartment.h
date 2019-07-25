








































#ifndef jsion_ion_compartment_h__
#define jsion_ion_compartment_h__

#include "IonCode.h"
#include "jsvalue.h"

namespace js {
namespace ion {

typedef void * CalleeToken;

typedef JSBool (*EnterIonCode)(void *code, int argc, Value *argv, Value *vp,
                               CalleeToken calleeToken);

class IonCompartment {
    JSC::ExecutableAllocator *execAlloc_;

    
    IonCode *enterJIT_;

    IonCode *generateEnterJIT(JSContext *cx);

  public:
    bool initialize(JSContext *cx);
    IonCompartment();
    ~IonCompartment();

    void mark(JSTracer *trc, JSCompartment *compartment);
    void sweep(JSContext *cx);

    JSC::ExecutableAllocator *execAlloc() {
        return execAlloc_;
    }

    EnterIonCode enterJIT(JSContext *cx) {
        if (!enterJIT_) {
            enterJIT_ = generateEnterJIT(cx);
            if (!enterJIT_)
                return NULL;
        }
        return enterJIT_->as<EnterIonCode>();
    }
};


} 
} 

#endif 

