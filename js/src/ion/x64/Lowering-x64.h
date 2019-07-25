








































#ifndef jsion_ion_lowering_x64_h__
#define jsion_ion_lowering_x64_h__

#include "ion/shared/Lowering-x86-shared.h"

namespace js {
namespace ion {

class LIRGeneratorX64 : public LIRGeneratorX86Shared
{
  public:
    LIRGeneratorX64(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorX86Shared(gen, graph, lirGraph)
    { }

  protected:
    void lowerUntypedPhiInput(MPhi *phi, uint32 inputPosition, LBlock *block, size_t lirIndex);
    bool defineUntypedPhi(MPhi *phi, size_t lirIndex);

    
    bool useBox(LInstruction *lir, size_t n, MDefinition *mir,
                LUse::Policy policy = LUse::REGISTER, bool useAtStart = false);

    bool lowerForShift(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs,
                       MDefinition *rhs);

    bool lowerForALU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir, MDefinition *input);
    bool lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs,
                     MDefinition *rhs);
    bool lowerForFPU(LMathD *ins, MDefinition *mir, MDefinition *lhs, MDefinition *rhs);

    bool lowerConstantDouble(double d, MInstruction *ins);
    bool lowerDivI(MDiv *div);
    bool lowerModI(MMod *mod);
    bool visitGuardShape(MGuardShape *ins);

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

