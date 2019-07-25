





































#if !defined jsjaeger_loopstate_h__ && defined JS_METHODJIT
#define jsjaeger_loopstate_h__

#include "jsanalyze.h"
#include "methodjit/BaseCompiler.h"

namespace js {
namespace mjit {


































class LoopState : public MacroAssemblerTypedefs
{
    JSContext *cx;
    JSScript *script;
    Compiler &cc;
    FrameState &frame;
    analyze::Script *analysis;
    analyze::LifetimeScript *liveness;

    
    analyze::LifetimeLoop *lifetime;

    
    RegisterAllocation *alloc;

    




    Jump entry;

    
    Registers loopRegs;

    
    bool skipAnalysis;

    
    struct StubJoin {
        unsigned index;
        bool script;
    };
    Vector<StubJoin,16,CompilerAllocPolicy> loopJoins;

    
    struct StubJoinPatch {
        StubJoin join;
        Address address;
        AnyRegisterID reg;
    };
    Vector<StubJoinPatch,16,CompilerAllocPolicy> loopPatches;

    



    struct HoistedBoundsCheck
    {
        
        uint32 arraySlot;
        uint32 valueSlot;
        int32 constant;
    };
    Vector<HoistedBoundsCheck, 4, CompilerAllocPolicy> hoistedBoundsChecks;

    bool loopInvariantEntry(const FrameEntry *fe);
    void addHoistedCheck(uint32 arraySlot, uint32 valueSlot, int32 constant);

  public:

    
    LoopState *outer;

    
    jsbytecode *PC;

    LoopState(JSContext *cx, JSScript *script,
              Compiler *cc, FrameState *frame,
              analyze::Script *analysis, analyze::LifetimeScript *liveness);
    bool init(jsbytecode *head, Jump entry, jsbytecode *entryTarget);

    uint32 headOffset() { return lifetime->head; }
    uint32 getLoopRegs() { return loopRegs.freeMask; }

    Jump entryJump() { return entry; }
    uint32 entryOffset() { return lifetime->entry; }

    
    bool carriesLoopReg(FrameEntry *fe) { return alloc->hasAnyReg(frame.indexOfFe(fe)); }

    void setLoopReg(AnyRegisterID reg, FrameEntry *fe);

    void clearLoopReg(AnyRegisterID reg)
    {
        



        JS_ASSERT(loopRegs.hasReg(reg) == alloc->loop(reg));
        if (loopRegs.hasReg(reg)) {
            loopRegs.takeReg(reg);
            alloc->setUnassigned(reg);
            JaegerSpew(JSpew_Regalloc, "clearing loop register %s\n", reg.name());
        }
    }

    void addJoin(unsigned index, bool script);
    void flushRegisters(StubCompiler &stubcc);

    bool hoistArrayLengthCheck(const FrameEntry *obj, const FrameEntry *id);
    bool checkHoistedBounds(jsbytecode *PC, Assembler &masm, Vector<Jump> *jumps);
};

} 
} 

#endif 
