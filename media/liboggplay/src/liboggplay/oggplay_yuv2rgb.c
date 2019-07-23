











































#include "oggplay_private.h"
#include "oggplay_yuv2rgb_template.h"

#ifdef __SUNPRO_C
#define DISABLE_CPU_FEATURES



#else

#include "cpu.c"
#endif





typedef void (*yuv_convert_fptr) (const OggPlayYUVChannels *yuv, 
                                  OggPlayRGBChannels *rgb);





static struct OggPlayYUVConverters {
	yuv_convert_fptr yuv420rgba; 
	yuv_convert_fptr yuv420bgra; 
	yuv_convert_fptr yuv420argb; 
	yuv_convert_fptr yuv422rgba; 
	yuv_convert_fptr yuv422bgra; 
	yuv_convert_fptr yuv422argb; 
	yuv_convert_fptr yuv444rgba; 
	yuv_convert_fptr yuv444bgra; 
	yuv_convert_fptr yuv444argb; 
} yuv_conv = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};









#define prec 15 
static const int CoY	= (int)(1.164 * (1 << prec) + 0.5);
static const int CoRV	= (int)(1.596 * (1 << prec) + 0.5);
static const int CoGU	= (int)(0.391 * (1 << prec) + 0.5);
static const int CoGV	= (int)(0.813 * (1 << prec) + 0.5);
static const int CoBU	= (int)(2.018 * (1 << prec) + 0.5);

static int CoefsGU[256] = {0};
static int CoefsGV[256]; 
static int CoefsBU[256]; 
static int CoefsRV[256];
static int CoefsY[256];

#define CLAMP(v)    ((v) > 255 ? 255 : (v) < 0 ? 0 : (v))

#define VANILLA_YUV2RGB_PIXEL(y, ruv, guv, buv)	\
r = (CoefsY[y] + ruv) >> prec;	\
g = (CoefsY[y] + guv) >> prec;	\
b = (CoefsY[y] + buv) >> prec;	\

#define VANILLA_RGBA_OUT(out, r, g, b) \
out[0] = CLAMP(r); \
out[1] = CLAMP(g); \
out[2] = CLAMP(b); \
out[3] = 255;

#define VANILLA_BGRA_OUT(out, r, g, b) \
out[0] = CLAMP(b); \
out[1] = CLAMP(g); \
out[2] = CLAMP(r); \
out[3] = 255;

#define VANILLA_ARGB_OUT(out, r, g, b) \
out[0] = 255;	   \
out[1] = CLAMP(r); \
out[2] = CLAMP(g); \
out[3] = CLAMP(b);

#define VANILLA_ABGR_OUT(out, r, g, b) \
out[0] = 255;	   \
out[1] = CLAMP(b); \
out[2] = CLAMP(g); \
out[3] = CLAMP(r);

#define LOOKUP_COEFFS int ruv = CoefsRV[*pv]; 			\
		      int guv = CoefsGU[*pu] + CoefsGV[*pv]; 	\
		      int buv = CoefsBU[*pu]; 			\
                      int r, g, b;


#define CONVERT(OUTPUT_FUNC) LOOKUP_COEFFS				 \
			     VANILLA_YUV2RGB_PIXEL(py[0], ruv, guv, buv) \
			     OUTPUT_FUNC(dst, r, g, b)  \
			     VANILLA_YUV2RGB_PIXEL(py[1], ruv, guv, buv) \
			     OUTPUT_FUNC((dst+4), r, g, b)

#define CLEANUP

YUV_CONVERT(yuv420_to_rgba_vanilla, CONVERT(VANILLA_RGBA_OUT), VANILLA_RGBA_OUT, 2, 8, 2, 1, 2)
YUV_CONVERT(yuv420_to_bgra_vanilla, CONVERT(VANILLA_BGRA_OUT), VANILLA_BGRA_OUT, 2, 8, 2, 1, 2)
YUV_CONVERT(yuv420_to_abgr_vanilla, CONVERT(VANILLA_ABGR_OUT), VANILLA_ABGR_OUT, 2, 8, 2, 1, 2)
YUV_CONVERT(yuv420_to_argb_vanilla, CONVERT(VANILLA_ARGB_OUT), VANILLA_ARGB_OUT, 2, 8, 2, 1, 2)

YUV_CONVERT(yuv422_to_rgba_vanilla, CONVERT(VANILLA_RGBA_OUT), VANILLA_RGBA_OUT, 2, 8, 2, 1, 1)
YUV_CONVERT(yuv422_to_bgra_vanilla, CONVERT(VANILLA_BGRA_OUT), VANILLA_BGRA_OUT, 2, 8, 2, 1, 1)
YUV_CONVERT(yuv422_to_abgr_vanilla, CONVERT(VANILLA_ABGR_OUT), VANILLA_ABGR_OUT, 2, 8, 2, 1, 1)
YUV_CONVERT(yuv422_to_argb_vanilla, CONVERT(VANILLA_ARGB_OUT), VANILLA_ARGB_OUT, 2, 8, 2, 1, 1)

