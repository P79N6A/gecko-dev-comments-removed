







#ifdef ANDROID
    #include <machine/cpu-features.h>
#endif

#include "SkBlitRow.h"
#include "SkBlitMask.h"
#include "SkColorPriv.h"
#include "SkDither.h"

#if defined(__ARM_HAVE_NEON)
#include <arm_neon.h>
#endif

#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)
static void S32A_D565_Opaque_neon(uint16_t* SK_RESTRICT dst,
                                  const SkPMColor* SK_RESTRICT src, int count,
                                  U8CPU alpha, int , int ) {
    SkASSERT(255 == alpha);

    if (count >= 8) {
        uint16_t* SK_RESTRICT keep_dst;
        
        asm volatile (
                      "ands       ip, %[count], #7            \n\t"
                      "vmov.u8    d31, #1<<7                  \n\t"
                      "vld1.16    {q12}, [%[dst]]             \n\t"
                      "vld4.8     {d0-d3}, [%[src]]           \n\t"
                      "moveq      ip, #8                      \n\t"
                      "mov        %[keep_dst], %[dst]         \n\t"
                      
                      "add        %[src], %[src], ip, LSL#2   \n\t"
                      "add        %[dst], %[dst], ip, LSL#1   \n\t"
                      "subs       %[count], %[count], ip      \n\t"
                      "b          9f                          \n\t"
                      
                      "2:                                         \n\t"
                      
                      "vld1.16    {q12}, [%[dst]]!            \n\t"
                      "vld4.8     {d0-d3}, [%[src]]!          \n\t"
                      "vst1.16    {q10}, [%[keep_dst]]        \n\t"
                      "sub        %[keep_dst], %[dst], #8*2   \n\t"
                      "subs       %[count], %[count], #8      \n\t"
                      "9:                                         \n\t"
                      "pld        [%[dst],#32]                \n\t"
                      
                      "vmovn.u16  d4, q12                     \n\t"
                      "vshr.u16   q11, q12, #5                \n\t"
                      "vshr.u16   q10, q12, #6+5              \n\t"
                      "vmovn.u16  d5, q11                     \n\t"
                      "vmovn.u16  d6, q10                     \n\t"
                      "vshl.u8    d4, d4, #3                  \n\t"
                      "vshl.u8    d5, d5, #2                  \n\t"
                      "vshl.u8    d6, d6, #3                  \n\t"
                      
                      "vmovl.u8   q14, d31                    \n\t"
                      "vmovl.u8   q13, d31                    \n\t"
                      "vmovl.u8   q12, d31                    \n\t"
                      
                      
                      "vmvn.8     d30, d3                     \n\t"
                      "vmlal.u8   q14, d30, d6                \n\t"
                      "vmlal.u8   q13, d30, d5                \n\t"
                      "vmlal.u8   q12, d30, d4                \n\t"
                      "vshr.u16   q8, q14, #5                 \n\t"
                      "vshr.u16   q9, q13, #6                 \n\t"
                      "vaddhn.u16 d6, q14, q8                 \n\t"
                      "vshr.u16   q8, q12, #5                 \n\t"
                      "vaddhn.u16 d5, q13, q9                 \n\t"
                      "vqadd.u8   d6, d6, d0                  \n\t"  
                      "vaddhn.u16 d4, q12, q8                 \n\t"
                      
                      
                      
                      "vqadd.u8   d5, d5, d1                  \n\t"
                      "vqadd.u8   d4, d4, d2                  \n\t"
                      
                      
                      "vshll.u8   q10, d6, #8                 \n\t"
                      "vshll.u8   q3, d5, #8                  \n\t"
                      "vshll.u8   q2, d4, #8                  \n\t"
                      "vsri.u16   q10, q3, #5                 \n\t"
                      "vsri.u16   q10, q2, #11                \n\t"
                      
                      "bne        2b                          \n\t"
                      
                      "1:                                         \n\t"
                      "vst1.16      {q10}, [%[keep_dst]]      \n\t"
                      : [count] "+r" (count)
                      : [dst] "r" (dst), [keep_dst] "r" (keep_dst), [src] "r" (src) 
                      : "ip", "cc", "memory", "d0","d1","d2","d3","d4","d5","d6","d7",
                      "d16","d17","d18","d19","d20","d21","d22","d23","d24","d25","d26","d27","d28","d29",
                      "d30","d31"
                      );
    }
    else 
    {   
        uint16_t* SK_RESTRICT keep_dst;
        
        asm volatile (
                      "vmov.u8    d31, #1<<7                  \n\t"
                      "mov        %[keep_dst], %[dst]         \n\t"
                      
                      "tst        %[count], #4                \n\t"
                      "beq        14f                         \n\t"
                      "vld1.16    {d25}, [%[dst]]!            \n\t"
                      "vld1.32    {q1}, [%[src]]!             \n\t"
                      
                      "14:                                        \n\t"
                      "tst        %[count], #2                \n\t"
                      "beq        12f                         \n\t"
                      "vld1.32    {d24[1]}, [%[dst]]!         \n\t"
                      "vld1.32    {d1}, [%[src]]!             \n\t"
                      
                      "12:                                        \n\t"
                      "tst        %[count], #1                \n\t"
                      "beq        11f                         \n\t"
                      "vld1.16    {d24[1]}, [%[dst]]!         \n\t"
                      "vld1.32    {d0[1]}, [%[src]]!          \n\t"
                      
                      "11:                                        \n\t"
                      
                      "vuzpq.u16  q0, q1                      \n\t"
                      "vuzp.u8    d0, d1                      \n\t"
                      "vuzp.u8    d2, d3                      \n\t"
                      
                      "vmovn.u16  d4, q12                     \n\t"
                      "vshr.u16   q11, q12, #5                \n\t"
                      "vshr.u16   q10, q12, #6+5              \n\t"
                      "vmovn.u16  d5, q11                     \n\t"
                      "vmovn.u16  d6, q10                     \n\t"
                      "vshl.u8    d4, d4, #3                  \n\t"
                      "vshl.u8    d5, d5, #2                  \n\t"
                      "vshl.u8    d6, d6, #3                  \n\t"
                      
                      "vmovl.u8   q14, d31                    \n\t"
                      "vmovl.u8   q13, d31                    \n\t"
                      "vmovl.u8   q12, d31                    \n\t"
                      
                      
                      "vmvn.8     d30, d3                     \n\t"
                      "vmlal.u8   q14, d30, d6                \n\t"
                      "vmlal.u8   q13, d30, d5                \n\t"
                      "vmlal.u8   q12, d30, d4                \n\t"
                      "vshr.u16   q8, q14, #5                 \n\t"
                      "vshr.u16   q9, q13, #6                 \n\t"
                      "vaddhn.u16 d6, q14, q8                 \n\t"
                      "vshr.u16   q8, q12, #5                 \n\t"
                      "vaddhn.u16 d5, q13, q9                 \n\t"
                      "vqadd.u8   d6, d6, d0                  \n\t"  
                      "vaddhn.u16 d4, q12, q8                 \n\t"
                      
                      
                      
                      "vqadd.u8   d5, d5, d1                  \n\t"
                      "vqadd.u8   d4, d4, d2                  \n\t"
                      
                      
                      "vshll.u8   q10, d6, #8                 \n\t"
                      "vshll.u8   q3, d5, #8                  \n\t"
                      "vshll.u8   q2, d4, #8                  \n\t"
                      "vsri.u16   q10, q3, #5                 \n\t"
                      "vsri.u16   q10, q2, #11                \n\t"
                      
                      
                      "tst        %[count], #4                \n\t"
                      "beq        24f                         \n\t"
                      "vst1.16    {d21}, [%[keep_dst]]!       \n\t"
                      
                      "24:                                        \n\t"
                      "tst        %[count], #2                \n\t"
                      "beq        22f                         \n\t"
                      "vst1.32    {d20[1]}, [%[keep_dst]]!    \n\t"
                      
                      "22:                                        \n\t"
                      "tst        %[count], #1                \n\t"
                      "beq        21f                         \n\t"
                      "vst1.16    {d20[1]}, [%[keep_dst]]!    \n\t"
                      
                      "21:                                        \n\t"
                      : [count] "+r" (count)
                      : [dst] "r" (dst), [keep_dst] "r" (keep_dst), [src] "r" (src)
                      : "ip", "cc", "memory", "d0","d1","d2","d3","d4","d5","d6","d7",
                      "d16","d17","d18","d19","d20","d21","d22","d23","d24","d25","d26","d27","d28","d29",
                      "d30","d31"
                      );
    }
}

