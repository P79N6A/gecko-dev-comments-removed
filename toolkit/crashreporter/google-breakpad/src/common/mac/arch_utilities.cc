




























#include "common/mac/arch_utilities.h"

#include <mach-o/arch.h>
#include <stdio.h>
#include <string.h>

#ifndef CPU_TYPE_ARM
#define CPU_TYPE_ARM (static_cast<cpu_type_t>(12))
#endif  

#ifndef CPU_SUBTYPE_ARM_V7
#define CPU_SUBTYPE_ARM_V7 (static_cast<cpu_subtype_t>(9))
#endif  

#ifndef CPU_SUBTYPE_ARM_V7S
#define CPU_SUBTYPE_ARM_V7S (static_cast<cpu_subtype_t>(11))
#endif  

namespace {

const NXArchInfo* ArchInfo_armv7s() {
  NXArchInfo* armv7s = new NXArchInfo;
  *armv7s = *NXGetArchInfoFromCpuType(CPU_TYPE_ARM,
                                      CPU_SUBTYPE_ARM_V7);
  armv7s->name = "armv7s";
  armv7s->cpusubtype = CPU_SUBTYPE_ARM_V7S;
  armv7s->description = "arm v7s";
  return armv7s;
}

}  

namespace google_breakpad {

const NXArchInfo* BreakpadGetArchInfoFromName(const char* arch_name) {
  
  if (!strcmp("armv7s", arch_name))
    return BreakpadGetArchInfoFromCpuType(CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7S);
  return NXGetArchInfoFromName(arch_name);
}

const NXArchInfo* BreakpadGetArchInfoFromCpuType(cpu_type_t cpu_type,
                                                 cpu_subtype_t cpu_subtype) {
  
  if (cpu_type == CPU_TYPE_ARM && cpu_subtype == CPU_SUBTYPE_ARM_V7S) {
    static const NXArchInfo* armv7s = ArchInfo_armv7s();
    return armv7s;
  }
  return NXGetArchInfoFromCpuType(cpu_type, cpu_subtype);
}

}  
