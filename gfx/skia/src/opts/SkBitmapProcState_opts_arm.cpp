








#ifdef ANDROID
    #include <machine/cpu-features.h>
#endif

#include "SkBitmapProcState.h"
#include "SkColorPriv.h"
#include "SkUtils.h"

#if __ARM_ARCH__ >= 6 && !defined(SK_CPU_BENDIAN)
void SI8_D16_nofilter_DX_arm(
    const SkBitmapProcState& s,
    const uint32_t* SK_RESTRICT xy,
    int count,
    uint16_t* SK_RESTRICT colors) __attribute__((optimize("O1")));

void SI8_D16_nofilter_DX_arm(const SkBitmapProcState& s,
                             const uint32_t* SK_RESTRICT xy,
                             int count, uint16_t* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask));
    SkASSERT(s.fDoFilter == false);
    
    const uint16_t* SK_RESTRICT table = s.fBitmap->getColorTable()->lock16BitCache();
    const uint8_t* SK_RESTRICT srcAddr = (const uint8_t*)s.fBitmap->getPixels();
    
    
    
    SkASSERT((unsigned)xy[0] < (unsigned)s.fBitmap->height());
    srcAddr = (const uint8_t*)((const char*)srcAddr +
                               xy[0] * s.fBitmap->rowBytes());
    
    uint8_t src;
    
    if (1 == s.fBitmap->width()) {
        src = srcAddr[0];
        uint16_t dstValue = table[src];
        sk_memset16(colors, dstValue, count);
    } else {
        int i;
        int count8 = count >> 3;
        const uint16_t* SK_RESTRICT xx = (const uint16_t*)(xy + 1);
        
        asm volatile (
                      "cmp        %[count8], #0                   \n\t"   
                      "beq        2f                              \n\t"   
                      "1:                                             \n\t"
                      "ldmia      %[xx]!, {r5, r7, r9, r11}       \n\t"   
                      "subs       %[count8], %[count8], #1        \n\t"   
                      "uxth       r4, r5                          \n\t"   
                      "mov        r5, r5, lsr #16                 \n\t"   
                      "uxth       r6, r7                          \n\t"   
                      "mov        r7, r7, lsr #16                 \n\t"   
                      "ldrb       r4, [%[srcAddr], r4]            \n\t"   
                      "uxth       r8, r9                          \n\t"   
                      "ldrb       r5, [%[srcAddr], r5]            \n\t"   
                      "mov        r9, r9, lsr #16                 \n\t"   
                      "ldrb       r6, [%[srcAddr], r6]            \n\t"   
                      "uxth       r10, r11                        \n\t"   
                      "ldrb       r7, [%[srcAddr], r7]            \n\t"   
                      "mov        r11, r11, lsr #16               \n\t"   
                      "ldrb       r8, [%[srcAddr], r8]            \n\t"   
                      "add        r4, r4, r4                      \n\t"   
                      "ldrb       r9, [%[srcAddr], r9]            \n\t"   
                      "add        r5, r5, r5                      \n\t"   
                      "ldrb       r10, [%[srcAddr], r10]          \n\t"   
                      "add        r6, r6, r6                      \n\t"   
                      "ldrb       r11, [%[srcAddr], r11]          \n\t"   
                      "add        r7, r7, r7                      \n\t"   
                      "ldrh       r4, [%[table], r4]              \n\t"   
                      "add        r8, r8, r8                      \n\t"   
                      "ldrh       r5, [%[table], r5]              \n\t"   
                      "add        r9, r9, r9                      \n\t"   
                      "ldrh       r6, [%[table], r6]              \n\t"   
                      "add        r10, r10, r10                   \n\t"   
                      "ldrh       r7, [%[table], r7]              \n\t"   
                      "add        r11, r11, r11                   \n\t"   
                      "ldrh       r8, [%[table], r8]              \n\t"   
                      "ldrh       r9, [%[table], r9]              \n\t"   
                      "ldrh       r10, [%[table], r10]            \n\t"   
                      "ldrh       r11, [%[table], r11]            \n\t"   
                      "pkhbt      r5, r4, r5, lsl #16             \n\t"   
                      "pkhbt      r6, r6, r7, lsl #16             \n\t"   
                      "pkhbt      r8, r8, r9, lsl #16             \n\t"   
                      "pkhbt      r10, r10, r11, lsl #16          \n\t"   
                      "stmia      %[colors]!, {r5, r6, r8, r10}   \n\t"   
                      "bgt        1b                              \n\t"   
                      "2:                                             \n\t"
                      : [xx] "+r" (xx), [count8] "+r" (count8), [colors] "+r" (colors)
                      : [table] "r" (table), [srcAddr] "r" (srcAddr)
                      : "memory", "cc", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11"
                      );
        
        for (i = (count & 7); i > 0; --i) {
            src = srcAddr[*xx++]; *colors++ = table[src];
        }
    }

    s.fBitmap->getColorTable()->unlock16BitCache(); 
}

