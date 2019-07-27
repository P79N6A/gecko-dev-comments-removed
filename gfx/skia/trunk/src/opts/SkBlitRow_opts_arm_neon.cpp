






#include "SkBlitRow_opts_arm_neon.h"

#include "SkBlitMask.h"
#include "SkBlitRow.h"
#include "SkColorPriv.h"
#include "SkDither.h"
#include "SkMathPriv.h"
#include "SkUtils.h"

#include "SkColor_opts_neon.h"
#include <arm_neon.h>

#ifdef SK_CPU_ARM64
static inline uint8x8x4_t sk_vld4_u8_arm64_3(const SkPMColor* SK_RESTRICT & src) {
    uint8x8x4_t vsrc;
    uint8x8_t vsrc_0, vsrc_1, vsrc_2;

    asm (
        "ld4    {v0.8b - v3.8b}, [%[src]], #32 \t\n"
        "mov    %[vsrc0].8b, v0.8b             \t\n"
        "mov    %[vsrc1].8b, v1.8b             \t\n"
        "mov    %[vsrc2].8b, v2.8b             \t\n"
        : [vsrc0] "=w" (vsrc_0), [vsrc1] "=w" (vsrc_1),
          [vsrc2] "=w" (vsrc_2), [src] "+&r" (src)
        : : "v0", "v1", "v2", "v3"
    );

    vsrc.val[0] = vsrc_0;
    vsrc.val[1] = vsrc_1;
    vsrc.val[2] = vsrc_2;

    return vsrc;
}

static inline uint8x8x4_t sk_vld4_u8_arm64_4(const SkPMColor* SK_RESTRICT & src) {
    uint8x8x4_t vsrc;
    uint8x8_t vsrc_0, vsrc_1, vsrc_2, vsrc_3;

    asm (
        "ld4    {v0.8b - v3.8b}, [%[src]], #32 \t\n"
        "mov    %[vsrc0].8b, v0.8b             \t\n"
        "mov    %[vsrc1].8b, v1.8b             \t\n"
        "mov    %[vsrc2].8b, v2.8b             \t\n"
        "mov    %[vsrc3].8b, v3.8b             \t\n"
        : [vsrc0] "=w" (vsrc_0), [vsrc1] "=w" (vsrc_1),
          [vsrc2] "=w" (vsrc_2), [vsrc3] "=w" (vsrc_3),
          [src] "+&r" (src)
        : : "v0", "v1", "v2", "v3"
    );

    vsrc.val[0] = vsrc_0;
    vsrc.val[1] = vsrc_1;
    vsrc.val[2] = vsrc_2;
    vsrc.val[3] = vsrc_3;

    return vsrc;
}
#endif

void S32_D565_Opaque_neon(uint16_t* SK_RESTRICT dst,
                           const SkPMColor* SK_RESTRICT src, int count,
                           U8CPU alpha, int , int ) {
    SkASSERT(255 == alpha);

    while (count >= 8) {
        uint8x8x4_t vsrc;
        uint16x8_t vdst;

        
#ifdef SK_CPU_ARM64
        vsrc = sk_vld4_u8_arm64_3(src);
#else
        vsrc = vld4_u8((uint8_t*)src);
        src += 8;
#endif

        
        vdst = SkPixel32ToPixel16_neon8(vsrc);

        
        vst1q_u16(dst, vdst);

        
        dst += 8;
        count -= 8;
    };

    
    while (count > 0) {
        SkPMColor c = *src++;
        SkPMColorAssert(c);
        *dst = SkPixel32ToPixel16_ToU16(c);
        dst++;
        count--;
    };
}

void S32_D565_Blend_neon(uint16_t* SK_RESTRICT dst,
                          const SkPMColor* SK_RESTRICT src, int count,
                          U8CPU alpha, int , int ) {
    SkASSERT(255 > alpha);

    uint16x8_t vmask_blue, vscale;

    
    vscale = vdupq_n_u16(SkAlpha255To256(alpha));
    vmask_blue = vmovq_n_u16(0x1F);

    while (count >= 8) {
        uint8x8x4_t vsrc;
        uint16x8_t vdst, vdst_r, vdst_g, vdst_b;
        uint16x8_t vres_r, vres_g, vres_b;

        
#ifdef SK_CPU_ARM64
        vsrc = sk_vld4_u8_arm64_3(src);
#else
        {
        register uint8x8_t d0 asm("d0");
        register uint8x8_t d1 asm("d1");
        register uint8x8_t d2 asm("d2");
        register uint8x8_t d3 asm("d3");

        asm (
            "vld4.8    {d0-d3},[%[src]]!"
            : "=w" (d0), "=w" (d1), "=w" (d2), "=w" (d3), [src] "+&r" (src)
            :
        );
        vsrc.val[0] = d0;
        vsrc.val[1] = d1;
        vsrc.val[2] = d2;
        }
#endif

        
        vdst = vld1q_u16(dst);
        vdst_g = vshlq_n_u16(vdst, 5);        
        vdst_b = vandq_u16(vdst, vmask_blue); 
        vdst_r = vshrq_n_u16(vdst, 6+5);      
        vdst_g = vshrq_n_u16(vdst_g, 5+5);    

        
        vsrc.val[NEON_R] = vshr_n_u8(vsrc.val[NEON_R], 3);
        vsrc.val[NEON_G] = vshr_n_u8(vsrc.val[NEON_G], 2);
        vsrc.val[NEON_B] = vshr_n_u8(vsrc.val[NEON_B], 3);

        
        vres_r = vmovl_u8(vsrc.val[NEON_R]) - vdst_r;
        vres_g = vmovl_u8(vsrc.val[NEON_G]) - vdst_g;
        vres_b = vmovl_u8(vsrc.val[NEON_B]) - vdst_b;

        vres_r = vshrq_n_u16(vres_r * vscale, 8);
        vres_g = vshrq_n_u16(vres_g * vscale, 8);
        vres_b = vshrq_n_u16(vres_b * vscale, 8);

        vres_r += vdst_r;
        vres_g += vdst_g;
        vres_b += vdst_b;

        
        vres_b = vsliq_n_u16(vres_b, vres_g, 5);    
        vres_b = vsliq_n_u16(vres_b, vres_r, 6+5);  

        
        vst1q_u16(dst, vres_b);
        dst += 8;
        count -= 8;
    }
    if (count > 0) {
        int scale = SkAlpha255To256(alpha);
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            uint16_t d = *dst;
            *dst++ = SkPackRGB16(
                    SkAlphaBlend(SkPacked32ToR16(c), SkGetPackedR16(d), scale),
                    SkAlphaBlend(SkPacked32ToG16(c), SkGetPackedG16(d), scale),
                    SkAlphaBlend(SkPacked32ToB16(c), SkGetPackedB16(d), scale));
        } while (--count != 0);
    }
}

