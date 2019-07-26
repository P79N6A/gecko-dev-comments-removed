






#ifndef jsion_ion_instrumentation_h__
#define jsion_ion_instrumentation_h__

namespace js {

class SPSProfiler;

namespace ion {

class MacroAssembler;

typedef SPSInstrumentation<MacroAssembler, Register> BaseInstrumentation;

class IonInstrumentation : public BaseInstrumentation
{
    jsbytecode **trackedPc_;

  public:
    IonInstrumentation(SPSProfiler *profiler, jsbytecode **pc)
      : BaseInstrumentation(profiler),
        trackedPc_(pc)
    {
        JS_ASSERT(pc != NULL);
    }

    void leave(MacroAssembler &masm, Register reg) {
        BaseInstrumentation::leave(*trackedPc_, masm, reg);
    }
};

} 
} 

#endif 
