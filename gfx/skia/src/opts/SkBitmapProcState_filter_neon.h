








#include "SkColorPriv.h"










static inline void Filter_32_opaque_neon(unsigned x, unsigned y,
                                         SkPMColor a00, SkPMColor a01,
                                         SkPMColor a10, SkPMColor a11,
                                         SkPMColor *dst) {
    asm volatile(
                 "vdup.8         d0, %[y]                \n\t"   
                 "vmov.u8        d16, #16                \n\t"   
                 "vsub.u8        d1, d16, d0             \n\t"   

                 "vdup.32        d4, %[a00]              \n\t"   
                 "vdup.32        d5, %[a10]              \n\t"   
                 "vmov.32        d4[1], %[a01]           \n\t"   
                 "vmov.32        d5[1], %[a11]           \n\t"   

                 "vmull.u8       q3, d4, d1              \n\t"   
                 "vmull.u8       q0, d5, d0              \n\t"   

                 "vdup.16        d5, %[x]                \n\t"   
                 "vmov.u16       d16, #16                \n\t"   
                 "vsub.u16       d3, d16, d5             \n\t"   

                 "vmul.i16       d4, d7, d5              \n\t"   
                 "vmla.i16       d4, d1, d5              \n\t"   
                 "vmla.i16       d4, d6, d3              \n\t"   
                 "vmla.i16       d4, d0, d3              \n\t"   
                 "vshrn.i16      d0, q2, #8              \n\t"   
                 "vst1.32        {d0[0]}, [%[dst]]       \n\t"   
                 :
                 : [x] "r" (x), [y] "r" (y), [a00] "r" (a00), [a01] "r" (a01), [a10] "r" (a10), [a11] "r" (a11), [dst] "r" (dst)
                 : "cc", "memory", "r4", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d16"
                 );
}

static inline void Filter_32_alpha_neon(unsigned x, unsigned y,
                                        SkPMColor a00, SkPMColor a01,
                                        SkPMColor a10, SkPMColor a11,
                                        SkPMColor *dst, uint16_t scale) {
    asm volatile(
                 "vdup.8         d0, %[y]                \n\t"   
                 "vmov.u8        d16, #16                \n\t"   
                 "vsub.u8        d1, d16, d0             \n\t"   

                 "vdup.32        d4, %[a00]              \n\t"   
                 "vdup.32        d5, %[a10]              \n\t"   
                 "vmov.32        d4[1], %[a01]           \n\t"   
                 "vmov.32        d5[1], %[a11]           \n\t"   

                 "vmull.u8       q3, d4, d1              \n\t"   
                 "vmull.u8       q0, d5, d0              \n\t"   

                 "vdup.16        d5, %[x]                \n\t"   
                 "vmov.u16       d16, #16                \n\t"   
                 "vsub.u16       d3, d16, d5             \n\t"   

                 "vmul.i16       d4, d7, d5              \n\t"   
                 "vmla.i16       d4, d1, d5              \n\t"   
                 "vmla.i16       d4, d6, d3              \n\t"   
                 "vmla.i16       d4, d0, d3              \n\t"   
                 "vdup.16        d3, %[scale]            \n\t"   
                 "vshr.u16       d4, d4, #8              \n\t"   
                 "vmul.i16       d4, d4, d3              \n\t"   
                 "vshrn.i16      d0, q2, #8              \n\t"   
                 "vst1.32        {d0[0]}, [%[dst]]       \n\t"   
                 :
                 : [x] "r" (x), [y] "r" (y), [a00] "r" (a00), [a01] "r" (a01), [a10] "r" (a10), [a11] "r" (a11), [dst] "r" (dst), [scale] "r" (scale)
                 : "cc", "memory", "r4", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d16"
                 );
}
