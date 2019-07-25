










#ifndef DEQUANTIZE_X86_H
#define DEQUANTIZE_X86_H








#if HAVE_MMX
extern prototype_dequant_block(vp8_dequantize_b_mmx);
extern prototype_dequant_idct_add(vp8_dequant_idct_add_mmx);
extern prototype_dequant_idct_add_y_block(vp8_dequant_idct_add_y_block_mmx);
extern prototype_dequant_idct_add_uv_block(vp8_dequant_idct_add_uv_block_mmx);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef  vp8_dequant_block
#define vp8_dequant_block vp8_dequantize_b_mmx

#undef  vp8_dequant_idct_add
#define vp8_dequant_idct_add vp8_dequant_idct_add_mmx

#undef vp8_dequant_idct_add_y_block
#define vp8_dequant_idct_add_y_block vp8_dequant_idct_add_y_block_mmx

#undef vp8_dequant_idct_add_uv_block
#define vp8_dequant_idct_add_uv_block vp8_dequant_idct_add_uv_block_mmx

#endif
#endif

#if HAVE_SSE2
extern prototype_dequant_idct_add_y_block(vp8_dequant_idct_add_y_block_sse2);
extern prototype_dequant_idct_add_uv_block(vp8_dequant_idct_add_uv_block_sse2);

#if !CONFIG_RUNTIME_CPU_DETECT
#undef vp8_dequant_idct_add_y_block
#define vp8_dequant_idct_add_y_block vp8_dequant_idct_add_y_block_sse2

#undef vp8_dequant_idct_add_uv_block
#define vp8_dequant_idct_add_uv_block vp8_dequant_idct_add_uv_block_sse2

#endif
#endif

#endif