#ifdef SK_CPU_ARM32
void S32A_D565_Opaque_neon(uint16_t* SK_RESTRICT dst,
                           const SkPMColor* SK_RESTRICT src, int count,
                           U8CPU alpha, int , int ) {
    SkASSERT(255 == alpha);

    if (count >= 8) {
        uint16_t* SK_RESTRICT keep_dst = 0;

        asm volatile (
                      "ands       ip, %[count], #7            \n\t"
                      "vmov.u8    d31, #1<<7                  \n\t"
                      "vld1.16    {q12}, [%[dst]]             \n\t"
                      "vld4.8     {d0-d3}, [%[src]]           \n\t"
                      
                      
                      
                      "it eq                                  \n\t"
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
        uint16_t* SK_RESTRICT keep_dst = 0;

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

#else 

void S32A_D565_Opaque_neon(uint16_t* SK_RESTRICT dst,
                           const SkPMColor* SK_RESTRICT src, int count,
                           U8CPU alpha, int , int ) {
    SkASSERT(255 == alpha);

    if (count >= 16) {
        asm (
            "movi    v4.8h, #0x80                   \t\n"

            "1:                                     \t\n"
            "sub     %[count], %[count], #16        \t\n"
            "ld1     {v16.8h-v17.8h}, [%[dst]]      \t\n"
            "ld4     {v0.16b-v3.16b}, [%[src]], #64 \t\n"
            "prfm    pldl1keep, [%[src],#512]       \t\n"
            "prfm    pldl1keep, [%[dst],#256]       \t\n"
            "ushr    v20.8h, v17.8h, #5             \t\n"
            "ushr    v31.8h, v16.8h, #5             \t\n"
            "xtn     v6.8b, v31.8h                  \t\n"
            "xtn2    v6.16b, v20.8h                 \t\n"
            "ushr    v20.8h, v17.8h, #11            \t\n"
            "shl     v19.16b, v6.16b, #2            \t\n"
            "ushr    v31.8h, v16.8h, #11            \t\n"
            "xtn     v22.8b, v31.8h                 \t\n"
            "xtn2    v22.16b, v20.8h                \t\n"
            "shl     v18.16b, v22.16b, #3           \t\n"
            "mvn     v3.16b, v3.16b                 \t\n"
            "xtn     v16.8b, v16.8h                 \t\n"
            "mov     v7.16b, v4.16b                 \t\n"
            "xtn2    v16.16b, v17.8h                \t\n"
            "umlal   v7.8h, v3.8b, v19.8b           \t\n"
            "shl     v16.16b, v16.16b, #3           \t\n"
            "mov     v22.16b, v4.16b                \t\n"
            "ushr    v24.8h, v7.8h, #6              \t\n"
            "umlal   v22.8h, v3.8b, v18.8b          \t\n"
            "ushr    v20.8h, v22.8h, #5             \t\n"
            "addhn   v20.8b, v22.8h, v20.8h         \t\n"
            "cmp     %[count], #16                  \t\n"
            "mov     v6.16b, v4.16b                 \t\n"
            "mov     v5.16b, v4.16b                 \t\n"
            "umlal   v6.8h, v3.8b, v16.8b           \t\n"
            "umlal2  v5.8h, v3.16b, v19.16b         \t\n"
            "mov     v17.16b, v4.16b                \t\n"
            "ushr    v19.8h, v6.8h, #5              \t\n"
            "umlal2  v17.8h, v3.16b, v18.16b        \t\n"
            "addhn   v7.8b, v7.8h, v24.8h           \t\n"
            "ushr    v18.8h, v5.8h, #6              \t\n"
            "ushr    v21.8h, v17.8h, #5             \t\n"
            "addhn2  v7.16b, v5.8h, v18.8h          \t\n"
            "addhn2  v20.16b, v17.8h, v21.8h        \t\n"
            "mov     v22.16b, v4.16b                \t\n"
            "addhn   v6.8b, v6.8h, v19.8h           \t\n"
            "umlal2  v22.8h, v3.16b, v16.16b        \t\n"
            "ushr    v5.8h, v22.8h, #5              \t\n"
            "addhn2  v6.16b, v22.8h, v5.8h          \t\n"
            "uqadd   v7.16b, v1.16b, v7.16b         \t\n"
            "uqadd   v20.16b, v2.16b, v20.16b       \t\n"
            "uqadd   v6.16b, v0.16b, v6.16b         \t\n"
            "shll    v22.8h, v20.8b, #8             \t\n"
            "shll    v5.8h, v7.8b, #8               \t\n"
            "sri     v22.8h, v5.8h, #5              \t\n"
            "shll    v17.8h, v6.8b, #8              \t\n"
            "shll2   v23.8h, v20.16b, #8            \t\n"
            "shll2   v7.8h, v7.16b, #8              \t\n"
            "sri     v22.8h, v17.8h, #11            \t\n"
            "sri     v23.8h, v7.8h, #5              \t\n"
            "shll2   v6.8h, v6.16b, #8              \t\n"
            "st1     {v22.8h}, [%[dst]], #16        \t\n"
            "sri     v23.8h, v6.8h, #11             \t\n"
            "st1     {v23.8h}, [%[dst]], #16        \t\n"
            "b.ge    1b                             \t\n"
            : [dst] "+&r" (dst), [src] "+&r" (src), [count] "+&r" (count)
            :: "cc", "memory", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7",
               "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24",
               "v31"
        );
    }
        
    if (count > 0) {
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            if (c) {
                *dst = SkSrcOver32To16(c, *dst);
            }
            dst += 1;
        } while (--count != 0);
    }
}
#endif 

static inline uint16x8_t SkDiv255Round_neon8(uint16x8_t prod) {
    prod += vdupq_n_u16(128);
    prod += vshrq_n_u16(prod, 8);
    return vshrq_n_u16(prod, 8);
}

void S32A_D565_Blend_neon(uint16_t* SK_RESTRICT dst,
                          const SkPMColor* SK_RESTRICT src, int count,
                          U8CPU alpha, int , int ) {
   SkASSERT(255 > alpha);

    




    if (count >= 8) {
        uint16x8_t valpha_max, vmask_blue;
        uint8x8_t valpha;

        
        valpha_max = vmovq_n_u16(255);
        valpha = vdup_n_u8(alpha);
        vmask_blue = vmovq_n_u16(SK_B16_MASK);

        do {
            uint16x8_t vdst, vdst_r, vdst_g, vdst_b;
            uint16x8_t vres_a, vres_r, vres_g, vres_b;
            uint8x8x4_t vsrc;

            
            vdst = vld1q_u16(dst);
#ifdef SK_CPU_ARM64
            vsrc = sk_vld4_u8_arm64_4(src);
#else
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 6))
            asm (
                "vld4.u8 %h[vsrc], [%[src]]!"
                : [vsrc] "=w" (vsrc), [src] "+&r" (src)
                : :
            );
#else
            register uint8x8_t d0 asm("d0");
            register uint8x8_t d1 asm("d1");
            register uint8x8_t d2 asm("d2");
            register uint8x8_t d3 asm("d3");

            asm volatile (
                "vld4.u8    {d0-d3},[%[src]]!;"
                : "=w" (d0), "=w" (d1), "=w" (d2), "=w" (d3),
                  [src] "+&r" (src)
                : :
            );
            vsrc.val[0] = d0;
            vsrc.val[1] = d1;
            vsrc.val[2] = d2;
            vsrc.val[3] = d3;
#endif
#endif 


            
            vdst_g = vshlq_n_u16(vdst, SK_R16_BITS);        
            vdst_b = vdst & vmask_blue;                     
            vdst_r = vshrq_n_u16(vdst, SK_R16_SHIFT);       
            vdst_g = vshrq_n_u16(vdst_g, SK_R16_BITS + SK_B16_BITS); 

            
            vsrc.val[NEON_R] = vshr_n_u8(vsrc.val[NEON_R], 8 - SK_R16_BITS);
            vsrc.val[NEON_G] = vshr_n_u8(vsrc.val[NEON_G], 8 - SK_G16_BITS);
            vsrc.val[NEON_B] = vshr_n_u8(vsrc.val[NEON_B], 8 - SK_B16_BITS);

            
            vres_a = vmull_u8(vsrc.val[NEON_A], valpha);
            vres_r = vmull_u8(vsrc.val[NEON_R], valpha);
            vres_g = vmull_u8(vsrc.val[NEON_G], valpha);
            vres_b = vmull_u8(vsrc.val[NEON_B], valpha);

            
            vres_a = SkDiv255Round_neon8(vres_a);
            vres_a = valpha_max - vres_a; 

            
            vres_r = vmlaq_u16(vres_r, vdst_r, vres_a);
            vres_g = vmlaq_u16(vres_g, vdst_g, vres_a);
            vres_b = vmlaq_u16(vres_b, vdst_b, vres_a);

#ifdef S32A_D565_BLEND_EXACT
            
            
            vres_r = SkDiv255Round_neon8(vres_r);
            vres_g = SkDiv255Round_neon8(vres_g);
            vres_b = SkDiv255Round_neon8(vres_b);
#else
            vres_r = vrshrq_n_u16(vres_r, 8);
            vres_g = vrshrq_n_u16(vres_g, 8);
            vres_b = vrshrq_n_u16(vres_b, 8);
#endif
            
            vres_b = vsliq_n_u16(vres_b, vres_g, SK_G16_SHIFT); 
            vres_b = vsliq_n_u16(vres_b, vres_r, SK_R16_SHIFT); 

            
            vst1q_u16(dst, vres_b);
            dst += 8;
            count -= 8;
        } while (count >= 8);
    }

    
    while (count-- > 0) {
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
    }
}






