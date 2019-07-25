










#include "vpx_config.h"
#include "vpx_ports/arm.h"
#include "vp8/encoder/onyx_int.h"

extern void (*vp8_yv12_copy_partial_frame_ptr)(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);
extern void vp8_yv12_copy_partial_frame(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);
extern void vp8_yv12_copy_partial_frame_neon(YV12_BUFFER_CONFIG *src_ybc, YV12_BUFFER_CONFIG *dst_ybc);

void vp8_arch_arm_encoder_init(VP8_COMP *cpi)
{
#if CONFIG_RUNTIME_CPU_DETECT
    int flags = cpi->common.rtcd.flags;

#if HAVE_ARMV5TE
    if (flags & HAS_EDSP)
    {
    }
#endif

#if HAVE_ARMV6
    if (flags & HAS_MEDIA)
    {
        cpi->rtcd.fdct.short4x4                  = vp8_short_fdct4x4_armv6;
        cpi->rtcd.fdct.short8x4                  = vp8_short_fdct8x4_armv6;
        cpi->rtcd.fdct.fast4x4                   = vp8_short_fdct4x4_armv6;
        cpi->rtcd.fdct.fast8x4                   = vp8_short_fdct8x4_armv6;
        cpi->rtcd.fdct.walsh_short4x4            = vp8_short_walsh4x4_armv6;

        


        cpi->rtcd.encodemb.subb                  = vp8_subtract_b_armv6;
        cpi->rtcd.encodemb.submby                = vp8_subtract_mby_armv6;
        cpi->rtcd.encodemb.submbuv               = vp8_subtract_mbuv_armv6;

        
        cpi->rtcd.quantize.fastquantb            = vp8_fast_quantize_b_armv6;
    }
#endif

#if HAVE_ARMV7
    if (flags & HAS_NEON)
    {
        cpi->rtcd.fdct.short4x4                  = vp8_short_fdct4x4_neon;
        cpi->rtcd.fdct.short8x4                  = vp8_short_fdct8x4_neon;
        cpi->rtcd.fdct.fast4x4                   = vp8_short_fdct4x4_neon;
        cpi->rtcd.fdct.fast8x4                   = vp8_short_fdct8x4_neon;
        cpi->rtcd.fdct.walsh_short4x4            = vp8_short_walsh4x4_neon;

        


        cpi->rtcd.encodemb.subb                  = vp8_subtract_b_neon;
        cpi->rtcd.encodemb.submby                = vp8_subtract_mby_neon;
        cpi->rtcd.encodemb.submbuv               = vp8_subtract_mbuv_neon;

        

        cpi->rtcd.quantize.fastquantb            = vp8_fast_quantize_b_neon;
        cpi->rtcd.quantize.fastquantb_pair       = vp8_fast_quantize_b_pair_neon;
    }
#endif 
#endif 

#if HAVE_ARMV7
#if CONFIG_RUNTIME_CPU_DETECT
    if (flags & HAS_NEON)
#endif
    {
        vp8_yv12_copy_partial_frame_ptr = vp8_yv12_copy_partial_frame_neon;
    }
#endif
}
