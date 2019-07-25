








































#ifndef jsion_ion_lowering_x64_h__
#define jsion_ion_lowering_x64_h__

#include "ion/IonLowering.h"

namespace js {
namespace ion {

class LIRGeneratorX64 : public LIRGenerator
{
  public:
    LIRGeneratorX64(MIRGenerator *gen, MIRGraph &graph)
      : LIRGenerator(gen, graph)
    { }

  protected:
    void fillSnapshot(LSnapshot *snapshot);
    bool preparePhi(MPhi *phi);
    bool fillBoxUses(LInstruction *lir, size_t n, MInstruction *mir);

  public:
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitConstant(MConstant *ins);
    bool visitReturn(MReturn *ret);
    bool visitPhi(MPhi *phi);
};

typedef LIRGeneratorX64 LIRBuilder;

} 
} 

#endif 

