





#ifndef jit_BaselineRegisters_h
#define jit_BaselineRegisters_h

#ifdef JS_ION

#if defined(JS_CPU_X86)
# include "jit/x86/BaselineRegisters-x86.h"
#elif defined(JS_CPU_X64)
# include "jit/x64/BaselineRegisters-x64.h"
#else
# include "jit/arm/BaselineRegisters-arm.h"
#endif

namespace js {
namespace jit {

} 
} 

#endif 

#endif 
