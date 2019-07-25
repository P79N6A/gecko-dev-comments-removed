










#ifndef VARIANCE_ARM_H
#define VARIANCE_ARM_H

#if HAVE_ARMV6

extern prototype_sad(vp8_sad16x16_armv6);
extern prototype_variance(vp8_variance16x16_armv6);
extern prototype_variance(vp8_variance8x8_armv6);
extern prototype_subpixvariance(vp8_sub_pixel_variance16x16_armv6);
extern prototype_subpixvariance(vp8_sub_pixel_variance8x8_armv6);
extern prototype_variance(vp8_variance_halfpixvar16x16_h_armv6);
extern prototype_variance(vp8_variance_halfpixvar16x16_v_armv6);
extern prototype_variance(vp8_variance_halfpixvar16x16_hv_armv6);
extern prototype_variance(vp8_mse16x16_armv6);

#if !CONFIG_RUNTIME_CPU_DETECT

#undef  vp8_variance_sad16x16
#define vp8_variance_sad16x16 vp8_sad16x16_armv6

#undef  vp8_variance_subpixvar16x16
#define vp8_variance_subpixvar16x16 vp8_sub_pixel_variance16x16_armv6

#undef  vp8_variance_subpixvar8x8
#define vp8_variance_subpixvar8x8 vp8_sub_pixel_variance8x8_armv6

#undef  vp8_variance_var16x16
#define vp8_variance_var16x16 vp8_variance16x16_armv6

#undef  vp8_variance_mse16x16
#define vp8_variance_mse16x16 vp8_mse16x16_armv6

#undef  vp8_variance_var8x8
#define vp8_variance_var8x8 vp8_variance8x8_armv6

#undef  vp8_variance_halfpixvar16x16_h
#define vp8_variance_halfpixvar16x16_h vp8_variance_halfpixvar16x16_h_armv6

#undef  vp8_variance_halfpixvar16x16_v
#define vp8_variance_halfpixvar16x16_v vp8_variance_halfpixvar16x16_v_armv6

#undef  vp8_variance_halfpixvar16x16_hv
#define vp8_variance_halfpixvar16x16_hv vp8_variance_halfpixvar16x16_hv_armv6

#endif 

#endif 


#if HAVE_ARMV7
extern prototype_sad(vp8_sad4x4_neon);
extern prototype_sad(vp8_sad8x8_neon);
extern prototype_sad(vp8_sad8x16_neon);
extern prototype_sad(vp8_sad16x8_neon);
extern prototype_sad(vp8_sad16x16_neon);


extern prototype_variance(vp8_variance8x8_neon);
extern prototype_variance(vp8_variance8x16_neon);
extern prototype_variance(vp8_variance16x8_neon);
extern prototype_variance(vp8_variance16x16_neon);


extern prototype_subpixvariance(vp8_sub_pixel_variance8x8_neon);


extern prototype_subpixvariance(vp8_sub_pixel_variance16x16_neon);
extern prototype_subpixvariance(vp8_sub_pixel_variance16x16_neon_func);
extern prototype_variance(vp8_variance_halfpixvar16x16_h_neon);
extern prototype_variance(vp8_variance_halfpixvar16x16_v_neon);
extern prototype_variance(vp8_variance_halfpixvar16x16_hv_neon);


extern prototype_variance(vp8_mse16x16_neon);
extern prototype_get16x16prederror(vp8_get4x4sse_cs_neon);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp8_variance_sad4x4
#define vp8_variance_sad4x4 vp8_sad4x4_neon

#undef  vp8_variance_sad8x8
#define vp8_variance_sad8x8 vp8_sad8x8_neon

#undef  vp8_variance_sad8x16
#define vp8_variance_sad8x16 vp8_sad8x16_neon

#undef  vp8_variance_sad16x8
#define vp8_variance_sad16x8 vp8_sad16x8_neon

#undef  vp8_variance_sad16x16
#define vp8_variance_sad16x16 vp8_sad16x16_neon




#undef  vp8_variance_var8x8
#define vp8_variance_var8x8 vp8_variance8x8_neon

#undef  vp8_variance_var8x16
#define vp8_variance_var8x16 vp8_variance8x16_neon

#undef  vp8_variance_var16x8
#define vp8_variance_var16x8 vp8_variance16x8_neon

#undef  vp8_variance_var16x16
#define vp8_variance_var16x16 vp8_variance16x16_neon




#undef  vp8_variance_subpixvar8x8
#define vp8_variance_subpixvar8x8 vp8_sub_pixel_variance8x8_neon







#undef  vp8_variance_subpixvar16x16
#define vp8_variance_subpixvar16x16 vp8_sub_pixel_variance16x16_neon

#undef  vp8_variance_halfpixvar16x16_h
#define vp8_variance_halfpixvar16x16_h vp8_variance_halfpixvar16x16_h_neon

#undef  vp8_variance_halfpixvar16x16_v
#define vp8_variance_halfpixvar16x16_v vp8_variance_halfpixvar16x16_v_neon

#undef  vp8_variance_halfpixvar16x16_hv
#define vp8_variance_halfpixvar16x16_hv vp8_variance_halfpixvar16x16_hv_neon




#undef  vp8_variance_mse16x16
#define vp8_variance_mse16x16 vp8_mse16x16_neon

#undef  vp8_variance_get4x4sse_cs
#define vp8_variance_get4x4sse_cs vp8_get4x4sse_cs_neon
#endif

#endif

#endif
