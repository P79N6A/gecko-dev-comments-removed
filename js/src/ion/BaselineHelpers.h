





#ifndef ion_BaselineHelpers_h
#define ion_BaselineHelpers_h

#ifdef JS_ION

#if defined(JS_CPU_X86)
# include "ion/x86/BaselineHelpers-x86.h"
#elif defined(JS_CPU_X64)
# include "ion/x64/BaselineHelpers-x64.h"
#elif defined(JS_CPU_ARM)
# include "ion/arm/BaselineHelpers-arm.h"
#else
# error "Unknown architecture!"
#endif

namespace js {
namespace ion {

} 
} 

#endif 

#endif 
