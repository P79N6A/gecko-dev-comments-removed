










#include "vpx_config.h"
#include "vpx_ports/arm.h"
#include "vp8/decoder/onyxd_int.h"

void vp8_arch_arm_decode_init(VP8D_COMP *pbi)
{
#if CONFIG_RUNTIME_CPU_DETECT
    int flags = pbi->common.rtcd.flags;

#if HAVE_ARMV5TE
    if (flags & HAS_EDSP)
    {
    }
#endif

#if HAVE_ARMV6
    if (flags & HAS_MEDIA)
    {
    }
#endif

#if HAVE_ARMV7
    if (flags & HAS_NEON)
    {
    }
#endif
#endif
}