static void S32A_D565_Blend_neon(uint16_t* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src, int count,
                                 U8CPU alpha, int , int ) {

    U8CPU alpha_for_asm = alpha;

    asm volatile (
    











                  
#if 1
		
                  "add        %[alpha], %[alpha], #1         \n\t"   
#else
                  "add        %[alpha], %[alpha], %[alpha], lsr #7    \n\t"   
#endif
                  "vmov.u16   q3, #255                        \n\t"   
                  "movs       r4, %[count], lsr #3            \n\t"   
                  "vmov.u16   d2[0], %[alpha]                 \n\t"   
                  "beq        2f                              \n\t"   
                  "vmov.u16   q15, #0x1f                      \n\t"   
                  
                  "1:                                             \n\t"
                  "vld1.u16   {d0, d1}, [%[dst]]              \n\t"   
                  "subs       r4, r4, #1                      \n\t"   
                  "vld4.u8    {d24, d25, d26, d27}, [%[src]]! \n\t"   
                  
                  
                  "vshl.u16   q9, q0, #5                      \n\t"   
                  "vand       q10, q0, q15                    \n\t"   
                  "vshr.u16   q8, q0, #11                     \n\t"   
                  "vshr.u16   q9, q9, #10                     \n\t"   
                  
                  
                  "vshr.u8    d24, d24, #3                    \n\t"   
                  "vshr.u8    d25, d25, #2                    \n\t"   
                  "vshr.u8    d26, d26, #3                    \n\t"   
                  
                  "vmovl.u8   q11, d24                        \n\t"   
                  "vmovl.u8   q12, d25                        \n\t"   
                  "vmovl.u8   q14, d27                        \n\t"   
                  "vmovl.u8   q13, d26                        \n\t"   
                  
                  
                  "vmul.u16   q2, q14, d2[0]                  \n\t"   
                  "vmul.u16   q11, q11, d2[0]                 \n\t"   
                  "vmul.u16   q12, q12, d2[0]                 \n\t"   
                  "vmul.u16   q13, q13, d2[0]                 \n\t"   
                  
                  "vshr.u16   q2, q2, #8                      \n\t"   
                  "vsub.u16   q2, q3, q2                      \n\t"   
                  
                  
                  "vmla.u16   q11, q8, q2                     \n\t"   
                  "vmla.u16   q12, q9, q2                     \n\t"   
                  "vmla.u16   q13, q10, q2                    \n\t"   

#if 1
	
	
	
                  "vrshr.u16   q11, q11, #8                    \n\t"   
                  "vrshr.u16   q12, q12, #8                    \n\t"   
                  "vrshr.u16   q13, q13, #8                    \n\t"   
#else
	
                  "vshr.u16   q11, q11, #8                    \n\t"   
                  "vshr.u16   q12, q12, #8                    \n\t"   
                  "vshr.u16   q13, q13, #8                    \n\t"   
#endif
                  
                  "vsli.u16   q13, q12, #5                    \n\t"   
                  "vsli.u16   q13, q11, #11                   \n\t"   
                  "vst1.16    {d26, d27}, [%[dst]]!           \n\t"   
                  
                  "bne        1b                              \n\t"   
                  "2:                                             \n\t"   
                  
                  : [src] "+r" (src), [dst] "+r" (dst), [count] "+r" (count), [alpha] "+r" (alpha_for_asm)
                  :
                  : "cc", "memory", "r4", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"
                  );

    count &= 7;
    if (count > 0) {
        do {
            SkPMColor sc = *src++;
            if (sc) {
                uint16_t dc = *dst;
                unsigned dst_scale = 255 - SkMulDiv255Round(SkGetPackedA32(sc), alpha);
                unsigned dr = SkMulS16(SkPacked32ToR16(sc), alpha) + SkMulS16(SkGetPackedR16(dc), dst_scale);
                unsigned dg = SkMulS16(SkPacked32ToG16(sc), alpha) + SkMulS16(SkGetPackedG16(dc), dst_scale);
                unsigned db = SkMulS16(SkPacked32ToB16(sc), alpha) + SkMulS16(SkGetPackedB16(dc), dst_scale);
                *dst = SkPackRGB16(SkDiv255Round(dr), SkDiv255Round(dg), SkDiv255Round(db));
            }
            dst += 1;
        } while (--count != 0);
    }
}






