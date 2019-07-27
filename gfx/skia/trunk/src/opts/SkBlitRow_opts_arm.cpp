






#include "SkBlitRow.h"
#include "SkColorPriv.h"
#include "SkDither.h"
#include "SkMathPriv.h"
#include "SkUtils.h"
#include "SkUtilsArm.h"

#include "SkCachePreload_arm.h"


#define USE_NEON_CODE  (!SK_ARM_NEON_IS_NONE)


#define USE_ARM_CODE   (!SK_ARM_NEON_IS_ALWAYS)

#if USE_NEON_CODE
  #include "SkBlitRow_opts_arm_neon.h"
#endif

#if USE_ARM_CODE

static void S32A_D565_Opaque(uint16_t* SK_RESTRICT dst,
                             const SkPMColor* SK_RESTRICT src, int count,
                             U8CPU alpha, int , int ) {
    SkASSERT(255 == alpha);

    asm volatile (
                  "1:                                   \n\t"
                  "ldr     r3, [%[src]], #4             \n\t"
                  "cmp     r3, #0xff000000              \n\t"
                  "blo     2f                           \n\t"
                  "and     r4, r3, #0x0000f8            \n\t"
                  "and     r5, r3, #0x00fc00            \n\t"
                  "and     r6, r3, #0xf80000            \n\t"
#ifdef SK_ARM_HAS_EDSP
                  "pld     [r1, #32]                    \n\t"
#endif
                  "lsl     r3, r4, #8                   \n\t"
                  "orr     r3, r3, r5, lsr #5           \n\t"
                  "orr     r3, r3, r6, lsr #19          \n\t"
                  "subs    %[count], %[count], #1       \n\t"
                  "strh    r3, [%[dst]], #2             \n\t"
                  "bne     1b                           \n\t"
                  "b       4f                           \n\t"
                  "2:                                   \n\t"
                  "lsrs    r7, r3, #24                  \n\t"
                  "beq     3f                           \n\t"
                  "ldrh    r4, [%[dst]]                 \n\t"
                  "rsb     r7, r7, #255                 \n\t"
                  "and     r6, r4, #0x001f              \n\t"
#if SK_ARM_ARCH <= 6
                  "lsl     r5, r4, #21                  \n\t"
                  "lsr     r5, r5, #26                  \n\t"
#else
                  "ubfx    r5, r4, #5, #6               \n\t"
#endif
#ifdef SK_ARM_HAS_EDSP
                  "pld     [r0, #16]                    \n\t"
#endif
                  "lsr     r4, r4, #11                  \n\t"
#ifdef SK_ARM_HAS_EDSP
                  "smulbb  r6, r6, r7                   \n\t"
                  "smulbb  r5, r5, r7                   \n\t"
                  "smulbb  r4, r4, r7                   \n\t"
#else
                  "mul     r6, r6, r7                   \n\t"
                  "mul     r5, r5, r7                   \n\t"
                  "mul     r4, r4, r7                   \n\t"
#endif
#if SK_ARM_ARCH >= 6
                  "uxtb    r7, r3, ROR #16              \n\t"
                  "uxtb    ip, r3, ROR #8               \n\t"
#else
                  "mov     ip, #0xff                    \n\t"
                  "and     r7, ip, r3, ROR #16          \n\t"
                  "and     ip, ip, r3, ROR #8           \n\t"
#endif
                  "and     r3, r3, #0xff                \n\t"
                  "add     r6, r6, #16                  \n\t"
                  "add     r5, r5, #32                  \n\t"
                  "add     r4, r4, #16                  \n\t"
                  "add     r6, r6, r6, lsr #5           \n\t"
                  "add     r5, r5, r5, lsr #6           \n\t"
                  "add     r4, r4, r4, lsr #5           \n\t"
                  "add     r6, r7, r6, lsr #5           \n\t"
                  "add     r5, ip, r5, lsr #6           \n\t"
                  "add     r4, r3, r4, lsr #5           \n\t"
                  "lsr     r6, r6, #3                   \n\t"
                  "and     r5, r5, #0xfc                \n\t"
                  "and     r4, r4, #0xf8                \n\t"
                  "orr     r6, r6, r5, lsl #3           \n\t"
                  "orr     r4, r6, r4, lsl #8           \n\t"
                  "strh    r4, [%[dst]], #2             \n\t"
#ifdef SK_ARM_HAS_EDSP
                  "pld     [r1, #32]                    \n\t"
#endif
                  "subs    %[count], %[count], #1       \n\t"
                  "bne     1b                           \n\t"
                  "b       4f                           \n\t"
                  "3:                                   \n\t"
                  "subs    %[count], %[count], #1       \n\t"
                  "add     %[dst], %[dst], #2           \n\t"
                  "bne     1b                           \n\t"
                  "4:                                   \n\t"
                  : [dst] "+r" (dst), [src] "+r" (src), [count] "+r" (count)
                  :
                  : "memory", "cc", "r3", "r4", "r5", "r6", "r7", "ip"
                  );
}

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




void S32A_Blend_BlitRow32_arm(SkPMColor* SK_RESTRICT dst,
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
#ifdef SK_ARM_HAS_EDSP
                  "smulbb r9, r9, %[alpha]           \n\t" 
                  "smulbb r10, r10, %[alpha]         \n\t" 
#else
                  "mul    r9, r9, %[alpha]           \n\t" 
                  "mul    r10, r10, %[alpha]         \n\t" 
#endif
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
#ifdef SK_ARM_HAS_EDSP
                  "smulbb r6, r6, %[alpha]           \n\t" 
#else
                  "mul    r6, r6, %[alpha]           \n\t" 
#endif
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



static const SkBlitRow::Proc sk_blitrow_platform_565_procs_arm[] = {
    
    
    
    
    S32A_D565_Opaque,   
    NULL,               
    S32A_D565_Opaque,   
    NULL,               

    
    NULL,   
    NULL,   
    NULL,   
    NULL,   
};

static const SkBlitRow::Proc32 sk_blitrow_platform_32_procs_arm[] = {
    NULL,   
    NULL,   
    S32A_Opaque_BlitRow32_arm,   
    S32A_Blend_BlitRow32_arm     
};

#endif 

SkBlitRow::Proc SkBlitRow::PlatformProcs565(unsigned flags) {
    return SK_ARM_NEON_WRAP(sk_blitrow_platform_565_procs_arm)[flags];
}

SkBlitRow::Proc32 SkBlitRow::PlatformProcs32(unsigned flags) {
    return SK_ARM_NEON_WRAP(sk_blitrow_platform_32_procs_arm)[flags];
}


#define Color32_arm  NULL
SkBlitRow::ColorProc SkBlitRow::PlatformColorProc() {
    return SK_ARM_NEON_WRAP(Color32_arm);
}

SkBlitRow::ColorRectProc PlatformColorRectProcFactory() {
    return NULL;
}