static const uint8_t gDitherMatrix_Neon[48] = {
    0, 4, 1, 5, 0, 4, 1, 5, 0, 4, 1, 5,
    6, 2, 7, 3, 6, 2, 7, 3, 6, 2, 7, 3,
    1, 5, 0, 4, 1, 5, 0, 4, 1, 5, 0, 4,
    7, 3, 6, 2, 7, 3, 6, 2, 7, 3, 6, 2,

};

void S32_D565_Blend_Dither_neon(uint16_t *dst, const SkPMColor *src,
                                int count, U8CPU alpha, int x, int y)
{

    SkASSERT(255 > alpha);

    
    int scale = SkAlpha255To256(alpha);

    if (count >= 8) {
        
        const uint8_t *dstart = &gDitherMatrix_Neon[(y&3)*12 + (x&3)];

        uint8x8_t vdither = vld1_u8(dstart);         
        uint8x8_t vdither_g = vshr_n_u8(vdither, 1); 

        int16x8_t vscale = vdupq_n_s16(scale);        
        uint16x8_t vmask_b = vdupq_n_u16(0x1F);         

        do {

            uint8x8x4_t vsrc;
            uint8x8_t vsrc_r, vsrc_g, vsrc_b;
            uint8x8_t vsrc565_r, vsrc565_g, vsrc565_b;
            uint16x8_t vsrc_dit_r, vsrc_dit_g, vsrc_dit_b;
            uint16x8_t vsrc_res_r, vsrc_res_g, vsrc_res_b;
            uint16x8_t vdst;
            uint16x8_t vdst_r, vdst_g, vdst_b;
            int16x8_t vres_r, vres_g, vres_b;
            int8x8_t vres8_r, vres8_g, vres8_b;

            
#ifdef SK_CPU_ARM64
            vsrc = sk_vld4_u8_arm64_3(src);
#else
            {
            register uint8x8_t d0 asm("d0");
            register uint8x8_t d1 asm("d1");
            register uint8x8_t d2 asm("d2");
            register uint8x8_t d3 asm("d3");

            asm (
                "vld4.8    {d0-d3},[%[src]]! "
                : "=w" (d0), "=w" (d1), "=w" (d2), "=w" (d3), [src] "+&r" (src)
                :
            );
            vsrc.val[0] = d0;
            vsrc.val[1] = d1;
            vsrc.val[2] = d2;
            }
#endif
            vsrc_r = vsrc.val[NEON_R];
            vsrc_g = vsrc.val[NEON_G];
            vsrc_b = vsrc.val[NEON_B];

            vsrc565_g = vshr_n_u8(vsrc_g, 6); 
            vsrc565_r = vshr_n_u8(vsrc_r, 5); 
            vsrc565_b = vshr_n_u8(vsrc_b, 5); 

            vsrc_dit_g = vaddl_u8(vsrc_g, vdither_g); 
            vsrc_dit_r = vaddl_u8(vsrc_r, vdither);   
            vsrc_dit_b = vaddl_u8(vsrc_b, vdither);   

            vsrc_dit_r = vsubw_u8(vsrc_dit_r, vsrc565_r);  
            vsrc_dit_g = vsubw_u8(vsrc_dit_g, vsrc565_g);  
            vsrc_dit_b = vsubw_u8(vsrc_dit_b, vsrc565_b);  

            vsrc_res_r = vshrq_n_u16(vsrc_dit_r, 3);
            vsrc_res_g = vshrq_n_u16(vsrc_dit_g, 2);
            vsrc_res_b = vshrq_n_u16(vsrc_dit_b, 3);

            
            vdst = vld1q_u16(dst);
            vdst_g = vshrq_n_u16(vdst, 5);                   
            vdst_r = vshrq_n_u16(vshlq_n_u16(vdst, 5), 5+5); 
            vdst_b = vandq_u16(vdst, vmask_b);               

            
            vres_r = vsubq_s16(vreinterpretq_s16_u16(vsrc_res_r), vreinterpretq_s16_u16(vdst_r));
            vres_g = vsubq_s16(vreinterpretq_s16_u16(vsrc_res_g), vreinterpretq_s16_u16(vdst_g));
            vres_b = vsubq_s16(vreinterpretq_s16_u16(vsrc_res_b), vreinterpretq_s16_u16(vdst_b));

            
            vres_r = vmulq_s16(vres_r, vscale);
            vres_g = vmulq_s16(vres_g, vscale);
            vres_b = vmulq_s16(vres_b, vscale);

            vres8_r = vshrn_n_s16(vres_r, 8);
            vres8_g = vshrn_n_s16(vres_g, 8);
            vres8_b = vshrn_n_s16(vres_b, 8);

            
            vres_r = vaddw_s8(vreinterpretq_s16_u16(vdst_r), vres8_r);
            vres_g = vaddw_s8(vreinterpretq_s16_u16(vdst_g), vres8_g);
            vres_b = vaddw_s8(vreinterpretq_s16_u16(vdst_b), vres8_b);

            
            vres_b = vsliq_n_s16(vres_b, vres_g, 5);   
            vres_b = vsliq_n_s16(vres_b, vres_r, 6+5); 

            
            vst1q_u16(dst, vreinterpretq_u16_s16(vres_b));

            
            dst += 8;
            count -= 8;

        } while (count >= 8);
    }

    
    if (count > 0) {
        int scale = SkAlpha255To256(alpha);
        DITHER_565_SCAN(y);
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);

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
        } while (--count != 0);
    }
}