#undef CONVERT


#define CONVERT(OUTPUT_FUNC) LOOKUP_COEFFS				 \
			     VANILLA_YUV2RGB_PIXEL(py[0], ruv, guv, buv) \
			     OUTPUT_FUNC(dst, r, g, b)  

YUV_CONVERT(yuv444_to_rgba_vanilla, CONVERT(VANILLA_RGBA_OUT), VANILLA_RGBA_OUT, 1, 4, 1, 1, 1)
YUV_CONVERT(yuv444_to_bgra_vanilla, CONVERT(VANILLA_BGRA_OUT), VANILLA_BGRA_OUT, 1, 4, 1, 1, 1)
YUV_CONVERT(yuv444_to_abgr_vanilla, CONVERT(VANILLA_ABGR_OUT), VANILLA_ABGR_OUT, 1, 4, 1, 1, 1)
YUV_CONVERT(yuv444_to_argb_vanilla, CONVERT(VANILLA_ARGB_OUT), VANILLA_ARGB_OUT, 1, 4, 1, 1, 1)

#undef CONVERT
#undef CLEANUP

#ifndef DISABLE_CPU_FEATURES




#if defined(i386) || defined(__x86__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_AMD64)
#if !defined(_M_AMD64)
#define ENABLE_MMX
#endif
#include "x86/oggplay_yuv2rgb_x86.c"
#if defined(_MSC_VER) || defined(ATTRIBUTE_ALIGNED_MAX) && ATTRIBUTE_ALIGNED_MAX >= 16 
#define ENABLE_SSE2
#endif
#elif defined(__ppc__) || defined(__ppc64__)
#define ENABLE_ALTIVEC




#endif
#endif





static void
init_vanilla_coeffs (void)
{
	int i;

	for(i = 0; i < 256; ++i)
	{
		CoefsGU[i] = -CoGU * (i - 128);
		CoefsGV[i] = -CoGV * (i - 128);
		CoefsBU[i] = CoBU * (i - 128);
		CoefsRV[i] = CoRV * (i - 128);
		CoefsY[i]  = CoY * (i - 16) + (prec/2);
	}
}







