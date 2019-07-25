








































#ifndef jsion_macro_assembler_x86_h__
#define jsion_macro_assembler_x86_h__

#include "ion/shared/MacroAssembler-x86-shared.h"

namespace js {
namespace ion {

class MacroAssemblerX86 : public MacroAssemblerX86Shared
{
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
};

typedef MacroAssemblerX86 MacroAssemblerSpecific;

} 
} 

#endif 

