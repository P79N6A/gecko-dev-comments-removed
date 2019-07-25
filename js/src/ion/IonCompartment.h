








































#ifndef jsion_ion_compartment_h__
#define jsion_ion_compartment_h__

#include "IonCode.h"

namespace js {
namespace ion {

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
};


} 
} 

#endif 