void S32A_Opaque_BlitRow32_neon(SkPMColor* SK_RESTRICT dst,
                                const SkPMColor* SK_RESTRICT src,
                                int count, U8CPU alpha) {

    SkASSERT(255 == alpha);
    if (count > 0) {


    uint8x8_t alpha_mask;

    static const uint8_t alpha_mask_setup[] = {3,3,3,3,7,7,7,7};
    alpha_mask = vld1_u8(alpha_mask_setup);

    
#define    UNROLL    4
    while (count >= UNROLL) {
        uint8x8_t src_raw, dst_raw, dst_final;
        uint8x8_t src_raw_2, dst_raw_2, dst_final_2;

        



        __builtin_prefetch(src+32);
        __builtin_prefetch(dst+32);

        
        src_raw = vreinterpret_u8_u32(vld1_u32(src));
#if    UNROLL > 2
        src_raw_2 = vreinterpret_u8_u32(vld1_u32(src+2));
#endif

        
        dst_raw = vreinterpret_u8_u32(vld1_u32(dst));
#if    UNROLL > 2
        dst_raw_2 = vreinterpret_u8_u32(vld1_u32(dst+2));
#endif

    
    {
        uint8x8_t dst_cooked;
        uint16x8_t dst_wide;
        uint8x8_t alpha_narrow;
        uint16x8_t alpha_wide;

        
        alpha_narrow = vtbl1_u8(src_raw, alpha_mask);
        alpha_wide = vsubw_u8(vdupq_n_u16(256), alpha_narrow);

        
        dst_wide = vmovl_u8(dst_raw);

        
        dst_wide = vmulq_u16 (dst_wide, alpha_wide);
        dst_cooked = vshrn_n_u16(dst_wide, 8);

        
        dst_final = vadd_u8(src_raw, dst_cooked);
    }

#if    UNROLL > 2
    
    {
        uint8x8_t dst_cooked;
        uint16x8_t dst_wide;
        uint8x8_t alpha_narrow;
        uint16x8_t alpha_wide;

        alpha_narrow = vtbl1_u8(src_raw_2, alpha_mask);
        alpha_wide = vsubw_u8(vdupq_n_u16(256), alpha_narrow);

        
        dst_wide = vmovl_u8(dst_raw_2);

        
        dst_wide = vmulq_u16 (dst_wide, alpha_wide);
        dst_cooked = vshrn_n_u16(dst_wide, 8);

        
        dst_final_2 = vadd_u8(src_raw_2, dst_cooked);
    }
#endif

        vst1_u32(dst, vreinterpret_u32_u8(dst_final));
#if    UNROLL > 2
        vst1_u32(dst+2, vreinterpret_u32_u8(dst_final_2));
#endif

        src += UNROLL;
        dst += UNROLL;
        count -= UNROLL;
    }
#undef    UNROLL

    
        while (--count >= 0) {
            *dst = SkPMSrcOver(*src, *dst);
            src += 1;
            dst += 1;
        }
    }
}

