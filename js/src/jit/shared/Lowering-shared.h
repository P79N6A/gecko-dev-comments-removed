





#ifndef jit_shared_Lowering_shared_h
#define jit_shared_Lowering_shared_h




#include "jit/LIR.h"
#include "jit/MIRGenerator.h"

namespace js {
namespace jit {

class MIRGenerator;
class MIRGraph;
class MDefinition;
class MInstruction;
class LOsiPoint;

class LIRGeneratorShared : public MDefinitionVisitor
{
  protected:
    MIRGenerator* gen;
    MIRGraph& graph;
    LIRGraph& lirGraph_;
    LBlock* current;
    MResumePoint* lastResumePoint_;
    LRecoverInfo* cachedRecoverInfo_;
    LOsiPoint* osiPoint_;

  public:
    LIRGeneratorShared(MIRGenerator* gen, MIRGraph& graph, LIRGraph& lirGraph)
      : gen(gen),
        graph(graph),
        lirGraph_(lirGraph),
        lastResumePoint_(nullptr),
        cachedRecoverInfo_(nullptr),
        osiPoint_(nullptr)
    { }

    MIRGenerator* mir() {
        return gen;
    }

  protected:

    static void ReorderCommutative(MDefinition** lhsp, MDefinition** rhsp, MInstruction* ins);
    static bool ShouldReorderCommutative(MDefinition* lhs, MDefinition* rhs, MInstruction* ins);

    
    
    
    
    
    
    
    
    
    inline void emitAtUses(MInstruction* mir);

    
    
    inline void ensureDefined(MDefinition* mir);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    inline LUse use(MDefinition* mir, LUse policy);
    inline LUse use(MDefinition* mir);
    inline LUse useAtStart(MDefinition* mir);
    inline LUse useRegister(MDefinition* mir);
    inline LUse useRegisterAtStart(MDefinition* mir);
    inline LUse useFixed(MDefinition* mir, Register reg);
    inline LUse useFixed(MDefinition* mir, FloatRegister reg);
    inline LUse useFixed(MDefinition* mir, AnyRegister reg);
    inline LUse useFixedAtStart(MDefinition* mir, Register reg);
    inline LAllocation useOrConstant(MDefinition* mir);
    inline LAllocation useOrConstantAtStart(MDefinition* mir);
    
    
    inline LAllocation useAny(MDefinition* mir);
    inline LAllocation useAnyOrConstant(MDefinition* mir);
    
    
    
    inline LAllocation useStorable(MDefinition* mir);
    inline LAllocation useStorableAtStart(MDefinition* mir);
    inline LAllocation useKeepaliveOrConstant(MDefinition* mir);
    inline LAllocation useRegisterOrConstant(MDefinition* mir);
    inline LAllocation useRegisterOrConstantAtStart(MDefinition* mir);
    inline LAllocation useRegisterOrZeroAtStart(MDefinition* mir);
    inline LAllocation useRegisterOrNonDoubleConstant(MDefinition* mir);

    inline LUse useRegisterForTypedLoad(MDefinition* mir, MIRType type);

#ifdef JS_NUNBOX32
    inline LUse useType(MDefinition* mir, LUse::Policy policy);
    inline LUse usePayload(MDefinition* mir, LUse::Policy policy);
    inline LUse usePayloadAtStart(MDefinition* mir, LUse::Policy policy);
    inline LUse usePayloadInRegisterAtStart(MDefinition* mir);

    
    
    
    inline void fillBoxUses(LInstruction* lir, size_t n, MDefinition* mir);
#endif

    
    inline LDefinition temp(LDefinition::Type type = LDefinition::GENERAL,
                            LDefinition::Policy policy = LDefinition::REGISTER);
    inline LDefinition tempFloat32();
    inline LDefinition tempDouble();
    inline LDefinition tempCopy(MDefinition* input, uint32_t reusedInput);

    
    inline LDefinition tempFixed(Register reg);

    template <size_t Ops, size_t Temps>
    inline void defineFixed(LInstructionHelper<1, Ops, Temps>* lir, MDefinition* mir,
                            const LAllocation& output);

    template <size_t Ops, size_t Temps>
    inline void defineBox(LInstructionHelper<BOX_PIECES, Ops, Temps>* lir, MDefinition* mir,
                          LDefinition::Policy policy = LDefinition::REGISTER);

    inline void defineReturn(LInstruction* lir, MDefinition* mir);

    template <size_t X>
    inline void define(details::LInstructionFixedDefsTempsHelper<1, X>* lir, MDefinition* mir,
                       LDefinition::Policy policy = LDefinition::REGISTER);
    template <size_t X>
    inline void define(details::LInstructionFixedDefsTempsHelper<1, X>* lir, MDefinition* mir,
                       const LDefinition& def);

    template <size_t Ops, size_t Temps>
    inline void defineReuseInput(LInstructionHelper<1, Ops, Temps>* lir, MDefinition* mir, uint32_t operand);

    
    inline void useBox(LInstruction* lir, size_t n, MDefinition* mir,
                       LUse::Policy policy = LUse::REGISTER, bool useAtStart = false);

    
    
    inline void redefine(MDefinition* ins, MDefinition* as);

    TempAllocator& alloc() const {
        return graph.alloc();
    }

    uint32_t getVirtualRegister() {
        uint32_t vreg = lirGraph_.getVirtualRegister();

        
        
        
        if (vreg + 1 >= MAX_VIRTUAL_REGISTERS) {
            gen->abort("max virtual registers");
            return 1;
        }
        return vreg;
    }

    template <typename T> void annotate(T* ins);
    template <typename T> void add(T* ins, MInstruction* mir = nullptr);

    void lowerTypedPhiInput(MPhi* phi, uint32_t inputPosition, LBlock* block, size_t lirIndex);
    void defineTypedPhi(MPhi* phi, size_t lirIndex);

    LOsiPoint* popOsiPoint() {
        LOsiPoint* tmp = osiPoint_;
        osiPoint_ = nullptr;
        return tmp;
    }

    LRecoverInfo* getRecoverInfo(MResumePoint* rp);
    LSnapshot* buildSnapshot(LInstruction* ins, MResumePoint* rp, BailoutKind kind);
    bool assignPostSnapshot(MInstruction* mir, LInstruction* ins);

    
    
    
    
    void assignSnapshot(LInstruction* ins, BailoutKind kind);

    
    
    
    void assignSafepoint(LInstruction* ins, MInstruction* mir,
                         BailoutKind kind = Bailout_DuringVMCall);

  public:
    void visitConstant(MConstant* ins);

    
    static bool allowTypedElementHoleCheck() {
        return false;
    }

    
    static bool allowStaticTypedArrayAccesses() {
        return false;
    }

};

} 
} 

#endif 
