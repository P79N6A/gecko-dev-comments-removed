








































#ifndef jsion_macro_assembler_x64_h__
#define jsion_macro_assembler_x64_h__

#include "ion/shared/MacroAssembler-x86-shared.h"

namespace js {
namespace ion {

class MacroAssemblerX64 : public MacroAssemblerX86Shared
{
    static const uint32 StackAlignment = 16;

#ifdef _WIN64
    static const uint32 ShadowStackSpace = 32;
#else
    static const uint32 ShadowStackSpace = 0;
#endif

  protected:
    uint32 alignStackForCall(uint32 stackForArgs) {
        uint32 total = stackForArgs + ShadowStackSpace;
        uint32 displacement = total + framePushed_;
        return total + ComputeByteAlignment(displacement, StackAlignment);
    }

    uint32 dynamicallyAlignStackForCall(uint32 stackForArgs, const Register &scratch) {
        
        
        
        movq(rsp, scratch);
        andq(Imm32(~(StackAlignment - 1)), rsp);
        push(scratch);
        uint32 total = stackForArgs + ShadowStackSpace;
        uint32 displacement = total + STACK_SLOT_SIZE;
        return total + ComputeByteAlignment(displacement, StackAlignment);
    }

    void restoreStackFromDynamicAlignment() {
        pop(rsp);
    }

  public:
    void reserveStack(uint32 amount) {
        if (amount)
            subq(Imm32(amount), rsp);
        framePushed_ += amount;
    }
    void freeStack(uint32 amount) {
        JS_ASSERT(amount <= framePushed_);
        if (amount)
            addq(Imm32(amount), rsp);
        framePushed_ -= amount;
    }
    void movePtr(ImmWord imm, const Register &dest) {
        movq(imm, dest);
    }
    void setStackArg(const Register &reg, uint32 arg) {
        movq(reg, Operand(rsp, (arg - NumArgRegs) * STACK_SLOT_SIZE + ShadowStackSpace));
    }
    void checkCallAlignment() {
#ifdef DEBUG
        Label good;
        movl(rsp, rax);
        testq(Imm32(StackAlignment - 1), rax);
        j(Equal, &good);
        breakpoint();
        bind(&good);
#endif
    }
};

typedef MacroAssemblerX64 MacroAssemblerSpecific;

} 
} 

#endif 

