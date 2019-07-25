










#include "vpx_config.h"
#include "vpx_ports/arm.h"
#include "vpx_scale/vpxscale.h"
#include "vpx_scale/yv12extend.h"

void vp8_arch_arm_vpx_scale_init()
{
#if HAVE_ARMV7
#if CONFIG_RUNTIME_CPU_DETECT
    int flags = arm_cpu_caps();
    if (flags & HAS_NEON)
#endif
    {
        vp8_yv12_extend_frame_borders_ptr = vp8_yv12_extend_frame_borders_neon;
        vp8_yv12_copy_y_ptr               = vp8_yv12_copy_y_neon;
        vp8_yv12_copy_frame_ptr           = vp8_yv12_copy_frame_neon;
    }
#endif
}
