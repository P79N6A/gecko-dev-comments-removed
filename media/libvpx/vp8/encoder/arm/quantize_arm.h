










#ifndef QUANTIZE_ARM_H
#define QUANTIZE_ARM_H

#if HAVE_ARMV6

extern prototype_quantize_block(vp8_fast_quantize_b_armv6);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp8_quantize_fastquantb
#define vp8_quantize_fastquantb vp8_fast_quantize_b_armv6
#endif

#endif 


#if HAVE_ARMV7

extern prototype_quantize_block(vp8_fast_quantize_b_neon);
extern prototype_quantize_block_pair(vp8_fast_quantize_b_pair_neon);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp8_quantize_fastquantb
#define vp8_quantize_fastquantb vp8_fast_quantize_b_neon

#undef  vp8_quantize_fastquantb_pair
#define vp8_quantize_fastquantb_pair vp8_fast_quantize_b_pair_neon

#undef vp8_quantize_mb
#define vp8_quantize_mb vp8_quantize_mb_neon

#undef vp8_quantize_mbuv
#define vp8_quantize_mbuv vp8_quantize_mbuv_neon

#undef vp8_quantize_mby
#define vp8_quantize_mby vp8_quantize_mby_neon
#endif

#endif 

#endif

