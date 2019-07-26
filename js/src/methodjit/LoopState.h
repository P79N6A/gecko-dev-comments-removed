






#if !defined jsjaeger_loopstate_h__ && defined JS_METHODJIT
#define jsjaeger_loopstate_h__

#include "mozilla/PodOperations.h"

#include "jsanalyze.h"
#include "methodjit/Compiler.h"

namespace js {
namespace mjit {









































struct TemporaryCopy;

enum InvariantArrayKind { DENSE_ARRAY, TYPED_ARRAY };

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
        enum EntryKind {
            



            DENSE_ARRAY_BOUNDS_CHECK,
            TYPED_ARRAY_BOUNDS_CHECK,

            
            NEGATIVE_CHECK,

            
            RANGE_CHECK,

            
            DENSE_ARRAY_SLOTS,
            DENSE_ARRAY_LENGTH,

            
            TYPED_ARRAY_SLOTS,
            TYPED_ARRAY_LENGTH,

            
            INVARIANT_ARGS_BASE,
            INVARIANT_ARGS_LENGTH,

            
            INVARIANT_PROPERTY
        } kind;
        union {
            struct {
                uint32_t arraySlot;
                uint32_t valueSlot1;
                uint32_t valueSlot2;
                int32_t constant;
            } check;
            struct {
                uint32_t arraySlot;
                uint32_t temporary;
            } array;
            struct {
                uint32_t objectSlot;
                uint32_t propertySlot;
                uint32_t temporary;
                jsid id;
            } property;
        } u;
        InvariantEntry() { mozilla::PodZero(this); }
        bool isBoundsCheck() const {
            return kind == DENSE_ARRAY_BOUNDS_CHECK || kind == TYPED_ARRAY_BOUNDS_CHECK;
        }
        bool isCheck() const {
            return isBoundsCheck() || kind == NEGATIVE_CHECK || kind == RANGE_CHECK;
        }
    };
    Vector<InvariantEntry, 4, CompilerAllocPolicy> invariantEntries;

    static inline bool entryRedundant(const InvariantEntry &e0, const InvariantEntry &e1);
    bool checkRedundantEntry(const InvariantEntry &entry);

    bool loopInvariantEntry(uint32_t slot);
    bool addHoistedCheck(InvariantArrayKind arrayKind, uint32_t arraySlot,
                         uint32_t valueSlot1, uint32_t valueSlot2, int32_t constant);
    void addNegativeCheck(uint32_t valueSlot, int32_t constant);
    void addRangeCheck(uint32_t valueSlot1, uint32_t valueSlot2, int32_t constant);
    bool hasTestLinearRelationship(uint32_t slot);

    bool hasInvariants() { return !invariantEntries.empty(); }
    void restoreInvariants(jsbytecode *pc, Assembler &masm,
                           Vector<TemporaryCopy> *temporaryCopies, Vector<Jump> *jumps);

  public:

    
    LoopState *outer;

    
    uint32_t temporariesStart;

    LoopState(JSContext *cx, analyze::CrossScriptSSA *ssa,
              Compiler *cc, FrameState *frame);
    bool init(jsbytecode *head, Jump entry, jsbytecode *entryTarget);

    void setOuterPC(jsbytecode *pc)
    {
        if (uint32_t(pc - outerScript->code) == lifetime->entry && lifetime->entry != lifetime->head)
            reachedEntryPoint = true;
    }

    bool generatingInvariants() { return !skipAnalysis; }

    
    void addInvariantCall(Jump jump, Label label, bool ool, bool entry, unsigned patchIndex, Uses uses);

    uint32_t headOffset() { return lifetime->head; }
    uint32_t getLoopRegs() { return loopRegs.freeMask; }

    Jump entryJump() { return entry; }
    uint32_t entryOffset() { return lifetime->entry; }
    uint32_t backedgeOffset() { return lifetime->backedge; }

    
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

    



    bool hoistArrayLengthCheck(InvariantArrayKind arrayKind,
                               const analyze::CrossSSAValue &obj,
                               const analyze::CrossSSAValue &index);
    FrameEntry *invariantArraySlots(const analyze::CrossSSAValue &obj);

    
    bool hoistArgsLengthCheck(const analyze::CrossSSAValue &index);
    FrameEntry *invariantArguments();

    FrameEntry *invariantLength(const analyze::CrossSSAValue &obj);
    FrameEntry *invariantProperty(const analyze::CrossSSAValue &obj, RawId id);

    
    bool cannotIntegerOverflow(const analyze::CrossSSAValue &pushed);

    



    bool ignoreIntegerOverflow(const analyze::CrossSSAValue &pushed);

  private:
    

    






    enum { UNASSIGNED = UINT32_MAX };
    uint32_t testLHS;
    uint32_t testRHS;
    int32_t testConstant;
    bool testLessEqual;

    




    struct Increment {
        uint32_t slot;
        uint32_t offset;
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

    void analyzeLoopBody(unsigned frame);

    bool definiteArrayAccess(const analyze::SSAValue &obj, const analyze::SSAValue &index);

    bool addGrowArray(types::TypeObject *object);
    bool addModifiedProperty(types::TypeObject *object, jsid id);

    bool hasGrowArray(types::TypeObject *object);
    bool hasModifiedProperty(types::TypeObject *object, jsid id);

    uint32_t getIncrement(uint32_t slot);
    int32_t adjustConstantForIncrement(jsbytecode *pc, uint32_t slot);

    bool getEntryValue(const analyze::CrossSSAValue &v, uint32_t *pslot, int32_t *pconstant);
    bool computeInterval(const analyze::CrossSSAValue &v, int32_t *pmin, int32_t *pmax);
    bool valueFlowsToBitops(const analyze::SSAValue &v);
};

} 
} 

#endif 
