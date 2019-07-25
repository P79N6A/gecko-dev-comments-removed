








































#ifndef jsion_codegen_x86_shared_h__
#define jsion_codegen_x86_shared_h__

#include "ion/shared/CodeGenerator-shared.h"

namespace js {
namespace ion {

class CodeGeneratorX86Shared : public CodeGeneratorShared
{
    AssemblerX86Shared &masm;
    int32 stackDepth_;

  protected:
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

    inline int32 ArgToStackOffset(int32 slot) {
        JS_ASSERT(slot >= 0);
        return -((int32(stackDepth_) + ION_FRAME_OVERHEAD) * STACK_SLOT_SIZE);
    }

    inline int32 SlotToStackOffset(int32 slot) {
        JS_ASSERT(slot >= 0 && slot < stackDepth_);
        return slot * STACK_SLOT_SIZE;
    }

  public:
    CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph, AssemblerX86Shared &masm);

  public:
    virtual bool visitLabel(LLabel *label);
    virtual bool visitMove(LMove *move);
    virtual bool visitGoto(LGoto *jump);
    virtual bool visitAddI(LAddI *ins);
    virtual bool visitBitOp(LBitOp *ins);
};

} 
} 

#endif 