void S32A_Opaque_BlitRow32_neon_src_alpha(SkPMColor* SK_RESTRICT dst,
                                const SkPMColor* SK_RESTRICT src,
                                int count, U8CPU alpha) {
    SkASSERT(255 == alpha);

    if (count <= 0)
    return;

    
    const unsigned int ALPHA_OPAQ  = 0xFF000000;
    const unsigned int ALPHA_TRANS = 0x00FFFFFF;

#define UNROLL  4
    const SkPMColor* SK_RESTRICT src_end = src + count - (UNROLL + 1);
    const SkPMColor* SK_RESTRICT src_temp = src;

    
    uint8x8_t alpha_mask;
    static const uint8_t alpha_mask_setup[] = {3,3,3,3,7,7,7,7};
    alpha_mask = vld1_u8(alpha_mask_setup);

    uint8x8_t src_raw, dst_raw, dst_final;
    uint8x8_t src_raw_2, dst_raw_2, dst_final_2;
    uint8x8_t dst_cooked;
    uint16x8_t dst_wide;
    uint8x8_t alpha_narrow;
    uint16x8_t alpha_wide;

    
    if( src >= src_end)
        goto TAIL;
    if(*src <= ALPHA_TRANS)
        goto ALPHA_0;
    if(*src >= ALPHA_OPAQ)
        goto ALPHA_255;
    

ALPHA_1_TO_254:
    do {

        
        src_raw = vreinterpret_u8_u32(vld1_u32(src));
        src_raw_2 = vreinterpret_u8_u32(vld1_u32(src+2));

        
        dst_raw = vreinterpret_u8_u32(vld1_u32(dst));
        dst_raw_2 = vreinterpret_u8_u32(vld1_u32(dst+2));


        
        alpha_narrow = vtbl1_u8(src_raw, alpha_mask);
        
        
        alpha_wide = vsubw_u8(vdupq_n_u16(256), alpha_narrow);

        
        dst_wide = vmovl_u8(dst_raw);

        
        dst_wide = vmulq_u16 (dst_wide, alpha_wide);
        dst_cooked = vshrn_n_u16(dst_wide, 8);

        
        dst_final = vadd_u8(src_raw, dst_cooked);

        alpha_narrow = vtbl1_u8(src_raw_2, alpha_mask);
        
        
        alpha_wide = vsubw_u8(vdupq_n_u16(256), alpha_narrow);

        
        dst_wide = vmovl_u8(dst_raw_2);

        
        dst_wide = vmulq_u16 (dst_wide, alpha_wide);
        dst_cooked = vshrn_n_u16(dst_wide, 8);

        
        dst_final_2 = vadd_u8(src_raw_2, dst_cooked);

        vst1_u32(dst, vreinterpret_u32_u8(dst_final));
        vst1_u32(dst+2, vreinterpret_u32_u8(dst_final_2));

        src += UNROLL;
        dst += UNROLL;

        

        if((src[0] <= ALPHA_TRANS && src[1] <= ALPHA_TRANS) || (src[0] >= ALPHA_OPAQ && src[1] >= ALPHA_OPAQ))
            break;

    } while(src < src_end);

    if (src >= src_end)
        goto TAIL;

    if(src[0] >= ALPHA_OPAQ && src[1] >= ALPHA_OPAQ)
        goto ALPHA_255;

    

ALPHA_0:

    

    src_temp = src;  
    do {
        if(*(++src) > ALPHA_TRANS)
            break;
        if(*(++src) > ALPHA_TRANS)
            break;
        if(*(++src) > ALPHA_TRANS)
            break;
        if(*(++src) > ALPHA_TRANS)
            break;
    } while(src < src_end);

    dst += (src - src_temp);

    
    if( src >= src_end)
        goto TAIL;
    if(*src >= ALPHA_OPAQ)
        goto ALPHA_255;
    else
        goto ALPHA_1_TO_254;

ALPHA_255:
    while((src[0] & src[1] & src[2] & src[3]) >= ALPHA_OPAQ) {
        dst[0]=src[0];
        dst[1]=src[1];
        dst[2]=src[2];
        dst[3]=src[3];
        src+=UNROLL;
        dst+=UNROLL;
        if(src >= src_end)
            goto TAIL;
    }

    
    if(*src >= ALPHA_OPAQ) { *dst++ = *src++;
        if(*src >= ALPHA_OPAQ) { *dst++ = *src++;
            if(*src >= ALPHA_OPAQ) { *dst++ = *src++; }
        }
    }

    if( src >= src_end)
        goto TAIL;
    if(*src <= ALPHA_TRANS)
        goto ALPHA_0;
    else
        goto ALPHA_1_TO_254;

TAIL:
    
    src_end += UNROLL + 1;  
    while(src != src_end) {
        if( *src != 0 ) {
            if( *src >= ALPHA_OPAQ ) {
                *dst = *src;
            }
            else {
                *dst = SkPMSrcOver(*src, *dst);
            }
        }
        src++;
        dst++;
    }

#undef    UNROLL
    return;
}




