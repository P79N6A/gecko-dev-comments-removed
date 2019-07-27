








#include <arm_neon.h>
#include "SkColorPriv.h"













static SK_ALWAYS_INLINE void Filter_32_opaque_neon(unsigned x, unsigned y,
                                                   SkPMColor a00, SkPMColor a01,
                                                   SkPMColor a10, SkPMColor a11,
                                                   SkPMColor *dst) {
    uint8x8_t vy, vconst16_8, v16_y, vres;
    uint16x4_t vx, vconst16_16, v16_x, tmp;
    uint32x2_t va0, va1;
    uint16x8_t tmp1, tmp2;

    vy = vdup_n_u8(y);                
    vconst16_8 = vmov_n_u8(16);       
    v16_y = vsub_u8(vconst16_8, vy);  

    va0 = vdup_n_u32(a00);            
    va1 = vdup_n_u32(a10);            
    va0 = vset_lane_u32(a01, va0, 1); 
    va1 = vset_lane_u32(a11, va1, 1); 

    tmp1 = vmull_u8(vreinterpret_u8_u32(va0), v16_y); 
    tmp2 = vmull_u8(vreinterpret_u8_u32(va1), vy);    

    vx = vdup_n_u16(x);                
    vconst16_16 = vmov_n_u16(16);      
    v16_x = vsub_u16(vconst16_16, vx); 

    tmp = vmul_u16(vget_high_u16(tmp1), vx);        
    tmp = vmla_u16(tmp, vget_high_u16(tmp2), vx);   
    tmp = vmla_u16(tmp, vget_low_u16(tmp1), v16_x); 
    tmp = vmla_u16(tmp, vget_low_u16(tmp2), v16_x); 

    vres = vshrn_n_u16(vcombine_u16(tmp, vcreate_u16(0)), 8); 
    vst1_lane_u32(dst, vreinterpret_u32_u8(vres), 0);         
}

static SK_ALWAYS_INLINE void Filter_32_alpha_neon(unsigned x, unsigned y,
                                                  SkPMColor a00, SkPMColor a01,
                                                  SkPMColor a10, SkPMColor a11,
                                                  SkPMColor *dst,
                                                  uint16_t scale) {
    uint8x8_t vy, vconst16_8, v16_y, vres;
    uint16x4_t vx, vconst16_16, v16_x, tmp, vscale;
    uint32x2_t va0, va1;
    uint16x8_t tmp1, tmp2;

    vy = vdup_n_u8(y);                
    vconst16_8 = vmov_n_u8(16);       
    v16_y = vsub_u8(vconst16_8, vy);  

    va0 = vdup_n_u32(a00);            
    va1 = vdup_n_u32(a10);            
    va0 = vset_lane_u32(a01, va0, 1); 
    va1 = vset_lane_u32(a11, va1, 1); 

    tmp1 = vmull_u8(vreinterpret_u8_u32(va0), v16_y); 
    tmp2 = vmull_u8(vreinterpret_u8_u32(va1), vy);    

    vx = vdup_n_u16(x);                
    vconst16_16 = vmov_n_u16(16);      
    v16_x = vsub_u16(vconst16_16, vx); 

    tmp = vmul_u16(vget_high_u16(tmp1), vx);        
    tmp = vmla_u16(tmp, vget_high_u16(tmp2), vx);   
    tmp = vmla_u16(tmp, vget_low_u16(tmp1), v16_x); 
    tmp = vmla_u16(tmp, vget_low_u16(tmp2), v16_x); 

    vscale = vdup_n_u16(scale);        
    tmp = vshr_n_u16(tmp, 8);          
    tmp = vmul_u16(tmp, vscale);       

    vres = vshrn_n_u16(vcombine_u16(tmp, vcreate_u16(0)), 8); 
    vst1_lane_u32(dst, vreinterpret_u32_u8(vres), 0);         
}
