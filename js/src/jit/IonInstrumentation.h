





#ifndef jit_IonInstrumentatjit_h
#define jit_IonInstrumentatjit_h

namespace js {

class SPSProfiler;

namespace jit {

class MacroAssembler;

typedef SPSInstrumentation<MacroAssembler, Register> BaseInstrumentation;

class IonInstrumentation : public BaseInstrumentation
{
  public:
    IonInstrumentation(SPSProfiler *profiler, jsbytecode **pc)
      : BaseInstrumentation(profiler)
    {
        MOZ_ASSERT(pc != nullptr);
    }
};

} 
} 

#endif 
