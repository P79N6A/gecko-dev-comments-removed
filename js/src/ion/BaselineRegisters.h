





#ifndef ion_BaselineRegisters_h
#define ion_BaselineRegisters_h

#ifdef JS_ION

#if defined(JS_CPU_X86)
# include "x86/BaselineRegisters-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/BaselineRegisters-x64.h"
#else
# include "arm/BaselineRegisters-arm.h"
#endif

namespace js {
namespace ion {

} 
} 

#endif 

#endif 
