








































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

  public:
    bool visitBox(MBox *box);
    bool visitConstant(MConstant *ins);
    bool visitReturn(MReturn *ret);
};

typedef LIRGeneratorX64 LIRBuilder;

} 
} 

#endif 

