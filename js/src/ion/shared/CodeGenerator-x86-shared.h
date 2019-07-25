








































#ifndef jsion_codegen_x86_shared_h__
#define jsion_codegen_x86_shared_h__

#include "ion/shared/CodeGenerator-shared.h"
#if defined(JS_CPU_X86)
# include "ion/x86/Assembler-x86.h"
#elif defined(JS_CPU_X64)
# include "ion/x64/Assembler-x64.h"
#endif

namespace js {
namespace ion {

class CodeGeneratorX86Shared : public CodeGeneratorShared
{
    friend class MoveResolverX86;

    CodeGeneratorX86Shared *thisFromCtor() {
        return this;
    }

  protected:
    Assembler masm;
    
    
    
    
    int32 frameDepth_;

    
    
    
    int32 framePushed_;

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
        return framePushed_ + frameDepth_ + ION_FRAME_PREFIX_SIZE + slot;
    }

    inline int32 SlotToStackOffset(int32 slot) {
        JS_ASSERT(slot >= 0 && slot < int32(graph.stackHeight()));
        return framePushed_ + slot * STACK_SLOT_SIZE;
    }

  public:
    CodeGeneratorX86Shared(MIRGenerator *gen, LIRGraph &graph);

    bool generatePrologue();

  public:
    virtual bool visitLabel(LLabel *label);
    virtual bool visitGoto(LGoto *jump);
    virtual bool visitAddI(LAddI *ins);
    virtual bool visitBitOp(LBitOp *ins);
};

} 
} 

#endif 

