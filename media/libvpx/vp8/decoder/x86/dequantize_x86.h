










#ifndef DEQUANTIZE_X86_H
#define DEQUANTIZE_X86_H








#if HAVE_MMX
extern prototype_dequant_block(vp8_dequantize_b_mmx);
extern prototype_dequant_idct(vp8_dequant_idct_mmx);
extern prototype_dequant_idct_dc(vp8_dequant_dc_idct_mmx);


#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp8_dequant_block
#define vp8_dequant_block vp8_dequantize_b_mmx

#undef  vp8_dequant_idct
#define vp8_dequant_idct vp8_dequant_idct_mmx

#undef  vp8_dequant_idct_dc
#define vp8_dequant_idct_dc vp8_dequant_dc_idct_mmx

#endif
#endif

#endif
