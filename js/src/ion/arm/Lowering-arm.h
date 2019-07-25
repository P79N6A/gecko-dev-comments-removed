








































#ifndef jsion_ion_lowering_arm_h__
#define jsion_ion_lowering_arm_h__

#include "ion/shared/Lowering-shared.h"

namespace js {
namespace ion {

class LIRGeneratorARM : public LIRGeneratorShared
{
  public:
    LIRGeneratorARM(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorShared(gen, graph, lirGraph)
    { }

  protected:
    
    
    LUse useType(MDefinition *mir, LUse::Policy policy);
    LUse useTypeOrConstant(MDefinition *mir);
    LUse usePayload(MDefinition *mir, LUse::Policy policy);
    LUse usePayloadInRegister(MDefinition *mir);

    
    
    bool useBox(LInstruction *lir, size_t n, MDefinition *mir,
                LUse::Policy policy = LUse::REGISTER);

    
    
    
    bool fillBoxUses(LInstruction *lir, size_t n, MDefinition *mir);

    bool assignSnapshot(LInstruction *ins);
    void lowerUntypedPhiInput(MPhi *phi, uint32 inputPosition, LBlock *block, size_t lirIndex);
    bool defineUntypedPhi(MPhi *phi, size_t lirIndex);

    bool lowerForALU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir,
                     MDefinition *input);
    bool lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir,
                     MDefinition *lhs, MDefinition *rhs);

    bool lowerForFPU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir,
                     MDefinition *src);
    bool lowerForFPU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir,
                     MDefinition *lhs, MDefinition *rhs);

    bool lowerConstantDouble(double d, MInstruction *ins);

  public:
    bool visitConstant(MConstant *ins);
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitReturn(MReturn *ret);
    bool lowerPhi(MPhi *phi);
};

typedef LIRGeneratorARM LIRGeneratorSpecific;

} 
} 

#endif 

