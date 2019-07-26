





#if !defined(jsion_baseline_helpers_h__) && defined(JS_ION)
#define jsion_baseline_helpers_h__

#if defined(JS_CPU_X86)
# include "x86/BaselineHelpers-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/BaselineHelpers-x64.h"
#else
# include "arm/BaselineHelpers-arm.h"
#endif

namespace js {
namespace ion {

} 
} 

#endif