void SI8_opaque_D32_nofilter_DX_arm(
    const SkBitmapProcState& s,
    const uint32_t* SK_RESTRICT xy,
    int count,
    SkPMColor* SK_RESTRICT colors) __attribute__((optimize("O1")));

void SI8_opaque_D32_nofilter_DX_arm(const SkBitmapProcState& s,
                                    const uint32_t* SK_RESTRICT xy,
                                    int count, SkPMColor* SK_RESTRICT colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask));
    SkASSERT(s.fDoFilter == false);

    const SkPMColor* SK_RESTRICT table = s.fBitmap->getColorTable()->lockColors();
    const uint8_t* SK_RESTRICT srcAddr = (const uint8_t*)s.fBitmap->getPixels();

    
    
    SkASSERT((unsigned)xy[0] < (unsigned)s.fBitmap->height());
    srcAddr = (const uint8_t*)((const char*)srcAddr + xy[0] * s.fBitmap->rowBytes());

    if (1 == s.fBitmap->width()) {
        uint8_t src = srcAddr[0];
        SkPMColor dstValue = table[src];
        sk_memset32(colors, dstValue, count);
    } else {
        const uint16_t* xx = (const uint16_t*)(xy + 1);

        asm volatile (
                      "subs       %[count], %[count], #8          \n\t"   
                      "blt        2f                              \n\t"   
                      "1:                                             \n\t"   
                      "ldmia      %[xx]!, {r5, r7, r9, r11}       \n\t"   
                      "uxth       r4, r5                          \n\t"   
                      "mov        r5, r5, lsr #16                 \n\t"   
                      "uxth       r6, r7                          \n\t"   
                      "mov        r7, r7, lsr #16                 \n\t"   
                      "ldrb       r4, [%[srcAddr], r4]            \n\t"   
                      "uxth       r8, r9                          \n\t"   
                      "ldrb       r5, [%[srcAddr], r5]            \n\t"   
                      "mov        r9, r9, lsr #16                 \n\t"   
                      "ldrb       r6, [%[srcAddr], r6]            \n\t"   
                      "uxth       r10, r11                        \n\t"   
                      "ldrb       r7, [%[srcAddr], r7]            \n\t"   
                      "mov        r11, r11, lsr #16               \n\t"   
                      "ldrb       r8, [%[srcAddr], r8]            \n\t"   
                      "ldrb       r9, [%[srcAddr], r9]            \n\t"   
                      "ldrb       r10, [%[srcAddr], r10]          \n\t"   
                      "ldrb       r11, [%[srcAddr], r11]          \n\t"   
                      "ldr        r4, [%[table], r4, lsl #2]      \n\t"   
                      "ldr        r5, [%[table], r5, lsl #2]      \n\t"   
                      "ldr        r6, [%[table], r6, lsl #2]      \n\t"   
                      "ldr        r7, [%[table], r7, lsl #2]      \n\t"   
                      "ldr        r8, [%[table], r8, lsl #2]      \n\t"   
                      "ldr        r9, [%[table], r9, lsl #2]      \n\t"   
                      "ldr        r10, [%[table], r10, lsl #2]    \n\t"   
                      "ldr        r11, [%[table], r11, lsl #2]    \n\t"   
                      "subs       %[count], %[count], #8          \n\t"   
                      "stmia      %[colors]!, {r4-r11}            \n\t"   
                      "bge        1b                              \n\t"   
                      "2:                                             \n\t"
                      "adds       %[count], %[count], #8          \n\t"   
                      "beq        4f                              \n\t"   
                      "3:                                             \n\t"   
                      "ldrh       r4, [%[xx]], #2                 \n\t"   
                      "subs       %[count], %[count], #1          \n\t"   
                      "ldrb       r5, [%[srcAddr], r4]            \n\t"   
                      "ldr        r6, [%[table], r5, lsl #2]      \n\t"   
                      "str        r6, [%[colors]], #4             \n\t"   
                      "bne        3b                              \n\t"   
                      "4:                                             \n\t"   
                      : [xx] "+r" (xx), [count] "+r" (count), [colors] "+r" (colors)
                      : [table] "r" (table), [srcAddr] "r" (srcAddr)
                      : "memory", "cc", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11"
                      );
    }

    s.fBitmap->getColorTable()->unlockColors(false);
}
#endif 






void SkBitmapProcState::platformProcs() {
    bool doFilter = fDoFilter;
    bool isOpaque = 256 == fAlphaScale;
    bool justDx = false;

    if (fInvType <= (SkMatrix::kTranslate_Mask | SkMatrix::kScale_Mask)) {
        justDx = true;
    }

    switch (fBitmap->config()) {
        case SkBitmap::kIndex8_Config:
#if __ARM_ARCH__ >= 6 && !defined(SK_CPU_BENDIAN)
            if (justDx && !doFilter) {
#if 0   
                fSampleProc16 = SI8_D16_nofilter_DX_arm;
                fShaderProc16 = NULL;
#endif
                if (isOpaque) {
                    
                    fSampleProc32 = SI8_opaque_D32_nofilter_DX_arm;
                    fShaderProc32 = NULL;
                }
            }
#endif
            break;
        default:
            break;
    }
}