static void
init_yuv_converters(void)
{
	ogg_uint32_t features = 0;

	if ( yuv_conv.yuv420rgba == NULL )
	{
		init_vanilla_coeffs ();
#ifndef DISABLE_CPU_FEATURES
		features = oc_cpu_flags_get(); 
#endif
#ifdef ENABLE_SSE2	
		if (features & OC_CPU_X86_SSE2) 
		{
			yuv_conv.yuv420rgba = yuv420_to_rgba_sse2;
			yuv_conv.yuv420bgra = yuv420_to_bgra_sse2;
			yuv_conv.yuv420argb = yuv420_to_argb_sse2;
			yuv_conv.yuv422rgba = yuv422_to_rgba_sse2;
			yuv_conv.yuv422bgra = yuv422_to_bgra_sse2;
			yuv_conv.yuv422argb = yuv422_to_argb_sse2;
			yuv_conv.yuv444rgba = yuv444_to_rgba_sse2;
  		yuv_conv.yuv444bgra = yuv444_to_bgra_sse2;
  		yuv_conv.yuv444argb = yuv444_to_argb_sse2;
			return;
		}
#endif 
#ifdef ENABLE_MMX
#ifdef ENABLE_SSE2
		else
#endif
		if (features & OC_CPU_X86_MMXEXT)	
		{
			yuv_conv.yuv420rgba = yuv420_to_rgba_sse;
			yuv_conv.yuv420bgra = yuv420_to_bgra_sse;
			yuv_conv.yuv420argb = yuv420_to_argb_sse;
			yuv_conv.yuv422rgba = yuv422_to_rgba_sse;
			yuv_conv.yuv422bgra = yuv422_to_bgra_sse;
			yuv_conv.yuv422argb = yuv422_to_argb_sse;
			yuv_conv.yuv444rgba = yuv444_to_rgba_sse;
  		yuv_conv.yuv444bgra = yuv444_to_bgra_sse;
  		yuv_conv.yuv444argb = yuv444_to_argb_sse;
			return;
		}
		else if (features & OC_CPU_X86_MMX)
		{   
			yuv_conv.yuv420rgba = yuv420_to_rgba_mmx;
			yuv_conv.yuv420bgra = yuv420_to_bgra_mmx;
			yuv_conv.yuv420argb = yuv420_to_argb_mmx;
			yuv_conv.yuv422rgba = yuv422_to_rgba_mmx;
			yuv_conv.yuv422bgra = yuv422_to_bgra_mmx;
			yuv_conv.yuv422argb = yuv422_to_argb_mmx;
			yuv_conv.yuv444rgba = yuv444_to_rgba_mmx;
  		yuv_conv.yuv444bgra = yuv444_to_bgra_mmx;
  		yuv_conv.yuv444argb = yuv444_to_argb_mmx;
			return;
		}
#elif defined(ENABLE_ALTIVEC)		
		if (features & OC_CPU_PPC_ALTIVEC)
		{
			yuv_conv.yuv420rgba = yuv420_to_abgr_vanilla;
			yuv_conv.yuv420bgra = yuv420_to_argb_vanilla;
			yuv_conv.yuv420argb = yuv420_to_bgra_vanilla;
			yuv_conv.yuv422rgba = yuv422_to_abgr_vanilla;
			yuv_conv.yuv422bgra = yuv422_to_argb_vanilla;
			yuv_conv.yuv422argb = yuv422_to_bgra_vanilla;
			yuv_conv.yuv444rgba = yuv444_to_abgr_vanilla;
  		yuv_conv.yuv444bgra = yuv444_to_argb_vanilla;
  		yuv_conv.yuv444argb = yuv444_to_bgra_vanilla;
			return;
		}
#endif

		



#if WORDS_BIGENDIAN || IS_BIG_ENDIAN 
		yuv_conv.yuv420rgba = yuv420_to_abgr_vanilla;
		yuv_conv.yuv420bgra = yuv420_to_argb_vanilla;
		yuv_conv.yuv420argb = yuv420_to_bgra_vanilla;
		yuv_conv.yuv422rgba = yuv422_to_abgr_vanilla;
		yuv_conv.yuv422bgra = yuv422_to_argb_vanilla;
		yuv_conv.yuv422argb = yuv422_to_bgra_vanilla;
		yuv_conv.yuv444rgba = yuv444_to_abgr_vanilla;
		yuv_conv.yuv444bgra = yuv444_to_argb_vanilla;
		yuv_conv.yuv444argb = yuv444_to_bgra_vanilla;
#else
		yuv_conv.yuv420rgba = yuv420_to_rgba_vanilla;
		yuv_conv.yuv420bgra = yuv420_to_bgra_vanilla;
		yuv_conv.yuv420argb = yuv420_to_argb_vanilla;
		yuv_conv.yuv422rgba = yuv422_to_rgba_vanilla;
		yuv_conv.yuv422bgra = yuv422_to_bgra_vanilla;
		yuv_conv.yuv422argb = yuv422_to_argb_vanilla;
		yuv_conv.yuv444rgba = yuv444_to_rgba_vanilla;
		yuv_conv.yuv444bgra = yuv444_to_bgra_vanilla;
		yuv_conv.yuv444argb = yuv444_to_argb_vanilla;
#endif
	}
}


void
oggplay_yuv2rgba(const OggPlayYUVChannels* yuv, OggPlayRGBChannels* rgb)
{
	if (yuv_conv.yuv420rgba == NULL)
 		init_yuv_converters();

	if (yuv->y_height!=yuv->uv_height)
		yuv_conv.yuv420rgba(yuv, rgb);
	else if (yuv->y_width!=yuv->uv_width)
		yuv_conv.yuv422rgba(yuv,rgb);
	else
		yuv_conv.yuv444rgba(yuv,rgb);
}

void 
oggplay_yuv2bgra(const OggPlayYUVChannels* yuv, OggPlayRGBChannels * rgb)
{
	if (yuv_conv.yuv420bgra == NULL)
 		init_yuv_converters();

	if (yuv->y_height!=yuv->uv_height)
		yuv_conv.yuv420bgra(yuv, rgb);
	else if (yuv->y_width!=yuv->uv_width)
		yuv_conv.yuv422bgra(yuv,rgb);
	else
		yuv_conv.yuv444bgra(yuv,rgb);}

void 
oggplay_yuv2argb(const OggPlayYUVChannels* yuv, OggPlayRGBChannels * rgb)
{
	if (yuv_conv.yuv420argb == NULL)
 		init_yuv_converters();

	if (yuv->y_height!=yuv->uv_height)
		yuv_conv.yuv420argb(yuv, rgb);
	else if (yuv->y_width!=yuv->uv_width)
		yuv_conv.yuv422argb(yuv,rgb);
	else
		yuv_conv.yuv444argb(yuv,rgb);
}

