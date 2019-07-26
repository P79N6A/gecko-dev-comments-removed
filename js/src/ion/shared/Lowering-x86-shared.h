





#ifndef ion_shared_Lowering_x86_shared_h
#define ion_shared_Lowering_x86_shared_h

#include "ion/shared/Lowering-shared.h"

namespace js {
namespace ion {

class LIRGeneratorX86Shared : public LIRGeneratorShared
{
  protected:
    LIRGeneratorX86Shared(MIRGenerator *gen, MIRGraph &graph, LIRGraph &lirGraph)
      : LIRGeneratorShared(gen, graph, lirGraph)
    {}

    LTableSwitch *newLTableSwitch(const LAllocation &in, const LDefinition &inputCopy,
                                  MTableSwitch *ins);
    LTableSwitchV *newLTableSwitchV(MTableSwitch *ins);

    bool visitInterruptCheck(MInterruptCheck *ins);
    bool visitGuardShape(MGuardShape *ins);
    bool visitGuardObjectType(MGuardObjectType *ins);
    bool visitPowHalf(MPowHalf *ins);
    bool lowerForShift(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs,
                       MDefinition *rhs);
    bool lowerForALU(LInstructionHelper<1, 1, 0> *ins, MDefinition *mir, MDefinition *input);
    bool lowerForALU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs,
                     MDefinition *rhs);
    bool lowerForFPU(LInstructionHelper<1, 2, 0> *ins, MDefinition *mir, MDefinition *lhs,
                     MDefinition *rhs);
    bool visitConstant(MConstant *ins);
    bool visitAsmJSNeg(MAsmJSNeg *ins);
    bool visitAsmJSUDiv(MAsmJSUDiv *ins);
    bool visitAsmJSUMod(MAsmJSUMod *ins);
    bool lowerMulI(MMul *mul, MDefinition *lhs, MDefinition *rhs);
    bool lowerDivI(MDiv *div);
    bool lowerModI(MMod *mod);
    bool lowerUDiv(MInstruction *div);
    bool lowerUMod(MInstruction *mod);
    bool lowerUrshD(MUrsh *mir);
    bool lowerConstantDouble(double d, MInstruction *ins);
    bool lowerTruncateDToInt32(MTruncateToInt32 *ins);
};

} 
} 

#endif 
