








































#ifndef jsion_macro_assembler_h__
#define jsion_macro_assembler_h__

#ifdef JS_CPU_X86
# include "ion/x86/MacroAssembler-x86.h"
#elif JS_CPU_X64
# include "ion/x64/MacroAssembler-x64.h"
#endif
#include "MoveGroupResolver.h"

namespace js {
namespace ion {

class MacroAssembler : public MacroAssemblerSpecific
{
    MoveGroupResolver moveResolver_;

  public:
    MoveGroupResolver &moveResolver() {
        return moveResolver_;
    }
};

} 
} 

#endif 

