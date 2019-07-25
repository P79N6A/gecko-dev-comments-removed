








































#ifndef jsion_ion_lowering_x64_h__
#define jsion_ion_lowering_x64_h__

#include "ion/shared/Lowering-shared.h"

namespace js {
namespace ion {

class LIRGeneratorX64 : public LIRGeneratorShared
{
  public:
    LIRGeneratorX64(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorShared(gen, graph, lirGraph)
    { }

  protected:
    bool assignSnapshot(LInstruction *ins);
    bool preparePhi(MPhi *phi);
    bool fillBoxUses(LInstruction *lir, size_t n, MDefinition *mir);

    bool lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs,
                     MDefinition *rhs);
    bool lowerForFPU(LMathD *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs);

  public:
    bool visitConstant(MConstant *ins);
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitReturn(MReturn *ret);
};

typedef LIRGeneratorX64 LIRGeneratorSpecific;

} 
} 

#endif 

