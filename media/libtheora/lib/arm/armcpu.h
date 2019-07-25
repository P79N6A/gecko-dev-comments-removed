















#if !defined(_arm_armcpu_H)
# define _arm_armcpu_H (1)
#include "../internal.h"


#define OC_CPU_ARM_MEDIA    (1<<24)

#define OC_CPU_ARM_EDSP     (1<<7)
#define OC_CPU_ARM_NEON     (1<<12)

ogg_uint32_t oc_cpu_flags_get(void);

#endif