static const uint8_t gDitherMatrix_Neon[48] = {
    0, 4, 1, 5, 0, 4, 1, 5, 0, 4, 1, 5,
    6, 2, 7, 3, 6, 2, 7, 3, 6, 2, 7, 3,
    1, 5, 0, 4, 1, 5, 0, 4, 1, 5, 0, 4,
    7, 3, 6, 2, 7, 3, 6, 2, 7, 3, 6, 2,
    
};

static void S32_D565_Blend_Dither_neon(uint16_t *dst, const SkPMColor *src,
                                       int count, U8CPU alpha, int x, int y)
{
    
    const uint8_t *dstart = &gDitherMatrix_Neon[(y&3)*12 + (x&3)];
    
    
    int scale = SkAlpha255To256(alpha);
    
    asm volatile (
                  "vld1.8         {d31}, [%[dstart]]              \n\t"   
                  "vshr.u8        d30, d31, #1                    \n\t"   
                  "vdup.16        d6, %[scale]                    \n\t"   
                  "vmov.i8        d29, #0x3f                      \n\t"   
                  "vmov.i8        d28, #0x1f                      \n\t"   
                  "1:                                                 \n\t"
                  "vld4.8         {d0, d1, d2, d3}, [%[src]]!     \n\t"   
                  "vshr.u8        d22, d0, #5                     \n\t"   
                  "vshr.u8        d23, d1, #6                     \n\t"   
                  "vshr.u8        d24, d2, #5                     \n\t"   
                  "vaddl.u8       q8, d0, d31                     \n\t"   
                  "vaddl.u8       q9, d1, d30                     \n\t"   
                  "vaddl.u8       q10, d2, d31                    \n\t"   
                  "vsubw.u8       q8, q8, d22                     \n\t"   
                  "vsubw.u8       q9, q9, d23                     \n\t"   
                  "vsubw.u8       q10, q10, d24                   \n\t"   
                  "vshrn.i16      d22, q8, #3                     \n\t"   
                  "vshrn.i16      d23, q9, #2                     \n\t"   
                  "vshrn.i16      d24, q10, #3                    \n\t"   
                  
                  "vld1.16        {d0, d1}, [%[dst]]              \n\t"   
                  "vshrn.i16      d17, q0, #5                     \n\t"   
                  "vmovn.i16      d18, q0                         \n\t"   
                  "vshr.u16       q0, q0, #11                     \n\t"   
                  "vand           d17, d17, d29                   \n\t"   
                  "vand           d18, d18, d28                   \n\t"   
                  "vmovn.i16      d16, q0                         \n\t"   
                  
                  
                  
                  "vsubl.s8       q0, d22, d16                    \n\t"   
                  "vsubl.s8       q1, d23, d17                    \n\t"   
                  "vsubl.s8       q2, d24, d18                    \n\t"   
                  
                  "vmul.i16       q0, q0, d6[0]                   \n\t"   
                  "vmul.i16       q1, q1, d6[0]                   \n\t"   
                  "vmul.i16       q2, q2, d6[0]                   \n\t"   
                  "subs           %[count], %[count], #8          \n\t"   
                  "vshrn.i16      d0, q0, #8                      \n\t"   
                  "vshrn.i16      d2, q1, #8                      \n\t"   
                  "vshrn.i16      d4, q2, #8                      \n\t"   
                  
                  "vaddl.s8       q0, d0, d16                     \n\t"   
                  "vaddl.s8       q1, d2, d17                     \n\t"   
                  "vaddl.s8       q2, d4, d18                     \n\t"   
                  
                  "vsli.i16       q2, q1, #5                      \n\t"   
                  "vsli.i16       q2, q0, #11                     \n\t"   
                  "vst1.16        {d4, d5}, [%[dst]]!             \n\t"   
                  "bgt            1b                              \n\t"   
                  : [src] "+r" (src), [dst] "+r" (dst), [count] "+r" (count)
                  : [dstart] "r" (dstart), [scale] "r" (scale)
                  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
                  );
    
    DITHER_565_SCAN(y);
    
    while((count & 7) > 0)
    {
        SkPMColor c = *src++;
        
        int dither = DITHER_VALUE(x);
        int sr = SkGetPackedR32(c);
        int sg = SkGetPackedG32(c);
        int sb = SkGetPackedB32(c);
        sr = SkDITHER_R32To565(sr, dither);
        sg = SkDITHER_G32To565(sg, dither);
        sb = SkDITHER_B32To565(sb, dither);
        
        uint16_t d = *dst;
        *dst++ = SkPackRGB16(SkAlphaBlend(sr, SkGetPackedR16(d), scale),
                             SkAlphaBlend(sg, SkGetPackedG16(d), scale),
                             SkAlphaBlend(sb, SkGetPackedB16(d), scale));
        DITHER_INC_X(x);
        count--;
    }
}

#define S32A_D565_Opaque_PROC       S32A_D565_Opaque_neon
#define S32A_D565_Blend_PROC        S32A_D565_Blend_neon
#define S32_D565_Blend_Dither_PROC  S32_D565_Blend_Dither_neon
#else
#define S32A_D565_Opaque_PROC       NULL
#define S32A_D565_Blend_PROC        NULL
#define S32_D565_Blend_Dither_PROC  NULL
#endif




