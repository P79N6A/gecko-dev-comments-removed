






#ifndef jsion_ion_lowering_x86_shared_h__
#define jsion_ion_lowering_x86_shared_h__

#include "ion/shared/Lowering-shared.h"

namespace js {
namespace ion {

class LIRGeneratorX86Shared : public LIRGeneratorShared
{
  protected:
    LIRGeneratorX86Shared(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorShared(gen, graph, lirGraph)
    {}

    bool visitTableSwitch(MTableSwitch *tableswitch);
    bool visitRecompileCheck(MRecompileCheck *ins);
    bool visitInterruptCheck(MInterruptCheck *ins);
    bool visitGuardShape(MGuardShape *ins);
    bool lowerMulI(MMul *mul, MDefinition *lhs, MDefinition *rhs);
    bool lowerModI(MMod *mod);
};

} 
} 

#endif 
