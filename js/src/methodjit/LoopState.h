





































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
    JSScript *script;
    analyze::ScriptAnalysis *analysis;
    Compiler &cc;
    FrameState &frame;

    
    analyze::LoopAnalysis *lifetime;

    
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

    
    jsbytecode *PC;

    LoopState(JSContext *cx, JSScript *script,
              Compiler *cc, FrameState *frame);
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

    



    bool hoistArrayLengthCheck(const FrameEntry *obj, types::TypeSet *objTypes,
                               unsigned indexPopped);
    FrameEntry *invariantSlots(const FrameEntry *obj);
    FrameEntry *invariantLength(const FrameEntry *obj, types::TypeSet *objTypes);
    FrameEntry *invariantProperty(const FrameEntry *obj, types::TypeSet *objTypes, jsid id);

    
    bool cannotIntegerOverflow();

    



    bool ignoreIntegerOverflow();

  private:
    

    






    enum { UNASSIGNED = uint32(-1) };
    uint32 testLHS;
    uint32 testRHS;
    int32 testConstant;
    bool testLessEqual;

    



    bool testLength;
    bool testLengthKnownObject;

    




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
    void analyzeLoopBody();
    bool definiteArrayAccess(const analyze::SSAValue &obj, const analyze::SSAValue &index);
    void markBitwiseOperand(const analyze::SSAValue &v);

    bool getLoopTestAccess(const analyze::SSAValue &v, uint32 *pslot, int32 *pconstant);

    bool addGrowArray(types::TypeObject *object);
    bool addModifiedProperty(types::TypeObject *object, jsid id);

    bool hasGrowArray(types::TypeObject *object);
    bool hasModifiedProperty(types::TypeObject *object, jsid id);

    uint32 getIncrement(uint32 slot);
    int32 adjustConstantForIncrement(jsbytecode *pc, uint32 slot);

    bool getEntryValue(const analyze::SSAValue &v, uint32 *pslot, int32 *pconstant);
    bool computeInterval(const analyze::SSAValue &v, int32 *pmin, int32 *pmax);
    bool valueFlowsToBitops(const analyze::SSAValue &v);
};

} 
} 

#endif 