#define S32_D565_Opaque_PROC    S32A_D565_Opaque_PROC
#define S32_D565_Blend_PROC     S32A_D565_Blend_PROC



#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)

static void S32A_Opaque_BlitRow32_neon(SkPMColor* SK_RESTRICT dst,
                                  const SkPMColor* SK_RESTRICT src,
                                  int count, U8CPU alpha) {

    SkASSERT(255 == alpha);
    if (count > 0) {


	uint8x8_t alpha_mask;

	static const uint8_t alpha_mask_setup[] = {3,3,3,3,7,7,7,7};
	alpha_mask = vld1_u8(alpha_mask_setup);

	
#define	UNROLL	4
	while (count >= UNROLL) {
	    uint8x8_t src_raw, dst_raw, dst_final;
	    uint8x8_t src_raw_2, dst_raw_2, dst_final_2;

	    
	    src_raw = vreinterpret_u8_u32(vld1_u32(src));
#if	UNROLL > 2
	    src_raw_2 = vreinterpret_u8_u32(vld1_u32(src+2));
#endif

	    
	    dst_raw = vreinterpret_u8_u32(vld1_u32(dst));
#if	UNROLL > 2
	    dst_raw_2 = vreinterpret_u8_u32(vld1_u32(dst+2));
#endif

	
	{
	    uint8x8_t dst_cooked;
	    uint16x8_t dst_wide;
	    uint8x8_t alpha_narrow;
	    uint16x8_t alpha_wide;

	    
	    alpha_narrow = vtbl1_u8(src_raw, alpha_mask);
#if 1
	    
	    
	    alpha_wide = vsubw_u8(vdupq_n_u16(256), alpha_narrow);
#else
	    alpha_wide = vsubw_u8(vdupq_n_u16(255), alpha_narrow);
	    alpha_wide = vaddq_u16(alpha_wide, vshrq_n_u16(alpha_wide,7));
#endif

	    
	    dst_wide = vmovl_u8(dst_raw);

	    
	    dst_wide = vmulq_u16 (dst_wide, alpha_wide);
	    dst_cooked = vshrn_n_u16(dst_wide, 8);

	    
	    dst_final = vadd_u8(src_raw, dst_cooked);
	}

#if	UNROLL > 2
	
	{
	    uint8x8_t dst_cooked;
	    uint16x8_t dst_wide;
	    uint8x8_t alpha_narrow;
	    uint16x8_t alpha_wide;

	    alpha_narrow = vtbl1_u8(src_raw_2, alpha_mask);
#if 1
	    
	    
	    alpha_wide = vsubw_u8(vdupq_n_u16(256), alpha_narrow);
#else
	    alpha_wide = vsubw_u8(vdupq_n_u16(255), alpha_narrow);
	    alpha_wide = vaddq_u16(alpha_wide, vshrq_n_u16(alpha_wide,7));
#endif

	    
	    dst_wide = vmovl_u8(dst_raw_2);

	    
	    dst_wide = vmulq_u16 (dst_wide, alpha_wide);
	    dst_cooked = vshrn_n_u16(dst_wide, 8);

	    
	    dst_final_2 = vadd_u8(src_raw_2, dst_cooked);
	}
#endif

	    vst1_u32(dst, vreinterpret_u32_u8(dst_final));
#if	UNROLL > 2
	    vst1_u32(dst+2, vreinterpret_u32_u8(dst_final_2));
#endif

	    src += UNROLL;
	    dst += UNROLL;
	    count -= UNROLL;
	}
#undef	UNROLL

	
        while (--count >= 0) {
#ifdef TEST_SRC_ALPHA
            SkPMColor sc = *src;
            if (sc) {
                unsigned srcA = SkGetPackedA32(sc);
                SkPMColor result = sc;
                if (srcA != 255) {
                    result = SkPMSrcOver(sc, *dst);
                }
                *dst = result;
            }
#else
            *dst = SkPMSrcOver(*src, *dst);
#endif
            src += 1;
            dst += 1;
        }
    }
}

#define	S32A_Opaque_BlitRow32_PROC	S32A_Opaque_BlitRow32_neon

#else

#ifdef TEST_SRC_ALPHA
#error The ARM asm version of S32A_Opaque_BlitRow32 does not support TEST_SRC_ALPHA
#endif

