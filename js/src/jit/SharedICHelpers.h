





#ifndef jit_SharedICHelpers_h
#define jit_SharedICHelpers_h

#if defined(JS_CODEGEN_X86)
# include "jit/x86/SharedICHelpers-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/SharedICHelpers-x64.h"
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/SharedICHelpers-arm.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/SharedICHelpers-mips.h"
#elif defined(JS_CODEGEN_NONE)
# include "jit/none/SharedICHelpers-none.h"
#else
# error "Unknown architecture!"
#endif

namespace js {
namespace jit {

} 
} 

#endif 
