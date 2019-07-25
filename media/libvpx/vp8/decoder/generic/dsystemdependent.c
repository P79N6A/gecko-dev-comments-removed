










#include "vpx_config.h"
#include "vp8/common/dequantize.h"
#include "vp8/decoder/onyxd_int.h"

extern void vp8_arch_x86_decode_init(VP8D_COMP *pbi);
extern void vp8_arch_arm_decode_init(VP8D_COMP *pbi);

void vp8_dmachine_specific_config(VP8D_COMP *pbi)
{
    
#if CONFIG_RUNTIME_CPU_DETECT
    pbi->mb.rtcd                               = &pbi->common.rtcd;
#endif

#if ARCH_X86 || ARCH_X86_64
    vp8_arch_x86_decode_init(pbi);
#endif

#if ARCH_ARM
    vp8_arch_arm_decode_init(pbi);
#endif
}
