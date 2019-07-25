










#ifndef SUBPIXEL_ARM_H
#define SUBPIXEL_ARM_H

#if HAVE_ARMV6
extern prototype_subpixel_predict(vp8_sixtap_predict16x16_armv6);
extern prototype_subpixel_predict(vp8_sixtap_predict8x8_armv6);
extern prototype_subpixel_predict(vp8_sixtap_predict8x4_armv6);
extern prototype_subpixel_predict(vp8_sixtap_predict_armv6);
extern prototype_subpixel_predict(vp8_bilinear_predict16x16_armv6);
extern prototype_subpixel_predict(vp8_bilinear_predict8x8_armv6);
extern prototype_subpixel_predict(vp8_bilinear_predict8x4_armv6);
extern prototype_subpixel_predict(vp8_bilinear_predict4x4_armv6);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp8_subpix_sixtap16x16
#define vp8_subpix_sixtap16x16 vp8_sixtap_predict16x16_armv6

#undef  vp8_subpix_sixtap8x8
#define vp8_subpix_sixtap8x8 vp8_sixtap_predict8x8_armv6

#undef  vp8_subpix_sixtap8x4
#define vp8_subpix_sixtap8x4 vp8_sixtap_predict8x4_armv6

#undef  vp8_subpix_sixtap4x4
#define vp8_subpix_sixtap4x4 vp8_sixtap_predict_armv6

#undef  vp8_subpix_bilinear16x16
#define vp8_subpix_bilinear16x16 vp8_bilinear_predict16x16_armv6

#undef  vp8_subpix_bilinear8x8
#define vp8_subpix_bilinear8x8 vp8_bilinear_predict8x8_armv6

#undef  vp8_subpix_bilinear8x4
#define vp8_subpix_bilinear8x4 vp8_bilinear_predict8x4_armv6

#undef  vp8_subpix_bilinear4x4
#define vp8_subpix_bilinear4x4 vp8_bilinear_predict4x4_armv6
#endif
#endif

#if HAVE_ARMV7
extern prototype_subpixel_predict(vp8_sixtap_predict16x16_neon);
extern prototype_subpixel_predict(vp8_sixtap_predict8x8_neon);
extern prototype_subpixel_predict(vp8_sixtap_predict8x4_neon);
extern prototype_subpixel_predict(vp8_sixtap_predict_neon);
extern prototype_subpixel_predict(vp8_bilinear_predict16x16_neon);
extern prototype_subpixel_predict(vp8_bilinear_predict8x8_neon);
extern prototype_subpixel_predict(vp8_bilinear_predict8x4_neon);
extern prototype_subpixel_predict(vp8_bilinear_predict4x4_neon);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp8_subpix_sixtap16x16
#define vp8_subpix_sixtap16x16 vp8_sixtap_predict16x16_neon

#undef  vp8_subpix_sixtap8x8
#define vp8_subpix_sixtap8x8 vp8_sixtap_predict8x8_neon

#undef  vp8_subpix_sixtap8x4
#define vp8_subpix_sixtap8x4 vp8_sixtap_predict8x4_neon

#undef  vp8_subpix_sixtap4x4
#define vp8_subpix_sixtap4x4 vp8_sixtap_predict_neon

#undef  vp8_subpix_bilinear16x16
#define vp8_subpix_bilinear16x16 vp8_bilinear_predict16x16_neon

#undef  vp8_subpix_bilinear8x8
#define vp8_subpix_bilinear8x8 vp8_bilinear_predict8x8_neon

#undef  vp8_subpix_bilinear8x4
#define vp8_subpix_bilinear8x4 vp8_bilinear_predict8x4_neon

#undef  vp8_subpix_bilinear4x4
#define vp8_subpix_bilinear4x4 vp8_bilinear_predict4x4_neon
#endif
#endif

#endif