void S32_Blend_BlitRow32_neon(SkPMColor* SK_RESTRICT dst,
                              const SkPMColor* SK_RESTRICT src,
                              int count, U8CPU alpha) {
    SkASSERT(alpha <= 255);

    if (count <= 0) {
        return;
    }

    uint16_t src_scale = SkAlpha255To256(alpha);
    uint16_t dst_scale = 256 - src_scale;

    while (count >= 2) {
        uint8x8_t vsrc, vdst, vres;
        uint16x8_t vsrc_wide, vdst_wide;

        



        
        

        
        vsrc = vreinterpret_u8_u32(vld1_u32(src));
        vdst = vreinterpret_u8_u32(vld1_u32(dst));

        
        vsrc_wide = vmovl_u8(vsrc);
        vsrc_wide = vmulq_u16(vsrc_wide, vdupq_n_u16(src_scale));

        
        vdst_wide = vmull_u8(vdst, vdup_n_u8(dst_scale));

        
        vres = vshrn_n_u16(vdst_wide, 8) + vshrn_n_u16(vsrc_wide, 8);

        
        vst1_u32(dst, vreinterpret_u32_u8(vres));

        src += 2;
        dst += 2;
        count -= 2;
    }

    if (count == 1) {
        uint8x8_t vsrc = vdup_n_u8(0), vdst = vdup_n_u8(0), vres;
        uint16x8_t vsrc_wide, vdst_wide;

        
        vsrc = vreinterpret_u8_u32(vld1_lane_u32(src, vreinterpret_u32_u8(vsrc), 0));
        vdst = vreinterpret_u8_u32(vld1_lane_u32(dst, vreinterpret_u32_u8(vdst), 0));

        
        vsrc_wide = vmovl_u8(vsrc);
        vsrc_wide = vmulq_u16(vsrc_wide, vdupq_n_u16(src_scale));
        vdst_wide = vmull_u8(vdst, vdup_n_u8(dst_scale));
        vres = vshrn_n_u16(vdst_wide, 8) + vshrn_n_u16(vsrc_wide, 8);

        
        vst1_lane_u32(dst, vreinterpret_u32_u8(vres), 0);
    }
}

#ifdef SK_CPU_ARM32
void S32A_Blend_BlitRow32_neon(SkPMColor* SK_RESTRICT dst,
                         const SkPMColor* SK_RESTRICT src,
                         int count, U8CPU alpha) {

    SkASSERT(255 >= alpha);

    if (count <= 0) {
        return;
    }

    unsigned alpha256 = SkAlpha255To256(alpha);

    
    if (count & 1) {
        uint8x8_t vsrc = vdup_n_u8(0), vdst = vdup_n_u8(0), vres;
        uint16x8_t vdst_wide, vsrc_wide;
        unsigned dst_scale;

        
        vsrc = vreinterpret_u8_u32(vld1_lane_u32(src, vreinterpret_u32_u8(vsrc), 0));
        vdst = vreinterpret_u8_u32(vld1_lane_u32(dst, vreinterpret_u32_u8(vdst), 0));

        
        dst_scale = vget_lane_u8(vsrc, 3);
        dst_scale *= alpha256;
        dst_scale >>= 8;
        dst_scale = 256 - dst_scale;

        
        vsrc_wide = vmovl_u8(vsrc);
        vsrc_wide = vmulq_n_u16(vsrc_wide, alpha256);

        
        vdst_wide = vmovl_u8(vdst);
        vdst_wide = vmulq_n_u16(vdst_wide, dst_scale);

        
        vres = vshrn_n_u16(vdst_wide, 8) + vshrn_n_u16(vsrc_wide, 8);

        vst1_lane_u32(dst, vreinterpret_u32_u8(vres), 0);
        dst++;
        src++;
        count--;
    }

    if (count) {
        uint8x8_t alpha_mask;
        static const uint8_t alpha_mask_setup[] = {3,3,3,3,7,7,7,7};
        alpha_mask = vld1_u8(alpha_mask_setup);

        do {

            uint8x8_t vsrc, vdst, vres, vsrc_alphas;
            uint16x8_t vdst_wide, vsrc_wide, vsrc_scale, vdst_scale;

            __builtin_prefetch(src+32);
            __builtin_prefetch(dst+32);

            
            vsrc = vreinterpret_u8_u32(vld1_u32(src));
            vdst = vreinterpret_u8_u32(vld1_u32(dst));

            
            vsrc_scale = vdupq_n_u16(alpha256);

            
            vsrc_alphas = vtbl1_u8(vsrc, alpha_mask);
            vdst_scale = vmovl_u8(vsrc_alphas);
            vdst_scale *= vsrc_scale;
            vdst_scale = vshrq_n_u16(vdst_scale, 8);
            vdst_scale = vsubq_u16(vdupq_n_u16(256), vdst_scale);

            
            vsrc_wide = vmovl_u8(vsrc);
            vsrc_wide *= vsrc_scale;

            
            vdst_wide = vmovl_u8(vdst);
            vdst_wide *= vdst_scale;

            
            vres = vshrn_n_u16(vdst_wide, 8) + vshrn_n_u16(vsrc_wide, 8);

            vst1_u32(dst, vreinterpret_u32_u8(vres));

            src += 2;
            dst += 2;
            count -= 2;
        } while(count);
    }
}



