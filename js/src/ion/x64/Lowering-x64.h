








































#ifndef jsion_ion_lowering_x64_h__
#define jsion_ion_lowering_x64_h__

#include "ion/Lowering.h"

namespace js {
namespace ion {

class LIRGeneratorX64 : public LIRGenerator
{
  public:
    LIRGeneratorX64(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGenerator(gen, graph, lirGraph)
    { }

  protected:
    void fillSnapshot(LSnapshot *snapshot);
    bool preparePhi(MPhi *phi);
    bool fillBoxUses(LInstruction *lir, size_t n, MDefinition *mir);

    bool lowerForALU(LMathI *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs);
    bool lowerForFPU(LMathD *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs);

  public:
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitConstant(MConstant *ins);
    bool visitReturn(MReturn *ret);
};

typedef LIRGeneratorX64 LIRBuilder;

} 
} 

#endif 

