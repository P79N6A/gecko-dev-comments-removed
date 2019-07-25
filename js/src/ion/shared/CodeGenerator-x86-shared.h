








































#ifndef jsion_codegen_x86_shared_h__
#define jsion_codegen_x86_shared_h__

#include "ion/shared/CodeGenerator-shared.h"

namespace js {
namespace ion {

class CodeGeneratorX86Shared : public CodeGeneratorShared
{
    friend class MoveResolverX86;

    CodeGeneratorX86Shared *thisFromCtor() {
        return this;
    }

  protected:
    Label *returnLabel_;

    inline Operand ToOperand(const LAllocation &a) {
        if (a.isGeneralReg())
            return Operand(a.toGeneralReg()->reg());
        if (a.isFloatReg())
            return Operand(a.toFloatReg()->reg());
        int32 disp;
        if (a.isStackSlot())
            disp = SlotToStackOffset(a.toStackSlot()->slot());
        else
            disp = ArgToStackOffset(a.toArgument()->index());
        return Operand(StackPointer, disp);
    }
    inline Operand ToOperand(const LAllocation *a) {
        return ToOperand(*a);
    }
    inline Operand ToOperand(const LDefinition *def) {
        return ToOperand(def->output());
    }

    MoveResolver::MoveOperand toMoveOperand(const LAllocation *a);

  public:
    CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph);

    bool generatePrologue();
    bool generateEpilogue();

  public:
    virtual bool visitLabel(LLabel *label);
    virtual bool visitGoto(LGoto *jump);
    virtual bool visitAddI(LAddI *ins);
    virtual bool visitBitOp(LBitOp *ins);
    virtual bool visitMoveGroup(LMoveGroup *group);
    virtual bool visitInteger(LInteger *ins);
};

} 
} 

#endif 