static void S32A_Opaque_BlitRow32_arm(SkPMColor* SK_RESTRICT dst,
                                  const SkPMColor* SK_RESTRICT src,
                                  int count, U8CPU alpha) {

    SkASSERT(255 == alpha);

    
    asm volatile (
                  "cmp    %[count], #0               \n\t" 
                  "beq    3f                         \n\t" 

                  "mov    ip, #0xff                  \n\t" 
                  "orr    ip, ip, ip, lsl #16        \n\t" 

                  "cmp    %[count], #2               \n\t" 
                  "blt    2f                         \n\t" 

                  
                  "1:                                \n\t" 
                  "ldm    %[src]!, {r5,r6}           \n\t" 
                  "ldm    %[dst], {r7,r8}            \n\t" 
                  "lsr    r4, r5, #24                \n\t" 

                  
                  "and    r9, ip, r7                 \n\t" 
                  "rsb    r4, r4, #256               \n\t" 
                  "and    r10, ip, r7, lsr #8        \n\t" 

                  "mul    r9, r9, r4                 \n\t" 
                  "mul    r10, r10, r4               \n\t" 
                  "and    r9, ip, r9, lsr #8         \n\t" 

                  "and    r10, r10, ip, lsl #8       \n\t" 
                  "lsr    r4, r6, #24                \n\t" 
                  "orr    r7, r9, r10                \n\t" 

                  "add    r7, r5, r7                 \n\t" 
                  "rsb    r4, r4, #256               \n\t" 

                  
                  "and    r9, ip, r8                 \n\t" 

                  "and    r10, ip, r8, lsr #8        \n\t" 
                  "mul    r9, r9, r4                 \n\t" 
                  "sub    %[count], %[count], #2     \n\t"
                  "mul    r10, r10, r4               \n\t" 

                  "and    r9, ip, r9, lsr #8         \n\t" 
                  "and    r10, r10, ip, lsl #8       \n\t" 
                  "cmp    %[count], #1               \n\t" 
                  "orr    r8, r9, r10                \n\t" 

                  "add    r8, r6, r8                 \n\t" 

                  
                  "stm    %[dst]!, {r7,r8}           \n\t" 
                  

                  "bgt    1b                         \n\t" 
                  "blt    3f                         \n\t" 

                  
                  "2:                                \n\t" 
                  "ldr    r5, [%[src]], #4           \n\t" 
                  "ldr    r7, [%[dst]]               \n\t" 
                  "lsr    r4, r5, #24                \n\t" 

                  
                  "and    r9, ip, r7                 \n\t" 
                  "rsb    r4, r4, #256               \n\t" 

                  "and    r10, ip, r7, lsr #8        \n\t" 
                  "mul    r9, r9, r4                 \n\t" 
                  "mul    r10, r10, r4               \n\t" 
                  "and    r9, ip, r9, lsr #8         \n\t" 

                  "and    r10, r10, ip, lsl #8       \n\t" 
                  "orr    r7, r9, r10                \n\t" 

                  "add    r7, r5, r7                 \n\t" 

                  
                  "str    r7, [%[dst]], #4           \n\t" 
                  

                  "3:                                \n\t" 
                  : [dst] "+r" (dst), [src] "+r" (src), [count] "+r" (count)
                  :
                  : "cc", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "ip", "memory"
                  );
}
#define	S32A_Opaque_BlitRow32_PROC	S32A_Opaque_BlitRow32_arm
#endif




static void S32A_Blend_BlitRow32_arm(SkPMColor* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src,
                                 int count, U8CPU alpha) {
    asm volatile (
                  "cmp    %[count], #0               \n\t" 
                  "beq    3f                         \n\t" 

                  "mov    r12, #0xff                 \n\t" 
                  "orr    r12, r12, r12, lsl #16     \n\t" 

                  
                  "add    %[alpha], %[alpha], #1     \n\t" 

                  "cmp    %[count], #2               \n\t" 
                  "blt    2f                         \n\t" 

                  
                  "1:                                \n\t" 
                  "ldm    %[src]!, {r5, r6}          \n\t" 
                  "ldm    %[dst], {r7, r8}           \n\t" 

                  
                  "lsr    r9, r5, #24                \n\t" 
                  "lsr    r10, r6, #24               \n\t" 
                  "smulbb r9, r9, %[alpha]           \n\t" 
                  "smulbb r10, r10, %[alpha]         \n\t" 
                  "lsr    r9, r9, #8                 \n\t" 
                  "lsr    r10, r10, #8               \n\t" 
                  "rsb    r9, r9, #256               \n\t" 
                  "rsb    r10, r10, #256             \n\t" 

                  

                  
                  "and    r11, r12, r5, lsr #8       \n\t" 
                  "and    r4, r12, r5                \n\t" 
                  "mul    r11, r11, %[alpha]         \n\t" 
                  "mul    r4, r4, %[alpha]           \n\t" 
                  "and    r11, r11, r12, lsl #8      \n\t" 
                  "and    r4, r12, r4, lsr #8        \n\t" 
                  "orr    r5, r11, r4                \n\t" 

                  
                  "and    r11, r12, r7, lsr #8       \n\t" 
                  "and    r4, r12, r7                \n\t" 
                  "mul    r11, r11, r9               \n\t" 
                  "mul    r4, r4, r9                 \n\t" 
                  "and    r11, r11, r12, lsl #8      \n\t" 
                  "and    r4, r12, r4, lsr #8        \n\t" 
                  "orr    r9, r11, r4                \n\t" 

                  
                  "add    r9, r5, r9                 \n\t" 
                  

                  

                  
                  "and    r11, r12, r6, lsr #8       \n\t" 
                  "and    r4, r12, r6                \n\t" 
                  "mul    r11, r11, %[alpha]         \n\t" 
                  "mul    r4, r4, %[alpha]           \n\t" 
                  "and    r11, r11, r12, lsl #8      \n\t" 
                  "and    r4, r12, r4, lsr #8        \n\t" 
                  "orr    r6, r11, r4                \n\t" 

                  
                  "and    r11, r12, r8, lsr #8       \n\t" 
                  "and    r4, r12, r8                \n\t" 
                  "mul    r11, r11, r10              \n\t" 
                  "mul    r4, r4, r10                \n\t" 
                  "and    r11, r11, r12, lsl #8      \n\t" 
                  "and    r4, r12, r4, lsr #8        \n\t" 
                  "orr    r10, r11, r4               \n\t" 

                  "sub    %[count], %[count], #2     \n\t" 
                  
                  "add    r10, r6, r10               \n\t" 
                  
                  "cmp    %[count], #1               \n\t" 
                  
                  "stm    %[dst]!, {r9, r10}         \n\t" 
                  

                  "bgt    1b                         \n\t" 
                  "blt    3f                         \n\t" 
                                                           
                  
                  "2:                                \n\t" 
                  "ldr    r5, [%[src]], #4           \n\t" 
                  "ldr    r7, [%[dst]]               \n\t" 

                  "lsr    r6, r5, #24                \n\t" 
                  "and    r8, r12, r5, lsr #8        \n\t" 
                  "smulbb r6, r6, %[alpha]           \n\t" 
                  "and    r9, r12, r5                \n\t" 
                  "lsr    r6, r6, #8                 \n\t" 
                  "mul    r8, r8, %[alpha]           \n\t" 
                  "rsb    r6, r6, #256               \n\t" 

                  
                  "mul    r9, r9, %[alpha]           \n\t" 
                  "and    r8, r8, r12, lsl #8        \n\t" 
                  "and    r9, r12, r9, lsr #8        \n\t" 
                  "orr    r10, r8, r9                \n\t" 

                  
                  "and    r8, r12, r7, lsr #8        \n\t" 
                  "and    r9, r12, r7                \n\t" 
                  "mul    r8, r8, r6                 \n\t" 
                  "mul    r9, r9, r6                 \n\t" 
                  "and    r8, r8, r12, lsl #8        \n\t" 
                  "and    r9, r12, r9, lsr #8        \n\t" 
                  "orr    r7, r8, r9                 \n\t" 

                  "add    r10, r7, r10               \n\t" 

                  
                  "str    r10, [%[dst]], #4          \n\t" 
                  

                  "3:                                \n\t" 
                  : [dst] "+r" (dst), [src] "+r" (src), [count] "+r" (count), [alpha] "+r" (alpha)
                  :
                  : "cc", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "memory"
                  );

}
#define	S32A_Blend_BlitRow32_PROC	S32A_Blend_BlitRow32_arm




