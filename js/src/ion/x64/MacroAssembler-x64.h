








































#ifndef jsion_macro_assembler_x64_h__
#define jsion_macro_assembler_x64_h__

#include "ion/shared/MacroAssembler-x86-shared.h"

namespace js {
namespace ion {

class MacroAssemblerX64 : public MacroAssemblerX86Shared
{
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
};

typedef MacroAssemblerX64 MacroAssemblerSpecific;

} 
} 

#endif 

