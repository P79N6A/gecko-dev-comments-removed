








































#ifndef jsion_ion_compartment_h__
#define jsion_ion_compartment_h__

#include "IonCode.h"

namespace js {
namespace ion {

struct IonTrampolines {
    IonCode *ionTrampoline;
};

class IonCompartment {
    JSC::ExecutableAllocator *execAlloc_;
    IonTrampolines           trampolines;

    void Finish() { }

  public:
    bool Initialize() {
        execAlloc_ = new JSC::ExecutableAllocator();
        trampolines.ionTrampoline = GenerateTrampoline(execAlloc_);

        if (!trampolines.ionTrampoline)
            return false;

        return true;
    }

    ~IonCompartment() { Finish(); }

    JSC::ExecutableAllocator *execAlloc() {
        return execAlloc_;
    }

    IonCode *GenerateTrampoline(JSC::ExecutableAllocator *execAlloc);
};


} 
} 

#endif 

