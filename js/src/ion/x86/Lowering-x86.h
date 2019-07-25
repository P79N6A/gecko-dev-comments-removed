








































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
    
    
    LUse useType(MDefinition *mir, LUse::Policy policy);
    LUse useTypeOrConstant(MDefinition *mir);
    LUse usePayload(MDefinition *mir, LUse::Policy policy);
    LUse usePayloadInRegister(MDefinition *mir);

    
    
    
    bool fillBoxUses(LInstruction *lir, size_t n, MDefinition *mir);

    void fillSnapshot(LSnapshot *snapshot);
    bool preparePhi(MPhi *phi);

    bool lowerForALU(LMathI *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs);

  public:
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitConstant(MConstant *ins);
    bool visitReturn(MReturn *ret);
    bool lowerPhi(MPhi *phi);
};

typedef LIRGeneratorX86 LIRBuilder;

} 
} 

#endif 

