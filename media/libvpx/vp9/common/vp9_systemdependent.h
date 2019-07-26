









#ifndef VP9_COMMON_VP9_SYSTEMDEPENDENT_H_
#define VP9_COMMON_VP9_SYSTEMDEPENDENT_H_

#ifdef _MSC_VER
#include <math.h>
#define snprintf _snprintf
#endif

#include "./vpx_config.h"
#if ARCH_X86 || ARCH_X86_64
void vpx_reset_mmx_state(void);
#define vp9_clear_system_state() vpx_reset_mmx_state()
#else
#define vp9_clear_system_state()
#endif

#if defined(_MSC_VER) && _MSC_VER < 1800

static int round(double x) {
  if (x < 0)
    return (int)ceil(x - 0.5);
  else
    return (int)floor(x + 0.5);
}
#endif

struct VP9Common;
void vp9_machine_specific_config(struct VP9Common *cm);

#endif  
