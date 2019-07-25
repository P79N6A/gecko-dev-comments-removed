





































#if !defined jsjaeger_loopstate_h__ && defined JS_METHODJIT
#define jsjaeger_loopstate_h__

#include "jsanalyze.h"
#include "methodjit/Compiler.h"

namespace js {
namespace mjit {









































struct TemporaryCopy;

class LoopState : public MacroAssemblerTypedefs
{
    JSContext *cx;
    analyze::CrossScriptSSA *ssa;
    JSScript *outerScript;
    analyze::ScriptAnalysis *outerAnalysis;

    Compiler &cc;
    FrameState &frame;

    
    analyze::LoopAnalysis *lifetime;

    
    RegisterAllocation *alloc;

    



    bool reachedEntryPoint;

    




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
        bool entry;
        unsigned patchIndex;  

        
        Vector<TemporaryCopy> *temporaryCopies;
    };
    Vector<RestoreInvariantCall> restoreInvariantCalls;

    




    struct InvariantEntry {
        enum {
            



            BOUNDS_CHECK,

            
            NEGATIVE_CHECK,

            
            RANGE_CHECK,

            INVARIANT_SLOTS,
            INVARIANT_LENGTH,

            INVARIANT_PROPERTY
        } kind;
        union {
            struct {
                uint32 arraySlot;
                uint32 valueSlot1;
                uint32 valueSlot2;
                int32 constant;
            } check;
            struct {
                uint32 arraySlot;
                uint32 temporary;
            } array;
            struct {
                uint32 objectSlot;
                uint32 propertySlot;
                uint32 temporary;
                jsid id;
            } property;
        } u;
        InvariantEntry() { PodZero(this); }
        bool isCheck() const {
            return kind == BOUNDS_CHECK || kind == NEGATIVE_CHECK || kind == RANGE_CHECK;
        }
    };
    Vector<InvariantEntry, 4, CompilerAllocPolicy> invariantEntries;

    static inline bool entryRedundant(const InvariantEntry &e0, const InvariantEntry &e1);
    bool checkRedundantEntry(const InvariantEntry &entry);

    bool loopInvariantEntry(uint32 slot);
    bool addHoistedCheck(uint32 arraySlot,
                         uint32 valueSlot1, uint32 valueSlot2, int32 constant);
    void addNegativeCheck(uint32 valueSlot, int32 constant);
    void addRangeCheck(uint32 valueSlot1, uint32 valueSlot2, int32 constant);
    bool hasTestLinearRelationship(uint32 slot);

    bool hasInvariants() { return !invariantEntries.empty(); }
    void restoreInvariants(jsbytecode *pc, Assembler &masm,
                           Vector<TemporaryCopy> *temporaryCopies, Vector<Jump> *jumps);

  public:

    
    LoopState *outer;

    
    uint32 temporariesStart;

    LoopState(JSContext *cx, analyze::CrossScriptSSA *ssa,
              Compiler *cc, FrameState *frame);
    bool init(jsbytecode *head, Jump entry, jsbytecode *entryTarget);

    void setOuterPC(jsbytecode *pc)
    {
        if (uint32(pc - outerScript->code) == lifetime->entry && lifetime->entry != lifetime->head)
            reachedEntryPoint = true;
    }

    bool generatingInvariants() { return !skipAnalysis; }

    
    void addInvariantCall(Jump jump, Label label, bool ool, bool entry, unsigned patchIndex);

    uint32 headOffset() { return lifetime->head; }
    uint32 getLoopRegs() { return loopRegs.freeMask; }

    Jump entryJump() { return entry; }
    uint32 entryOffset() { return lifetime->entry; }
    uint32 backedgeOffset() { return lifetime->backedge; }

    
    bool carriesLoopReg(FrameEntry *fe) { return alloc->hasAnyReg(frame.entrySlot(fe)); }

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

    



    bool hoistArrayLengthCheck(const analyze::CrossSSAValue &obj,
                               const analyze::CrossSSAValue &index);
    FrameEntry *invariantSlots(const analyze::CrossSSAValue &obj);
    FrameEntry *invariantLength(const analyze::CrossSSAValue &obj);
    FrameEntry *invariantProperty(const analyze::CrossSSAValue &obj, jsid id);

    
    bool cannotIntegerOverflow(const analyze::CrossSSAValue &pushed);

    



    bool ignoreIntegerOverflow(const analyze::CrossSSAValue &pushed);

  private:
    

    






    enum { UNASSIGNED = uint32(-1) };
    uint32 testLHS;
    uint32 testRHS;
    int32 testConstant;
    bool testLessEqual;

    




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

    




    bool constrainedLoop;

    void analyzeLoopTest();
    void analyzeLoopIncrements();
    void analyzeLoopBody(unsigned frame);

    bool definiteArrayAccess(const analyze::SSAValue &obj, const analyze::SSAValue &index);
    bool getLoopTestAccess(const analyze::SSAValue &v, uint32 *pslot, int32 *pconstant);

    bool addGrowArray(types::TypeObject *object);
    bool addModifiedProperty(types::TypeObject *object, jsid id);

    bool hasGrowArray(types::TypeObject *object);
    bool hasModifiedProperty(types::TypeObject *object, jsid id);

    uint32 getIncrement(uint32 slot);
    int32 adjustConstantForIncrement(jsbytecode *pc, uint32 slot);

    bool getEntryValue(const analyze::CrossSSAValue &v, uint32 *pslot, int32 *pconstant);
    bool computeInterval(const analyze::CrossSSAValue &v, int32 *pmin, int32 *pmax);
    bool valueFlowsToBitops(const analyze::SSAValue &v);
};

} 
} 

#endif 
