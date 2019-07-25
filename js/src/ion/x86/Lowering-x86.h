








































#ifndef jsion_ion_lowering_x86_h__
#define jsion_ion_lowering_x86_h__

#include "ion/shared/Lowering-shared.h"

namespace js {
namespace ion {

class LIRGeneratorX86 : public LIRGeneratorShared
{
  public:
    LIRGeneratorX86(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorShared(gen, graph, lirGraph)
    { }

  protected:
    
    
    LUse useType(MDefinition *mir, LUse::Policy policy);
    LUse useTypeOrConstant(MDefinition *mir);
    LUse usePayload(MDefinition *mir, LUse::Policy policy);
    LUse usePayloadInRegister(MDefinition *mir);

    
    
    
    bool fillBoxUses(LInstruction *lir, size_t n, MDefinition *mir);

    bool assignSnapshot(LInstruction *ins);
    bool preparePhi(MPhi *phi);

    bool lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs,
                     MDefinition *rhs);
    bool lowerForFPU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs,
                     MDefinition *rhs);

  public:
    bool visitConstant(MConstant *ins);
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitReturn(MReturn *ret);
    bool lowerPhi(MPhi *phi);
};

typedef LIRGeneratorX86 LIRGeneratorSpecific;

} 
} 

#endif 

