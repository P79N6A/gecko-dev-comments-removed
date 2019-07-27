





#ifndef jit_BaselineRegisters_h
#define jit_BaselineRegisters_h

#ifdef JS_ION

#if defined(JS_CODEGEN_X86)
# include "jit/x86/BaselineRegisters-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/BaselineRegisters-x64.h"
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/BaselineRegisters-arm.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/BaselineRegisters-mips.h"
#elif defined(JS_CODEGEN_NONE)
# include "jit/none/BaselineRegisters-none.h"
#else
# error "Unknown architecture!"
#endif

namespace js {
namespace jit {

} 
} 

#endif 

#endif 
