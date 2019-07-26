





#ifndef jit_IonInstrumentatjit_h
#define jit_IonInstrumentatjit_h

namespace js {

class SPSProfiler;

namespace jit {

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
        JS_ASSERT(pc != nullptr);
    }

    void leave(MacroAssembler &masm, Register reg, bool inlinedFunction = false) {
        BaseInstrumentation::leave(*trackedPc_, masm, reg, inlinedFunction);
    }

    bool enterInlineFrame() {
        return BaseInstrumentation::enterInlineFrame(*trackedPc_);
    }
};

} 
} 

#endif 