#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)
static void S32_Blend_BlitRow32_neon(SkPMColor* SK_RESTRICT dst,
                                const SkPMColor* SK_RESTRICT src,
                                int count, U8CPU alpha) {
    SkASSERT(alpha <= 255);
    if (count > 0) {
        uint16_t src_scale = SkAlpha255To256(alpha);
        uint16_t dst_scale = 256 - src_scale;

	
	







	





#define	UNROLL	2
	while (count >= UNROLL) {
	    uint8x8_t  src_raw, dst_raw, dst_final;
	    uint16x8_t  src_wide, dst_wide;

	    
	    src_raw = vreinterpret_u8_u32(vld1_u32(src));
	    src_wide = vmovl_u8(src_raw);
	    
	    src_wide = vmulq_u16 (src_wide, vdupq_n_u16(src_scale));

	    
	    dst_raw = vreinterpret_u8_u32(vld1_u32(dst));
	    dst_wide = vmovl_u8(dst_raw);

	    
	    dst_wide = vmlaq_u16(src_wide, dst_wide, vdupq_n_u16(dst_scale));

	    dst_final = vshrn_n_u16(dst_wide, 8);
	    vst1_u32(dst, vreinterpret_u32_u8(dst_final));

	    src += UNROLL;
	    dst += UNROLL;
	    count -= UNROLL;
	}
	







#if	UNROLL == 2
	if (count == 1) {
            *dst = SkAlphaMulQ(*src, src_scale) + SkAlphaMulQ(*dst, dst_scale);
	}
#else
	if (count > 0) {
            do {
                *dst = SkAlphaMulQ(*src, src_scale) + SkAlphaMulQ(*dst, dst_scale);
                src += 1;
                dst += 1;
            } while (--count > 0);
	}
#endif

#undef	UNROLL
    }
}

#define	S32_Blend_BlitRow32_PROC	S32_Blend_BlitRow32_neon
#else
#define	S32_Blend_BlitRow32_PROC	NULL
#endif



#if defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)

#undef	DEBUG_OPAQUE_DITHER

#if	defined(DEBUG_OPAQUE_DITHER)
static void showme8(char *str, void *p, int len)
{
	static char buf[256];
	char tbuf[32];
	int i;
	char *pc = (char*) p;
	sprintf(buf,"%8s:", str);
	for(i=0;i<len;i++) {
	    sprintf(tbuf, "   %02x", pc[i]);
	    strcat(buf, tbuf);
	}
	SkDebugf("%s\n", buf);
}
static void showme16(char *str, void *p, int len)
{
	static char buf[256];
	char tbuf[32];
	int i;
	uint16_t *pc = (uint16_t*) p;
	sprintf(buf,"%8s:", str);
	len = (len / sizeof(uint16_t));	
	for(i=0;i<len;i++) {
	    sprintf(tbuf, " %04x", pc[i]);
	    strcat(buf, tbuf);
	}
	SkDebugf("%s\n", buf);
}
#endif

