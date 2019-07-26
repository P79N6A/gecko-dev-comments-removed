





#ifndef jsion_baseline_helpers_h__
#define jsion_baseline_helpers_h__

#ifdef JS_ION

#if defined(JS_CPU_X86)
# include "x86/BaselineHelpers-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/BaselineHelpers-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/BaselineHelpers-arm.h"
#else
# error "Unknown architecture!"
#endif

namespace js {
namespace ion {

} 
} 

#endif 

#endif 
