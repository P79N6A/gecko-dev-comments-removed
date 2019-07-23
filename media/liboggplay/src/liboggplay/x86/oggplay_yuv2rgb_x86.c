


































#include "oggplay_private.h"
#include "oggplay_yuv2rgb_template.h"
#include "cpu.h"

#if defined(_MSC_VER)
# if defined(_M_AMD64)
  
#include <emmintrin.h>
# endif
#include "yuv2rgb_x86_vs.h" 
#elif defined(__GNUC__)
#include "yuv2rgb_x86.h" 
#endif

typedef union
{
	long long               q[2];
	unsigned long long	uq[2]; 	
	int                     d[4]; 
	unsigned int            ud[4];
	short                   w[8];
	unsigned short          uw[8];
	char                    b[16];
	unsigned char           ub[16];
	float                   s[4];
} ATTR_ALIGN(16) simd_t;

#define UV_128 0x0080008000800080LL 
#define Y_16 0x1010101010101010LL
#define Y_Co 0x253f253f253f253fLL
#define GU_Co 0xf37df37df37df37dLL
#define GV_Co 0xe5fce5fce5fce5fcLL
#define BU_Co 0x4093409340934093LL
#define RV_Co 0x3312331233123312LL
#define Y_MASK 0x00ff00ff00ff00ffLL 
#define ALFA 0xffffffffffffffffLL 




static const simd_t simd_table[9] = {
	{{UV_128, UV_128}},
	{{GU_Co, GU_Co}},
	{{GV_Co, GV_Co}},
	{{BU_Co, BU_Co}},
	{{RV_Co, RV_Co}},
	{{Y_16, Y_16}},
	{{Y_Co, Y_Co}},
	{{Y_MASK, Y_MASK}},
	{{ALFA, ALFA}}
};


#if !(defined(_MSC_VER) && defined(_M_AMD64))





#define YUV_CONVERT_MMX(FUNC, CONVERT, CONV_BY_PIXEL, UV_SHIFT, UV_VERT_SUB) YUV_CONVERT(FUNC, CONVERT, CONV_BY_PIXEL, 8, 32, 8, UV_SHIFT, UV_VERT_SUB)

#define CLEANUP emms()
#define OUT_RGBA_32 OUTPUT_RGBA_32(movq, mm, 8, 16, 24)
#define OUT_ARGB_32 OUTPUT_ARGB_32(movq, mm, 8, 16, 24)
#define OUT_BGRA_32 OUTPUT_BGRA_32(movq, mm, 8, 16, 24)
#define MOVNTQ MMX_MOVNTQ


#define CONVERT(OUTPUT_FUNC) LOAD_YUV_PLANAR_2(movq, mm) \
                             YUV_2_RGB(movq, mm) 	\
                             OUTPUT_FUNC

YUV_CONVERT_MMX(yuv420_to_rgba_mmx, CONVERT(OUT_RGBA_32), VANILLA_RGBA_OUT, 4, 2)
YUV_CONVERT_MMX(yuv420_to_bgra_mmx, CONVERT(OUT_BGRA_32), VANILLA_BGRA_OUT, 4, 2) 
YUV_CONVERT_MMX(yuv420_to_argb_mmx, CONVERT(OUT_ARGB_32), VANILLA_ARGB_OUT, 4, 2) 

YUV_CONVERT_MMX(yuv422_to_rgba_mmx, CONVERT(OUT_RGBA_32), VANILLA_RGBA_OUT, 4, 1)
YUV_CONVERT_MMX(yuv422_to_bgra_mmx, CONVERT(OUT_BGRA_32), VANILLA_BGRA_OUT, 4, 1) 
YUV_CONVERT_MMX(yuv422_to_argb_mmx, CONVERT(OUT_ARGB_32), VANILLA_ARGB_OUT, 4, 1)

YUV_CONVERT_MMX(yuv444_to_rgba_mmx, CONVERT(OUT_RGBA_32), VANILLA_RGBA_OUT, 8, 1)
YUV_CONVERT_MMX(yuv444_to_bgra_mmx, CONVERT(OUT_BGRA_32), VANILLA_BGRA_OUT, 8, 1) 
YUV_CONVERT_MMX(yuv444_to_argb_mmx, CONVERT(OUT_ARGB_32), VANILLA_ARGB_OUT, 8, 1)

#undef MOVNTQ



#define MOVNTQ SSE_MOVNTQ