#undef    DEBUG_OPAQUE_DITHER

#if    defined(DEBUG_OPAQUE_DITHER)
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
#endif 

void S32A_D565_Opaque_Dither_neon (uint16_t * SK_RESTRICT dst,
                                   const SkPMColor* SK_RESTRICT src,
                                   int count, U8CPU alpha, int x, int y) {
    SkASSERT(255 == alpha);

#define    UNROLL    8

    if (count >= UNROLL) {

#if defined(DEBUG_OPAQUE_DITHER)
    uint16_t tmpbuf[UNROLL];
    int td[UNROLL];
    int tdv[UNROLL];
    int ta[UNROLL];
    int tap[UNROLL];
    uint16_t in_dst[UNROLL];
    int offset = 0;
    int noisy = 0;
#endif

    uint8x8_t dbase;
    const uint8_t *dstart = &gDitherMatrix_Neon[(y&3)*12 + (x&3)];
    dbase = vld1_u8(dstart);

        do {
        uint8x8x4_t vsrc;
        uint8x8_t sr, sg, sb, sa, d;
        uint16x8_t dst8, scale8, alpha8;
        uint16x8_t dst_r, dst_g, dst_b;

#if defined(DEBUG_OPAQUE_DITHER)
        
        {
        int my_y = y;
        int my_x = x;
        SkPMColor* my_src = (SkPMColor*)src;
        uint16_t* my_dst = dst;
        int i;

        DITHER_565_SCAN(my_y);
        for(i = 0; i < UNROLL; i++) {
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

#ifdef SK_CPU_ARM64
        vsrc = sk_vld4_u8_arm64_4(src);
#else
        {
        register uint8x8_t d0 asm("d0");
        register uint8x8_t d1 asm("d1");
        register uint8x8_t d2 asm("d2");
        register uint8x8_t d3 asm("d3");

        asm ("vld4.8    {d0-d3},[%[src]]! "
            : "=w" (d0), "=w" (d1), "=w" (d2), "=w" (d3), [src] "+r" (src)
            :
        );
        vsrc.val[0] = d0;
        vsrc.val[1] = d1;
        vsrc.val[2] = d2;
        vsrc.val[3] = d3;
        }
#endif
        sa = vsrc.val[NEON_A];
        sr = vsrc.val[NEON_R];
        sg = vsrc.val[NEON_G];
        sb = vsrc.val[NEON_B];

        


        alpha8 = vmovl_u8(dbase);
        alpha8 = vmlal_u8(alpha8, sa, dbase);
        d = vshrn_n_u16(alpha8, 8);    

        
        



        sr = vsub_u8(sr, vshr_n_u8(sr, 5));
        sr = vadd_u8(sr, d);

        
        sb = vsub_u8(sb, vshr_n_u8(sb, 5));
        sb = vadd_u8(sb, d);

        
        sg = vsub_u8(sg, vshr_n_u8(sg, 6));
        sg = vadd_u8(sg, vshr_n_u8(d,1));

        
        dst8 = vld1q_u16(dst);
        dst_b = vandq_u16(dst8, vdupq_n_u16(SK_B16_MASK));
        dst_g = vshrq_n_u16(vshlq_n_u16(dst8, SK_R16_BITS), SK_R16_BITS + SK_B16_BITS);
        dst_r = vshrq_n_u16(dst8, SK_R16_SHIFT);    

        
        scale8 = vsubw_u8(vdupq_n_u16(256), sa);

        
        scale8 = vshrq_n_u16(scale8, 3);
        dst_b = vmlaq_u16(vshll_n_u8(sb,2), dst_b, scale8);
        dst_g = vmlaq_u16(vshll_n_u8(sg,3), dst_g, scale8);
        dst_r = vmlaq_u16(vshll_n_u8(sr,2), dst_r, scale8);

        
        dst8 = vshrq_n_u16(dst_b, 5);
        dst8 = vsliq_n_u16(dst8, vshrq_n_u16(dst_g, 5), 5);
        dst8 = vsliq_n_u16(dst8, vshrq_n_u16(dst_r,5), 11);

        vst1q_u16(dst, dst8);

#if defined(DEBUG_OPAQUE_DITHER)
        
        {
        int i, bad=0;
        static int invocation;

        for (i = 0; i < UNROLL; i++) {
            if (tmpbuf[i] != dst[i]) {
                bad=1;
            }
        }
        if (bad) {
            SkDebugf("BAD S32A_D565_Opaque_Dither_neon(); invocation %d offset %d\n",
                     invocation, offset);
            SkDebugf("  alpha 0x%x\n", alpha);
            for (i = 0; i < UNROLL; i++)
                SkDebugf("%2d: %s %04x w %04x id %04x s %08x d %04x %04x %04x %04x\n",
                         i, ((tmpbuf[i] != dst[i])?"BAD":"got"), dst[i], tmpbuf[i],
                         in_dst[i], src[i-8], td[i], tdv[i], tap[i], ta[i]);

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
        count -= UNROLL;
        
        } while (count >= UNROLL);
    }
#undef    UNROLL

    
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



#undef    DEBUG_S32_OPAQUE_DITHER

void S32_D565_Opaque_Dither_neon(uint16_t* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src,
                                 int count, U8CPU alpha, int x, int y) {
    SkASSERT(255 == alpha);

#define    UNROLL    8
    if (count >= UNROLL) {
    uint8x8_t d;
    const uint8_t *dstart = &gDitherMatrix_Neon[(y&3)*12 + (x&3)];
    d = vld1_u8(dstart);

    while (count >= UNROLL) {
        uint8x8_t sr, sg, sb;
        uint16x8_t dr, dg, db;
        uint16x8_t dst8;
        uint8x8x4_t vsrc;

#ifdef SK_CPU_ARM64
        vsrc = sk_vld4_u8_arm64_3(src);
#else
        {
        register uint8x8_t d0 asm("d0");
        register uint8x8_t d1 asm("d1");
        register uint8x8_t d2 asm("d2");
        register uint8x8_t d3 asm("d3");

        asm (
            "vld4.8    {d0-d3},[%[src]]! "
            : "=w" (d0), "=w" (d1), "=w" (d2), "=w" (d3), [src] "+&r" (src)
            :
        );
        vsrc.val[0] = d0;
        vsrc.val[1] = d1;
        vsrc.val[2] = d2;
        }
#endif
        sr = vsrc.val[NEON_R];
        sg = vsrc.val[NEON_G];
        sb = vsrc.val[NEON_B];

        





        
        sr = vsub_u8(sr, vshr_n_u8(sr, 5));
        dr = vaddl_u8(sr, d);

        
        sb = vsub_u8(sb, vshr_n_u8(sb, 5));
        db = vaddl_u8(sb, d);

        
        sg = vsub_u8(sg, vshr_n_u8(sg, 6));
        dg = vaddl_u8(sg, vshr_n_u8(d, 1));

        
        dst8 = vshrq_n_u16(db, 3);
        dst8 = vsliq_n_u16(dst8, vshrq_n_u16(dg, 2), 5);
        dst8 = vsliq_n_u16(dst8, vshrq_n_u16(dr, 3), 11);

        
        vst1q_u16(dst, dst8);

#if    defined(DEBUG_S32_OPAQUE_DITHER)
        
        {
        int i, myx = x, myy = y;
        DITHER_565_SCAN(myy);
        for (i=0;i<UNROLL;i++) {
            
            SkPMColor c = src[i-8];
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
        
        count -= UNROLL;
        x += UNROLL;        
    }
    }
#undef    UNROLL

    
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

void Color32_arm_neon(SkPMColor* dst, const SkPMColor* src, int count,
                      SkPMColor color) {
    if (count <= 0) {
        return;
    }

    if (0 == color) {
        if (src != dst) {
            memcpy(dst, src, count * sizeof(SkPMColor));
        }
        return;
    }

    unsigned colorA = SkGetPackedA32(color);
    if (255 == colorA) {
        sk_memset32(dst, color, count);
        return;
    }

    unsigned scale = 256 - SkAlpha255To256(colorA);

    if (count >= 8) {
        uint32x4_t vcolor;
        uint8x8_t vscale;

        vcolor = vdupq_n_u32(color);

        
        vscale = vdup_n_u8(scale);

        do {
            
            
            uint32x2x4_t vsrc;
#if defined(SK_CPU_ARM32) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 6)))
            asm (
                "vld1.32    %h[vsrc], [%[src]]!"
                : [vsrc] "=w" (vsrc), [src] "+r" (src)
                : :
            );
#else 
            vsrc.val[0] = vld1_u32(src);
            vsrc.val[1] = vld1_u32(src+2);
            vsrc.val[2] = vld1_u32(src+4);
            vsrc.val[3] = vld1_u32(src+6);
            src += 8;
#endif

            
            
            uint16x8x4_t vtmp;
            vtmp.val[0] = vmull_u8(vreinterpret_u8_u32(vsrc.val[0]), vscale);
            vtmp.val[1] = vmull_u8(vreinterpret_u8_u32(vsrc.val[1]), vscale);
            vtmp.val[2] = vmull_u8(vreinterpret_u8_u32(vsrc.val[2]), vscale);
            vtmp.val[3] = vmull_u8(vreinterpret_u8_u32(vsrc.val[3]), vscale);

            
            
            
            uint8x16x2_t vres;
            vres.val[0] = vcombine_u8(
                            vshrn_n_u16(vtmp.val[0], 8),
                            vshrn_n_u16(vtmp.val[1], 8));
            vres.val[1] = vcombine_u8(
                            vshrn_n_u16(vtmp.val[2], 8),
                            vshrn_n_u16(vtmp.val[3], 8));

            
            uint32x4x2_t vdst;
            vdst.val[0] = vreinterpretq_u32_u8(vres.val[0] +
                                               vreinterpretq_u8_u32(vcolor));
            vdst.val[1] = vreinterpretq_u32_u8(vres.val[1] +
                                               vreinterpretq_u8_u32(vcolor));

            
            
#if defined(SK_CPU_ARM32) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 6)))
            asm (
                "vst1.32    %h[vdst], [%[dst]]!"
                : [dst] "+r" (dst)
                : [vdst] "w" (vdst)
                : "memory"
            );
#else 
            vst1q_u32(dst, vdst.val[0]);
            vst1q_u32(dst+4, vdst.val[1]);
            dst += 8;
#endif
            count -= 8;

        } while (count >= 8);
    }

    while (count > 0) {
        *dst = color + SkAlphaMulQ(*src, scale);
        src += 1;
        dst += 1;
        count--;
    }
}



const SkBlitRow::Proc sk_blitrow_platform_565_procs_arm_neon[] = {
    
    S32_D565_Opaque_neon,
    S32_D565_Blend_neon,
    S32A_D565_Opaque_neon,
    S32A_D565_Blend_neon,

    
    S32_D565_Opaque_Dither_neon,
    S32_D565_Blend_Dither_neon,
    S32A_D565_Opaque_Dither_neon,
    NULL,   
};

const SkBlitRow::Proc32 sk_blitrow_platform_32_procs_arm_neon[] = {
    NULL,   
    S32_Blend_BlitRow32_neon,        
    








#if SK_A32_SHIFT == 24
    
    S32A_Opaque_BlitRow32_neon_src_alpha,   
#else
    S32A_Opaque_BlitRow32_neon,     
#endif
#ifdef SK_CPU_ARM32
    S32A_Blend_BlitRow32_neon        
#else
    NULL
#endif
};
