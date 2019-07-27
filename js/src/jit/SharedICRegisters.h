





#ifndef jit_SharedICRegisters_h
#define jit_SharedICRegisters_h

#if defined(JS_CODEGEN_X86)
# include "jit/x86/SharedICRegisters-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/SharedICRegisters-x64.h"
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/SharedICRegisters-arm.h"
#elif defined(JS_CODEGEN_ARM64)
# include "jit/arm64/SharedICRegisters-arm64.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/SharedICRegisters-mips.h"
#elif defined(JS_CODEGEN_NONE)
# include "jit/none/SharedICRegisters-none.h"
#else
# error "Unknown architecture!"
#endif

namespace js {
namespace jit {

} 
} 

#endif 