static void S32A_D565_Opaque_Dither_neon (uint16_t * SK_RESTRICT dst,
                                      const SkPMColor* SK_RESTRICT src,
                                      int count, U8CPU alpha, int x, int y) {
    SkASSERT(255 == alpha);

#define	UNROLL	8

    if (count >= UNROLL) {
	uint8x8_t dbase;

#if	defined(DEBUG_OPAQUE_DITHER)
	uint16_t tmpbuf[UNROLL];
	int td[UNROLL];
	int tdv[UNROLL];
	int ta[UNROLL];
	int tap[UNROLL];
	uint16_t in_dst[UNROLL];
	int offset = 0;
	int noisy = 0;
#endif

	const uint8_t *dstart = &gDitherMatrix_Neon[(y&3)*12 + (x&3)];
	dbase = vld1_u8(dstart);

        do {
	    uint8x8_t sr, sg, sb, sa, d;
	    uint16x8_t dst8, scale8, alpha8;
	    uint16x8_t dst_r, dst_g, dst_b;

#if	defined(DEBUG_OPAQUE_DITHER)
	
	{
	  int my_y = y;
	  int my_x = x;
	  SkPMColor* my_src = (SkPMColor*)src;
	  uint16_t* my_dst = dst;
	  int i;

          DITHER_565_SCAN(my_y);
          for(i=0;i<UNROLL;i++) {
            SkPMColor c = *my_src++;
            SkPMColorAssert(c);
            if (c) {
                unsigned a = SkGetPackedA32(c);
                
                int d = SkAlphaMul(DITHER_VALUE(my_x), SkAlpha255To256(a));
		tdv[i] = DITHER_VALUE(my_x);
		ta[i] = a;
		tap[i] = SkAlpha255To256(a);
		td[i] = d;
                
                unsigned sr = SkGetPackedR32(c);
                unsigned sg = SkGetPackedG32(c);
                unsigned sb = SkGetPackedB32(c);
                sr = SkDITHER_R32_FOR_565(sr, d);
                sg = SkDITHER_G32_FOR_565(sg, d);
                sb = SkDITHER_B32_FOR_565(sb, d);
                
                uint32_t src_expanded = (sg << 24) | (sr << 13) | (sb << 2);
                uint32_t dst_expanded = SkExpand_rgb_16(*my_dst);
                dst_expanded = dst_expanded * (SkAlpha255To256(255 - a) >> 3);
                
                tmpbuf[i] = SkCompact_rgb_16((src_expanded + dst_expanded) >> 5);
		td[i] = d;

            } else {
		tmpbuf[i] = *my_dst;
		ta[i] = tdv[i] = td[i] = 0xbeef;
	    }
	    in_dst[i] = *my_dst;
            my_dst += 1;
            DITHER_INC_X(my_x);
          }
	}
#endif

	    
	    {
		register uint8x8_t d0 asm("d0");
		register uint8x8_t d1 asm("d1");
		register uint8x8_t d2 asm("d2");
		register uint8x8_t d3 asm("d3");

		asm ("vld4.8	{d0-d3},[%4]  /* r=%P0 g=%P1 b=%P2 a=%P3 */"
		    : "=w" (d0), "=w" (d1), "=w" (d2), "=w" (d3)
		    : "r" (src)
                    );
		    sr = d0; sg = d1; sb = d2; sa = d3;
	    }

	    
	    
#if ANDROID
	    
	    alpha8 = vaddw_u8(vmovl_u8(sa), vdup_n_u8(1));
#else
	    alpha8 = vaddw_u8(vmovl_u8(sa), vshr_n_u8(sa, 7));
#endif
	    alpha8 = vmulq_u16(alpha8, vmovl_u8(dbase)); 
	    d = vshrn_n_u16(alpha8, 8);	
	    
	    
	    


	    sr = vsub_u8(sr, vshr_n_u8(sr, 5));
	    sr = vadd_u8(sr, d);

	    
	    sb = vsub_u8(sb, vshr_n_u8(sb, 5));
	    sb = vadd_u8(sb, d);

	    
	    sg = vsub_u8(sg, vshr_n_u8(sg, 6));
	    sg = vadd_u8(sg, vshr_n_u8(d,1));

	    
	    dst8 = vld1q_u16(dst);
	    dst_b = vandq_u16(dst8, vdupq_n_u16(0x001F));
	    dst_g = vandq_u16(vshrq_n_u16(dst8,5), vdupq_n_u16(0x003F));
	    dst_r = vshrq_n_u16(dst8,11);	

	    
#if 1
	    
	    
	    scale8 = vsubw_u8(vdupq_n_u16(256), sa);
#else
	    scale8 = vsubw_u8(vdupq_n_u16(255), sa);
	    scale8 = vaddq_u16(scale8, vshrq_n_u16(scale8, 7));
#endif

#if 1
	    
	    scale8 = vshrq_n_u16(scale8, 3);
	    dst_b = vmlaq_u16(vshll_n_u8(sb,2), dst_b, scale8);
	    dst_g = vmlaq_u16(vshll_n_u8(sg,3), dst_g, scale8);
	    dst_r = vmlaq_u16(vshll_n_u8(sr,2), dst_r, scale8);
#else
	    
	    scale8 = vshrq_n_u16(scale8, 3);
	    dst_b = vmulq_u16(dst_b, scale8);
	    dst_g = vmulq_u16(dst_g, scale8);
	    dst_r = vmulq_u16(dst_r, scale8);

	    
	    
	    dst_b = vaddq_u16(dst_b, vshll_n_u8(sb,2));
	    dst_g = vaddq_u16(dst_g, vshll_n_u8(sg,3));
	    dst_r = vaddq_u16(dst_r, vshll_n_u8(sr,2));
#endif

	    
	    dst8 = vandq_u16(vshrq_n_u16(dst_b, 5), vdupq_n_u16(0x001F));
	    dst8 = vsliq_n_u16(dst8, vshrq_n_u16(dst_g, 5), 5);
	    dst8 = vsliq_n_u16(dst8, vshrq_n_u16(dst_r,5), 11);

	    vst1q_u16(dst, dst8);

#if	defined(DEBUG_OPAQUE_DITHER)
	    
	{
	   int i, bad=0;
	   static int invocation;

	   for (i=0;i<UNROLL;i++)
		if (tmpbuf[i] != dst[i]) bad=1;
	   if (bad) {
		SkDebugf("BAD S32A_D565_Opaque_Dither_neon(); invocation %d offset %d\n",
			invocation, offset);
		SkDebugf("  alpha 0x%x\n", alpha);
		for (i=0;i<UNROLL;i++)
		    SkDebugf("%2d: %s %04x w %04x id %04x s %08x d %04x %04x %04x %04x\n",
			i, ((tmpbuf[i] != dst[i])?"BAD":"got"),
			dst[i], tmpbuf[i], in_dst[i], src[i], td[i], tdv[i], tap[i], ta[i]);

		showme16("alpha8", &alpha8, sizeof(alpha8));
		showme16("scale8", &scale8, sizeof(scale8));
		showme8("d", &d, sizeof(d));
		showme16("dst8", &dst8, sizeof(dst8));
		showme16("dst_b", &dst_b, sizeof(dst_b));
		showme16("dst_g", &dst_g, sizeof(dst_g));
		showme16("dst_r", &dst_r, sizeof(dst_r));
		showme8("sb", &sb, sizeof(sb));
		showme8("sg", &sg, sizeof(sg));
		showme8("sr", &sr, sizeof(sr));

		
		return;
	   }
	   offset += UNROLL;
	   invocation++;
	}
#endif

            dst += UNROLL;
	    src += UNROLL;
	    count -= UNROLL;
	    
        } while (count >= UNROLL);
    }
#undef	UNROLL

    
    if (count > 0) {
        DITHER_565_SCAN(y);
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            if (c) {
                unsigned a = SkGetPackedA32(c);
                
                
                
                unsigned dither = DITHER_VALUE(x);
                unsigned alpha = SkAlpha255To256(a);
                int d = SkAlphaMul(dither, alpha);
                
                unsigned sr = SkGetPackedR32(c);
                unsigned sg = SkGetPackedG32(c);
                unsigned sb = SkGetPackedB32(c);
                sr = SkDITHER_R32_FOR_565(sr, d);
                sg = SkDITHER_G32_FOR_565(sg, d);
                sb = SkDITHER_B32_FOR_565(sb, d);
                
                uint32_t src_expanded = (sg << 24) | (sr << 13) | (sb << 2);
                uint32_t dst_expanded = SkExpand_rgb_16(*dst);
                dst_expanded = dst_expanded * (SkAlpha255To256(255 - a) >> 3);
                
                *dst = SkCompact_rgb_16((src_expanded + dst_expanded) >> 5);
            }
            dst += 1;
            DITHER_INC_X(x);
        } while (--count != 0);
    }
}

