








#ifndef QUANTIZE_X86_H
#define QUANTIZE_X86_H








#if HAVE_MMX

#endif 


#if HAVE_SSE2
extern prototype_quantize_block(vp8_regular_quantize_b_sse2);
extern prototype_quantize_block(vp8_fast_quantize_b_sse2);

#if !CONFIG_RUNTIME_CPU_DETECT

#undef vp8_quantize_quantb
#define vp8_quantize_quantb vp8_regular_quantize_b_sse2

#undef vp8_quantize_fastquantb
#define vp8_quantize_fastquantb vp8_fast_quantize_b_sse2

#endif 

#endif 


#if HAVE_SSSE3
extern prototype_quantize_block(vp8_fast_quantize_b_ssse3);

#if !CONFIG_RUNTIME_CPU_DETECT

#undef vp8_quantize_fastquantb
#define vp8_quantize_fastquantb vp8_fast_quantize_b_ssse3

#endif 

#endif 


#if HAVE_SSE4_1
extern prototype_quantize_block(vp8_regular_quantize_b_sse4);

#if !CONFIG_RUNTIME_CPU_DETECT

#undef vp8_quantize_quantb
#define vp8_quantize_quantb vp8_regular_quantize_b_sse4

#endif 

#endif 

#endif 
