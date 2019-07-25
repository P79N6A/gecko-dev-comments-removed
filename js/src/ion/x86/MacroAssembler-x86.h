








































#ifndef jsion_macro_assembler_x86_h__
#define jsion_macro_assembler_x86_h__

#include "ion/shared/MacroAssembler-x86-shared.h"

namespace js {
namespace ion {

class MacroAssemblerX86 : public MacroAssemblerX86Shared
{
    static const uint32 StackAlignment = 16;

  protected:
    uint32 alignStackForCall(uint32 stackForArgs) {
        
        uint32 displacement = stackForArgs + framePushed_;
        return stackForArgs + ComputeByteAlignment(displacement, StackAlignment);
    }

    uint32 dynamicallyAlignStackForCall(uint32 stackForArgs, const Register &scratch) {
        
        
        
        movl(esp, scratch);
        andl(Imm32(~(StackAlignment - 1)), esp);
        push(scratch);
        uint32 displacement = stackForArgs + STACK_SLOT_SIZE;
        return stackForArgs + ComputeByteAlignment(displacement, StackAlignment);
    }

    void restoreStackFromDynamicAlignment() {
        pop(esp);
    }

  public:
    void reserveStack(uint32 amount) {
        if (amount)
            subl(Imm32(amount), esp);
        framePushed_ += amount;
    }
    void freeStack(uint32 amount) {
        JS_ASSERT(amount <= framePushed_);
        if (amount)
            addl(Imm32(amount), esp);
        framePushed_ -= amount;
    }
    void movePtr(ImmWord imm, const Register &dest) {
        movl(Imm32(imm.value), dest);
    }
    void setStackArg(const Register &reg, uint32 arg) {
        movl(reg, Operand(esp, arg * STACK_SLOT_SIZE));
    }
    void checkCallAlignment() {
#ifdef DEBUG
        Label good;
        movl(esp, eax);
        testl(Imm32(StackAlignment - 1), eax);
        j(Equal, &good);
        breakpoint();
        bind(&good);
#endif
    }
};

typedef MacroAssemblerX86 MacroAssemblerSpecific;

} 
} 

#endif 

