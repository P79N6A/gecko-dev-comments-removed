








































#ifndef jsion_macro_assembler_x86_shared_h__
#define jsion_macro_assembler_x86_shared_h__

#ifdef JS_CPU_X86
# include "ion/x86/Assembler-x86.h"
#elif JS_CPU_X64
# include "ion/x64/Assembler-x64.h"
#endif

namespace js {
namespace ion {

class MacroAssemblerX86Shared : public Assembler
{
  protected:
    
    
    
    
    
    uint32 framePushed_;

  public:
    MacroAssemblerX86Shared()
      : framePushed_(0)
    { }

    void Push(const Register &reg) {
        push(reg);
        framePushed_ += STACK_SLOT_SIZE;
    }

    uint32 framePushed() const {
        return framePushed_;
    }
};

} 
} 

#endif 

