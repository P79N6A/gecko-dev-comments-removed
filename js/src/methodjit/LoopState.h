





































#if !defined jsjaeger_loopstate_h__ && defined JS_METHODJIT
#define jsjaeger_loopstate_h__

#include "jsanalyze.h"
#include "methodjit/Compiler.h"

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

    



    struct RestoreInvariantCall {
        Jump jump;
        Label label;
        bool ool;

        
        unsigned patchIndex;
        bool patchCall;
    };
    Vector<RestoreInvariantCall> restoreInvariantCalls;

    




    struct InvariantEntry {
        enum {
            



            BOUNDS_CHECK,

            
            NEGATIVE_CHECK,

            INVARIANT_SLOTS,
            INVARIANT_LENGTH
        } kind;
        union {
            struct {
                uint32 arraySlot;
                uint32 valueSlot;
                int32 constant;
            } check;
            struct {
                uint32 arraySlot;
                uint32 temporary;
            } array;
        } u;
        InvariantEntry() { PodZero(this); }
    };
    Vector<InvariantEntry, 4, CompilerAllocPolicy> invariantEntries;

    bool loopInvariantEntry(const FrameEntry *fe);
    bool addHoistedCheck(uint32 arraySlot, uint32 valueSlot, int32 constant);
    void addNegativeCheck(uint32 valueSlot, int32 constant);

    bool hasInvariants() { return !invariantEntries.empty(); }
    void restoreInvariants(Assembler &masm, Vector<Jump> *jumps);

  public:

    
    LoopState *outer;

    
    jsbytecode *PC;

    LoopState(JSContext *cx, JSScript *script,
              Compiler *cc, FrameState *frame,
              analyze::Script *analysis, analyze::LifetimeScript *liveness);
    bool init(jsbytecode *head, Jump entry, jsbytecode *entryTarget);

    bool generatingInvariants() { return !skipAnalysis; }

    
    void addInvariantCall(Jump jump, Label label, bool ool, unsigned patchIndex, bool patchCall);

    uint32 headOffset() { return lifetime->head; }
    uint32 getLoopRegs() { return loopRegs.freeMask; }

    Jump entryJump() { return entry; }
    uint32 entryOffset() { return lifetime->entry; }
    uint32 backedgeOffset() { return lifetime->backedge; }

    
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
    void clearLoopRegisters();

    void flushLoop(StubCompiler &stubcc);

    bool hoistArrayLengthCheck(const FrameEntry *obj, const FrameEntry *id);
    FrameEntry *invariantSlots(const FrameEntry *obj);
    FrameEntry *invariantLength(const FrameEntry *obj);

  private:
    

    
    analyze::StackAnalysis stack;

    






    enum { UNASSIGNED = uint32(-1) };
    uint32 testLHS;
    uint32 testRHS;
    int32 testConstant;
    bool testLessEqual;

    



    bool testLength;

    




    struct Increment {
        uint32 slot;
        uint32 offset;
    };
    Vector<Increment, 4, CompilerAllocPolicy> increments;

    
    bool unknownModset;

    



    Vector<types::TypeObject *, 4, CompilerAllocPolicy> growArrays;

    
    struct ModifiedProperty {
        types::TypeObject *object;
        jsid id;
    };
    Vector<ModifiedProperty, 4, CompilerAllocPolicy> modifiedProperties;

    void analyzeLoopTest();
    void analyzeLoopIncrements();
    void analyzeModset();

    bool loopVariableAccess(jsbytecode *pc);
    bool getLoopTestAccess(jsbytecode *pc, uint32 *slotp, int32 *constantp);

    bool addGrowArray(types::TypeObject *object);
    bool addModifiedProperty(types::TypeObject *object, jsid id);

    bool hasGrowArray(types::TypeObject *object);
    bool hasModifiedProperty(types::TypeObject *object, jsid id);

    inline types::TypeSet *poppedTypes(jsbytecode *pc, unsigned which);
};

} 
} 

#endif 
