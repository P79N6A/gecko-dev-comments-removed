






























#ifndef COMMON_MAC_ARCH_UTILITIES_H__
#define COMMON_MAC_ARCH_UTILITIES_H__

#include <mach-o/arch.h>

namespace google_breakpad {



const NXArchInfo* BreakpadGetArchInfoFromName(const char* arch_name);
const NXArchInfo* BreakpadGetArchInfoFromCpuType(cpu_type_t cpu_type,
                                                 cpu_subtype_t cpu_subtype);

}  

#endif  
