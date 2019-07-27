





#ifndef jit_BaselineHelpers_h
#define jit_BaselineHelpers_h

#ifdef JS_ION

#if defined(JS_CODEGEN_X86)
# include "jit/x86/BaselineHelpers-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/BaselineHelpers-x64.h"
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/BaselineHelpers-arm.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/BaselineHelpers-mips.h"
#elif defined(JS_CODEGEN_NONE)
# include "jit/none/BaselineHelpers-none.h"
#else
# error "Unknown architecture!"
#endif

namespace js {
namespace jit {

} 
} 

#endif 

#endif 
