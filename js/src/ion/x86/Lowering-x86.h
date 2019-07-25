








































#ifndef jsion_ion_lowering_x86_h__
#define jsion_ion_lowering_x86_h__

#include "ion/IonLowering.h"

namespace js {
namespace ion {

class LIRGeneratorX86 : public LIRGenerator
{
  public:
    LIRGeneratorX86(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGenerator(gen, graph, lirGraph)
    { }

  protected:
    
    
    LUse useType(MInstruction *mir);
    LUse useTypeOrConstant(MInstruction *mir);
    LUse usePayload(MInstruction *mir, LUse::Policy);
    LUse usePayloadInRegister(MInstruction *mir);

    
    
    
    bool fillBoxUses(LInstruction *lir, size_t n, MInstruction *mir);

    void fillSnapshot(LSnapshot *snapshot);
    bool preparePhi(MPhi *phi);

    bool lowerForALU(LMathI *ins, MInstruction *mir, MInstruction *lhs, MInstruction *rhs);

  public:
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitConstant(MConstant *ins);
    bool visitReturn(MReturn *ret);
    bool visitPhi(MPhi *phi);
};

typedef LIRGeneratorX86 LIRBuilder;

} 
} 

#endif 