YUV_CONVERT_MMX(yuv420_to_rgba_sse, CONVERT(OUT_RGBA_32), VANILLA_RGBA_OUT, 4, 2)
YUV_CONVERT_MMX(yuv420_to_bgra_sse, CONVERT(OUT_BGRA_32), VANILLA_BGRA_OUT, 4, 2)
YUV_CONVERT_MMX(yuv420_to_argb_sse, CONVERT(OUT_ARGB_32), VANILLA_ARGB_OUT, 4, 2)

YUV_CONVERT_MMX(yuv422_to_rgba_sse, CONVERT(OUT_RGBA_32), VANILLA_RGBA_OUT, 4, 1)
YUV_CONVERT_MMX(yuv422_to_bgra_sse, CONVERT(OUT_BGRA_32), VANILLA_BGRA_OUT, 4, 1)
YUV_CONVERT_MMX(yuv422_to_argb_sse, CONVERT(OUT_ARGB_32), VANILLA_ARGB_OUT, 4, 1)

YUV_CONVERT_MMX(yuv444_to_rgba_sse, CONVERT(OUT_RGBA_32), VANILLA_RGBA_OUT, 8, 1)
YUV_CONVERT_MMX(yuv444_to_bgra_sse, CONVERT(OUT_BGRA_32), VANILLA_BGRA_OUT, 8, 1)
YUV_CONVERT_MMX(yuv444_to_argb_sse, CONVERT(OUT_ARGB_32), VANILLA_ARGB_OUT, 8, 1)

#undef CONVERT
#undef CLEANUP
#undef OUT_RGBA_32
#undef OUT_ARGB_32
#undef OUT_BGRA_32
#undef MOVNTQ
#endif







#define YUV_CONVERT_SSE2(FUNC, CONVERT, CONV_BY_PIX, UV_SHIFT, UV_VERT_SUB) YUV_CONVERT(FUNC, CONVERT, CONV_BY_PIX, 16, 64, 16, UV_SHIFT, UV_VERT_SUB)

#define OUT_RGBA_32 OUTPUT_RGBA_32(movdqa, xmm, 16, 32, 48)
#define OUT_ARGB_32 OUTPUT_ARGB_32(movdqa, xmm, 16, 32, 48)
#define OUT_BGRA_32 OUTPUT_BGRA_32(movdqa, xmm, 16, 32, 48)
#define MOVNTQ SSE2_MOVNTQ
#define CLEANUP


#if defined(_MSC_VER) && defined(_M_AMD64)
#define CONVERT(OUTPUT_FUNC) __m128i xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7; \
				LOAD_YUV_PLANAR_2(movdqu, xmm) \
				YUV_2_RGB(movdqa, xmm)	\
				OUTPUT_FUNC
#else
#define CONVERT(OUTPUT_FUNC) LOAD_YUV_PLANAR_2(movdqu, xmm) \
				YUV_2_RGB(movdqa, xmm)	\
				OUTPUT_FUNC
#endif
				
YUV_CONVERT_SSE2(yuv420_to_rgba_sse2, CONVERT(OUT_RGBA_32), VANILLA_RGBA_OUT, 8, 2)
YUV_CONVERT_SSE2(yuv420_to_bgra_sse2, CONVERT(OUT_BGRA_32), VANILLA_BGRA_OUT, 8, 2)
YUV_CONVERT_SSE2(yuv420_to_argb_sse2, CONVERT(OUT_ARGB_32), VANILLA_ARGB_OUT, 8, 2)

YUV_CONVERT_SSE2(yuv422_to_rgba_sse2, CONVERT(OUT_RGBA_32), VANILLA_RGBA_OUT, 8, 1)
YUV_CONVERT_SSE2(yuv422_to_bgra_sse2, CONVERT(OUT_BGRA_32), VANILLA_BGRA_OUT, 8, 1)
YUV_CONVERT_SSE2(yuv422_to_argb_sse2, CONVERT(OUT_ARGB_32), VANILLA_ARGB_OUT, 8, 1)

YUV_CONVERT_SSE2(yuv444_to_rgba_sse2, CONVERT(OUT_RGBA_32), VANILLA_RGBA_OUT, 16, 1)
YUV_CONVERT_SSE2(yuv444_to_bgra_sse2, CONVERT(OUT_BGRA_32), VANILLA_BGRA_OUT, 16, 1)
YUV_CONVERT_SSE2(yuv444_to_argb_sse2, CONVERT(OUT_ARGB_32), VANILLA_ARGB_OUT, 16, 1)

#undef CONVERT
#undef OUT_RGBA_32
#undef OUT_ARGB_32
#undef OUT_BGRA_32
#undef MOVNTQ
#undef CLEANUP

