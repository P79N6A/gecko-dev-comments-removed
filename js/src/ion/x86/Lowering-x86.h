








































#ifndef jsion_ion_lowering_x86_h__
#define jsion_ion_lowering_x86_h__

#include "ion/IonLowering.h"

namespace js {
namespace ion {

class LIRGeneratorX86 : public LIRGenerator
{
  public:
    LIRGeneratorX86(MIRGenerator *gen, MIRGraph &graph)
      : LIRGenerator(gen, graph)
    { }

  protected:
    
    
    LUse useType(MInstruction *mir);
    LUse usePayloadInRegister(MInstruction *mir);

  public:
    bool visitBox(MBox *box);
    bool visitUnbox(MUnbox *unbox);
    bool visitConstant(MConstant *ins);
    bool visitReturn(MReturn *ret);
};

typedef LIRGeneratorX86 LIRBuilder;

} 
} 

#endif 

