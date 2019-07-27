

















#ifndef AVUTIL_CPU_INTERNAL_H
#define AVUTIL_CPU_INTERNAL_H

#include "cpu.h"

#define CPUEXT_SUFFIX(flags, suffix, cpuext)                            \
    (HAVE_ ## cpuext ## suffix && ((flags) & AV_CPU_FLAG_ ## cpuext))

#define CPUEXT(flags, cpuext) CPUEXT_SUFFIX(flags, , cpuext)

int ff_get_cpu_flags_aarch64(void);
int ff_get_cpu_flags_arm(void);
int ff_get_cpu_flags_ppc(void);
int ff_get_cpu_flags_x86(void);

#endif 