#define	S32A_D565_Opaque_Dither_PROC S32A_D565_Opaque_Dither_neon
#else
#define	S32A_D565_Opaque_Dither_PROC NULL
#endif



#if	defined(__ARM_HAVE_NEON) && defined(SK_CPU_LENDIAN)







#undef	DEBUG_S32_OPAQUE_DITHER

static void S32_D565_Opaque_Dither_neon(uint16_t* SK_RESTRICT dst,
                                     const SkPMColor* SK_RESTRICT src,
                                     int count, U8CPU alpha, int x, int y) {
    SkASSERT(255 == alpha);

#define	UNROLL	8
    if (count >= UNROLL) {
	uint8x8_t d;
	const uint8_t *dstart = &gDitherMatrix_Neon[(y&3)*12 + (x&3)];
	d = vld1_u8(dstart);

	while (count >= UNROLL) {
	    uint8x8_t sr, sg, sb, sa;
	    uint16x8_t dr, dg, db, da;
	    uint16x8_t dst8;

	    
	    {
		register uint8x8_t d0 asm("d0");
		register uint8x8_t d1 asm("d1");
		register uint8x8_t d2 asm("d2");
		register uint8x8_t d3 asm("d3");

		asm ("vld4.8	{d0-d3},[%4]  /* r=%P0 g=%P1 b=%P2 a=%P3 */"
		    : "=w" (d0), "=w" (d1), "=w" (d2), "=w" (d3)
		    : "r" (src)
                    );
		    sr = d0; sg = d1; sb = d2; sa = d3;
	    }
	    





	    
	    sr = vsub_u8(sr, vshr_n_u8(sr, 5));
	    dr = vaddl_u8(sr, d);

	    
	    sb = vsub_u8(sb, vshr_n_u8(sb, 5));
	    db = vaddl_u8(sb, d);

	    
	    sg = vsub_u8(sg, vshr_n_u8(sg, 6));
	    dg = vaddl_u8(sg, vshr_n_u8(d,1));
	    

	    
	    dst8 = vshrq_n_u16(db, 3);
	    dst8 = vsliq_n_u16(dst8, vshrq_n_u16(dg, 2), 5);
	    dst8 = vsliq_n_u16(dst8, vshrq_n_u16(dr,3), 11);

	    
	    vst1q_u16(dst, dst8);

#if	defined(DEBUG_S32_OPAQUE_DITHER)
	    
	    {
		int i, myx = x, myy = y;
		DITHER_565_SCAN(myy);
		for (i=0;i<UNROLL;i++) {
		    SkPMColor c = src[i];
		    unsigned dither = DITHER_VALUE(myx);
		    uint16_t val = SkDitherRGB32To565(c, dither);
		    if (val != dst[i]) {
			SkDebugf("RBE: src %08x dither %02x, want %04x got %04x dbas[i] %02x\n",
			    c, dither, val, dst[i], dstart[i]);
		    }
		    DITHER_INC_X(myx);
		}
	    }
#endif

	    dst += UNROLL;
	    src += UNROLL;
	    count -= UNROLL;
	    x += UNROLL;		
	}
    }
#undef	UNROLL

    
    if (count > 0) {
        DITHER_565_SCAN(y);
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            SkASSERT(SkGetPackedA32(c) == 255);

            unsigned dither = DITHER_VALUE(x);
            *dst++ = SkDitherRGB32To565(c, dither);
            DITHER_INC_X(x);
        } while (--count != 0);
    }
}

#define	S32_D565_Opaque_Dither_PROC S32_D565_Opaque_Dither_neon
#else
#define	S32_D565_Opaque_Dither_PROC NULL
#endif



static const SkBlitRow::Proc platform_565_procs[] = {
    
    S32_D565_Opaque_PROC,
    S32_D565_Blend_PROC,
    S32A_D565_Opaque_PROC,
    S32A_D565_Blend_PROC,
    
    
    S32_D565_Opaque_Dither_PROC,
    S32_D565_Blend_Dither_PROC,
    S32A_D565_Opaque_Dither_PROC,
    NULL,   
};

static const SkBlitRow::Proc platform_4444_procs[] = {
    
    NULL,   
    NULL,   
    NULL,   
    NULL,   
    
    
    NULL,   
    NULL,   
    NULL,   
    NULL,   
};

static const SkBlitRow::Proc32 platform_32_procs[] = {
    NULL,   
    S32_Blend_BlitRow32_PROC,		
    S32A_Opaque_BlitRow32_PROC,		
    S32A_Blend_BlitRow32_PROC		
};

SkBlitRow::Proc SkBlitRow::PlatformProcs4444(unsigned flags) {
    return platform_4444_procs[flags];
}

SkBlitRow::Proc SkBlitRow::PlatformProcs565(unsigned flags) {
    return platform_565_procs[flags];
}

SkBlitRow::Proc32 SkBlitRow::PlatformProcs32(unsigned flags) {
    return platform_32_procs[flags];
}

SkBlitRow::ColorProc SkBlitRow::PlatformColorProc() {
    return NULL;
}


SkBlitMask::Proc SkBlitMask::PlatformProcs(SkBitmap::Config dstConfig,
                                           SkMask::Format maskFormat,
                                           SkColor color)
{
   return NULL;
}
