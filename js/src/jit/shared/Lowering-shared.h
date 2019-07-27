





#ifndef jit_shared_Lowering_shared_h
#define jit_shared_Lowering_shared_h




#include "jit/LIR.h"

namespace js {
namespace jit {

class MBasicBlock;
class MTableSwitch;
class MIRGenerator;
class MIRGraph;
class MDefinition;
class MInstruction;
class LOsiPoint;

class LIRGeneratorShared : public MDefinitionVisitorWithDefaults
{
  protected:
    MIRGenerator *gen;
    MIRGraph &graph;
    LIRGraph &lirGraph_;
    LBlock *current;
    MResumePoint *lastResumePoint_;
    LRecoverInfo *cachedRecoverInfo_;
    LOsiPoint *osiPoint_;

  public:
    LIRGeneratorShared(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : gen(gen),
        graph(graph),
        lirGraph_(lirGraph),
        lastResumePoint_(nullptr),
        cachedRecoverInfo_(nullptr),
        osiPoint_(nullptr)
    { }

    MIRGenerator *mir() {
        return gen;
    }

  protected:
    
    
    
    
    
    
    
    
    
    inline bool emitAtUses(MInstruction *mir);

    
    
    inline bool ensureDefined(MDefinition *mir);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    inline LUse use(MDefinition *mir, LUse policy);
    inline LUse use(MDefinition *mir);
    inline LUse useAtStart(MDefinition *mir);
    inline LUse useRegister(MDefinition *mir);
    inline LUse useRegisterAtStart(MDefinition *mir);
    inline LUse useFixed(MDefinition *mir, Register reg);
    inline LUse useFixed(MDefinition *mir, FloatRegister reg);
    inline LUse useFixed(MDefinition *mir, AnyRegister reg);
    inline LUse useFixedAtStart(MDefinition *mir, Register reg);
    inline LAllocation useOrConstant(MDefinition *mir);
    inline LAllocation useOrConstantAtStart(MDefinition *mir);
    
    
    inline LAllocation useAny(MDefinition *mir);
    inline LAllocation useAnyOrConstant(MDefinition *mir);
    
    
    
    inline LAllocation useStorable(MDefinition *mir);
    inline LAllocation useStorableAtStart(MDefinition *mir);
    inline LAllocation useKeepaliveOrConstant(MDefinition *mir);
    inline LAllocation useRegisterOrConstant(MDefinition *mir);
    inline LAllocation useRegisterOrConstantAtStart(MDefinition *mir);
    inline LAllocation useRegisterOrNonNegativeConstantAtStart(MDefinition *mir);
    inline LAllocation useRegisterOrNonDoubleConstant(MDefinition *mir);

#ifdef JS_NUNBOX32
    inline LUse useType(MDefinition *mir, LUse::Policy policy);
    inline LUse usePayload(MDefinition *mir, LUse::Policy policy);
    inline LUse usePayloadAtStart(MDefinition *mir, LUse::Policy policy);
    inline LUse usePayloadInRegisterAtStart(MDefinition *mir);

    
    
    
    inline bool fillBoxUses(LInstruction *lir, size_t n, MDefinition *mir);
#endif

    
    inline LDefinition temp(LDefinition::Type type = LDefinition::GENERAL,
                            LDefinition::Policy policy = LDefinition::REGISTER);
    inline LDefinition tempFloat32();
    inline LDefinition tempDouble();
    inline LDefinition tempCopy(MDefinition *input, uint32_t reusedInput);

    
    inline LDefinition tempFixed(Register reg);

    template <size_t Ops, size_t Temps>
    inline bool defineFixed(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir,
                            const LAllocation &output);

    template <size_t Ops, size_t Temps>
    inline bool defineBox(LInstructionHelper<BOX_PIECES, Ops, Temps> *lir, MDefinition *mir,
                          LDefinition::Policy policy = LDefinition::REGISTER);

    inline bool defineReturn(LInstruction *lir, MDefinition *mir);

    template <size_t Ops, size_t Temps>
    inline bool define(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir,
                        const LDefinition &def);

    template <size_t Ops, size_t Temps>
    inline bool define(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir,
                       LDefinition::Policy policy = LDefinition::REGISTER);

    template <size_t Ops, size_t Temps>
    inline bool defineReuseInput(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir, uint32_t operand);

    
    
    inline bool redefine(MDefinition *ins, MDefinition *as);

    
    
    inline bool defineAs(LInstruction *outLir, MDefinition *outMir, MDefinition *inMir);

    TempAllocator &alloc() const {
        return graph.alloc();
    }

    uint32_t getVirtualRegister() {
        return lirGraph_.getVirtualRegister();
    }

    template <typename T> void annotate(T *ins);
    template <typename T> bool add(T *ins, MInstruction *mir = nullptr);

    void lowerTypedPhiInput(MPhi *phi, uint32_t inputPosition, LBlock *block, size_t lirIndex);
    bool defineTypedPhi(MPhi *phi, size_t lirIndex);

    LOsiPoint *popOsiPoint() {
        LOsiPoint *tmp = osiPoint_;
        osiPoint_ = nullptr;
        return tmp;
    }

    LRecoverInfo *getRecoverInfo(MResumePoint *rp);
    LSnapshot *buildSnapshot(LInstruction *ins, MResumePoint *rp, BailoutKind kind);
    bool assignPostSnapshot(MInstruction *mir, LInstruction *ins);

    
    
    
    
    bool assignSnapshot(LInstruction *ins, BailoutKind kind);

    
    
    
    bool assignSafepoint(LInstruction *ins, MInstruction *mir,
                         BailoutKind kind = Bailout_DuringVMCall);

  public:
    bool visitConstant(MConstant *ins);

    
    static bool allowTypedElementHoleCheck() {
        return false;
    }

    
    static bool allowStaticTypedArrayAccesses() {
        return false;
    }

    
    static bool allowInlineForkJoinGetSlice() {
        return false;
    }

};

} 
} 

#endif 
