








































#ifndef jsion_lowering_shared_h__
#define jsion_lowering_shared_h__




#include "ion/IonAllocPolicy.h"
#include "ion/IonLIR.h"

namespace js {
namespace ion {

class MBasicBlock;
class MTableSwitch;
class MIRGenerator;
class MIRGraph;
class MDefinition;
class MInstruction;

class LIRGeneratorShared : public MInstructionVisitor
{
  protected:
    MIRGenerator *gen;
    MIRGraph &graph;
    LIRGraph &lirGraph_;
    LBlock *current;
    MResumePoint *lastResumePoint_;
    LSnapshot *postSnapshot_;

  public:
    LIRGeneratorShared(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : gen(gen),
        graph(graph),
        lirGraph_(lirGraph),
        lastResumePoint_(NULL),
        postSnapshot_(NULL)
    { }

    MIRGenerator *mir() {
        return gen;
    }

  protected:
    
    
    
    
    
    
    
    
    
    inline bool emitAtUses(MInstruction *mir);

    
    
    inline bool ensureDefined(MDefinition *mir);

    
    
    inline LUse use(MDefinition *mir, LUse policy);
    inline LUse use(MDefinition *mir);
    inline LUse useCopy(MDefinition *mir);
    inline LUse useRegister(MDefinition *mir);
    inline LUse useFixed(MDefinition *mir, Register reg);
    inline LUse useFixed(MDefinition *mir, FloatRegister reg);
    inline LAllocation useOrConstant(MDefinition *mir);
    inline LAllocation useKeepaliveOrConstant(MDefinition *mir);
    inline LAllocation useRegisterOrConstant(MDefinition *mir);

    
    inline LDefinition temp(LDefinition::Type type);
    inline LDefinition tempFloat();
    inline LDefinition tempFixed(Register reg);

    template <size_t Ops, size_t Temps>
    inline bool defineFixed(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir,
                            const LAllocation &output);

    template <size_t Ops, size_t Temps>
    inline bool defineBox(LInstructionHelper<BOX_PIECES, Ops, Temps> *lir, MDefinition *mir,
                          LDefinition::Policy policy = LDefinition::DEFAULT);

    template <size_t Ops, size_t Temps>
    inline bool defineReturn(LInstructionHelper<BOX_PIECES, Ops, Temps> *lir, MDefinition *mir);

    template <enum VMFunction::ReturnType DefType, size_t Defs, size_t Ops, size_t Temps>
    inline bool defineVMReturn(MDefinition *mir,
                               LVMCallInstructionHelper<DefType, Defs, Ops, Temps> *lir);

    template <size_t Ops, size_t Temps>
    inline bool define(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir,
                        const LDefinition &def);

    template <size_t Ops, size_t Temps>
    inline bool define(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir,
                       LDefinition::Policy policy = LDefinition::DEFAULT);

    template <size_t Ops, size_t Temps>
    inline bool defineReuseInput(LInstructionHelper<1, Ops, Temps> *lir, MDefinition *mir);

    
    
    inline bool redefine(MDefinition *ins, MDefinition *as);

    
    
    inline bool defineAs(LInstruction *outLir, MDefinition *outMir, MDefinition *inMir);

    uint32 getVirtualRegister() {
        return lirGraph_.getVirtualRegister();
    }

    template <typename T> bool annotate(T *ins);
    template <typename T> bool add(T *ins, MInstruction *mir = NULL);

    void lowerTypedPhiInput(MPhi *phi, uint32 inputPosition, LBlock *block, size_t lirIndex);
    bool defineTypedPhi(MPhi *phi, size_t lirIndex);

  public:
    bool visitConstant(MConstant *ins);
};

} 
} 

#endif 


