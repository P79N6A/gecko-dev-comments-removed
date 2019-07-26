






#if !defined(jsion_baseline_registers_h__) && defined(JS_ION)
#define jsion_baseline_registers_h__

#if defined(JS_CPU_X86)
# include "x86/BaselineRegisters-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/BaselineRegisters-x64.h"
#else
#error "CPU Not Supported"
#endif

namespace js {
namespace ion {

} 
} 

#endif

