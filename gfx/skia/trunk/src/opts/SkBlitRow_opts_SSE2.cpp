






#include <emmintrin.h>
#include "SkBitmapProcState_opts_SSE2.h"
#include "SkBlitRow_opts_SSE2.h"
#include "SkColorPriv.h"
#include "SkColor_opts_SSE2.h"
#include "SkDither.h"
#include "SkUtils.h"




void S32_Blend_BlitRow32_SSE2(SkPMColor* SK_RESTRICT dst,
                              const SkPMColor* SK_RESTRICT src,
                              int count, U8CPU alpha) {
    SkASSERT(alpha <= 255);
    if (count <= 0) {
        return;
    }

    uint32_t src_scale = SkAlpha255To256(alpha);
    uint32_t dst_scale = 256 - src_scale;

    if (count >= 4) {
        SkASSERT(((size_t)dst & 0x03) == 0);
        while (((size_t)dst & 0x0F) != 0) {
            *dst = SkAlphaMulQ(*src, src_scale) + SkAlphaMulQ(*dst, dst_scale);
            src++;
            dst++;
            count--;
        }

        const __m128i *s = reinterpret_cast<const __m128i*>(src);
        __m128i *d = reinterpret_cast<__m128i*>(dst);
        __m128i rb_mask = _mm_set1_epi32(0x00FF00FF);
        __m128i ag_mask = _mm_set1_epi32(0xFF00FF00);

        
        __m128i src_scale_wide = _mm_set1_epi16(src_scale << 8);
        __m128i dst_scale_wide = _mm_set1_epi16(dst_scale << 8);
        while (count >= 4) {
            
            __m128i src_pixel = _mm_loadu_si128(s);
            __m128i dst_pixel = _mm_load_si128(d);

            
            
            
            
            

            
            
            __m128i src_rb = _mm_and_si128(rb_mask, src_pixel);

            
            
            
            
            src_rb = _mm_mulhi_epu16(src_rb, src_scale_wide);

            
            
            __m128i src_ag = _mm_and_si128(ag_mask, src_pixel);

            
            
            src_ag = _mm_mulhi_epu16(src_ag, src_scale_wide);

            
            
            src_ag = _mm_and_si128(src_ag, ag_mask);

            
            
            __m128i dst_rb = _mm_and_si128(rb_mask, dst_pixel);
            dst_rb = _mm_mulhi_epu16(dst_rb, dst_scale_wide);
            __m128i dst_ag = _mm_and_si128(ag_mask, dst_pixel);
            dst_ag = _mm_mulhi_epu16(dst_ag, dst_scale_wide);
            dst_ag = _mm_and_si128(dst_ag, ag_mask);

            
            
            src_pixel = _mm_or_si128(src_rb, src_ag);
            dst_pixel = _mm_or_si128(dst_rb, dst_ag);

            
            __m128i result = _mm_add_epi8(src_pixel, dst_pixel);
            _mm_store_si128(d, result);
            s++;
            d++;
            count -= 4;
        }
        src = reinterpret_cast<const SkPMColor*>(s);
        dst = reinterpret_cast<SkPMColor*>(d);
    }

    while (count > 0) {
        *dst = SkAlphaMulQ(*src, src_scale) + SkAlphaMulQ(*dst, dst_scale);
        src++;
        dst++;
        count--;
    }
}

void S32A_Opaque_BlitRow32_SSE2(SkPMColor* SK_RESTRICT dst,
                                const SkPMColor* SK_RESTRICT src,
                                int count, U8CPU alpha) {
    SkASSERT(alpha == 255);
    if (count <= 0) {
        return;
    }

    if (count >= 4) {
        SkASSERT(((size_t)dst & 0x03) == 0);
        while (((size_t)dst & 0x0F) != 0) {
            *dst = SkPMSrcOver(*src, *dst);
            src++;
            dst++;
            count--;
        }

        const __m128i *s = reinterpret_cast<const __m128i*>(src);
        __m128i *d = reinterpret_cast<__m128i*>(dst);
#ifdef SK_USE_ACCURATE_BLENDING
        __m128i rb_mask = _mm_set1_epi32(0x00FF00FF);
        __m128i c_128 = _mm_set1_epi16(128);  
        __m128i c_255 = _mm_set1_epi16(255);  
        while (count >= 4) {
            
            __m128i src_pixel = _mm_loadu_si128(s);
            __m128i dst_pixel = _mm_load_si128(d);

            __m128i dst_rb = _mm_and_si128(rb_mask, dst_pixel);
            __m128i dst_ag = _mm_srli_epi16(dst_pixel, 8);
            
            __m128i alpha = _mm_srli_epi32(src_pixel, 24);

            
            alpha = _mm_or_si128(alpha, _mm_slli_epi32(alpha, 16));

            
            alpha = _mm_sub_epi16(c_255, alpha);

            
            dst_rb = _mm_mullo_epi16(dst_rb, alpha);
            
            dst_ag = _mm_mullo_epi16(dst_ag, alpha);

            
            __m128i dst_rb_low = _mm_srli_epi16(dst_rb, 8);
            __m128i dst_ag_low = _mm_srli_epi16(dst_ag, 8);

            
            dst_rb = _mm_add_epi16(dst_rb, dst_rb_low);
            dst_rb = _mm_add_epi16(dst_rb, c_128);
            dst_rb = _mm_srli_epi16(dst_rb, 8);

            
            dst_ag = _mm_add_epi16(dst_ag, dst_ag_low);
            dst_ag = _mm_add_epi16(dst_ag, c_128);
            dst_ag = _mm_andnot_si128(rb_mask, dst_ag);

            
            dst_pixel = _mm_or_si128(dst_rb, dst_ag);

            
            __m128i result = _mm_add_epi8(src_pixel, dst_pixel);
            _mm_store_si128(d, result);
            s++;
            d++;
            count -= 4;
        }
#else
        __m128i rb_mask = _mm_set1_epi32(0x00FF00FF);
        __m128i c_256 = _mm_set1_epi16(0x0100);  
        while (count >= 4) {
            
            __m128i src_pixel = _mm_loadu_si128(s);
            __m128i dst_pixel = _mm_load_si128(d);

            __m128i dst_rb = _mm_and_si128(rb_mask, dst_pixel);
            __m128i dst_ag = _mm_srli_epi16(dst_pixel, 8);

            
            __m128i alpha = _mm_srli_epi16(src_pixel, 8);

            
            alpha = _mm_shufflehi_epi16(alpha, 0xF5);

            
            alpha = _mm_shufflelo_epi16(alpha, 0xF5);

            
            alpha = _mm_sub_epi16(c_256, alpha);

            
            dst_rb = _mm_mullo_epi16(dst_rb, alpha);
            
            dst_ag = _mm_mullo_epi16(dst_ag, alpha);

            
            dst_rb = _mm_srli_epi16(dst_rb, 8);

            
            dst_ag = _mm_andnot_si128(rb_mask, dst_ag);

            
            dst_pixel = _mm_or_si128(dst_rb, dst_ag);

            
            __m128i result = _mm_add_epi8(src_pixel, dst_pixel);
            _mm_store_si128(d, result);
            s++;
            d++;
            count -= 4;
        }
#endif
        src = reinterpret_cast<const SkPMColor*>(s);
        dst = reinterpret_cast<SkPMColor*>(d);
    }

    while (count > 0) {
        *dst = SkPMSrcOver(*src, *dst);
        src++;
        dst++;
        count--;
    }
}

void S32A_Blend_BlitRow32_SSE2(SkPMColor* SK_RESTRICT dst,
                               const SkPMColor* SK_RESTRICT src,
                               int count, U8CPU alpha) {
    SkASSERT(alpha <= 255);
    if (count <= 0) {
        return;
    }

    if (count >= 4) {
        while (((size_t)dst & 0x0F) != 0) {
            *dst = SkBlendARGB32(*src, *dst, alpha);
            src++;
            dst++;
            count--;
        }

        uint32_t src_scale = SkAlpha255To256(alpha);

        const __m128i *s = reinterpret_cast<const __m128i*>(src);
        __m128i *d = reinterpret_cast<__m128i*>(dst);
        __m128i src_scale_wide = _mm_set1_epi16(src_scale << 8);
        __m128i rb_mask = _mm_set1_epi32(0x00FF00FF);
        __m128i c_256 = _mm_set1_epi16(256);  
        while (count >= 4) {
            
            __m128i src_pixel = _mm_loadu_si128(s);
            __m128i dst_pixel = _mm_load_si128(d);

            
            __m128i dst_rb = _mm_and_si128(rb_mask, dst_pixel);
            __m128i src_rb = _mm_and_si128(rb_mask, src_pixel);

            
            __m128i dst_ag = _mm_srli_epi16(dst_pixel, 8);
            __m128i src_ag = _mm_srli_epi16(src_pixel, 8);

            
            
            
            __m128i dst_alpha = _mm_shufflehi_epi16(src_ag, 0xF5);
            dst_alpha = _mm_shufflelo_epi16(dst_alpha, 0xF5);

            
            
            
            
            
            dst_alpha = _mm_mulhi_epu16(dst_alpha, src_scale_wide);

            
            dst_alpha = _mm_sub_epi16(c_256, dst_alpha);

            
            dst_rb = _mm_mullo_epi16(dst_rb, dst_alpha);
            
            dst_ag = _mm_mullo_epi16(dst_ag, dst_alpha);

            
            
            
            
            
            
            
            src_rb = _mm_mulhi_epu16(src_rb, src_scale_wide);
            
            
            src_ag = _mm_mulhi_epu16(src_ag, src_scale_wide);

            
            dst_rb = _mm_srli_epi16(dst_rb, 8);

            
            dst_ag = _mm_andnot_si128(rb_mask, dst_ag);
            
            
            src_ag = _mm_slli_epi16(src_ag, 8);

            
            dst_pixel = _mm_or_si128(dst_rb, dst_ag);
            src_pixel = _mm_or_si128(src_rb, src_ag);

            
            __m128i result = _mm_add_epi8(src_pixel, dst_pixel);
            _mm_store_si128(d, result);
            s++;
            d++;
            count -= 4;
        }
        src = reinterpret_cast<const SkPMColor*>(s);
        dst = reinterpret_cast<SkPMColor*>(d);
    }

    while (count > 0) {
        *dst = SkBlendARGB32(*src, *dst, alpha);
        src++;
        dst++;
        count--;
    }
}




void Color32_SSE2(SkPMColor dst[], const SkPMColor src[], int count,
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
    } else {
        unsigned scale = 256 - SkAlpha255To256(colorA);

        if (count >= 4) {
            SkASSERT(((size_t)dst & 0x03) == 0);
            while (((size_t)dst & 0x0F) != 0) {
                *dst = color + SkAlphaMulQ(*src, scale);
                src++;
                dst++;
                count--;
            }

            const __m128i *s = reinterpret_cast<const __m128i*>(src);
            __m128i *d = reinterpret_cast<__m128i*>(dst);
            __m128i rb_mask = _mm_set1_epi32(0x00FF00FF);
            __m128i src_scale_wide = _mm_set1_epi16(scale);
            __m128i color_wide = _mm_set1_epi32(color);
            while (count >= 4) {
                
                __m128i src_pixel = _mm_loadu_si128(s);

                
                __m128i src_rb = _mm_and_si128(rb_mask, src_pixel);

                
                __m128i src_ag = _mm_srli_epi16(src_pixel, 8);

                
                src_rb = _mm_mullo_epi16(src_rb, src_scale_wide);
                src_ag = _mm_mullo_epi16(src_ag, src_scale_wide);

                
                src_rb = _mm_srli_epi16(src_rb, 8);
                src_ag = _mm_andnot_si128(rb_mask, src_ag);

                
                src_pixel = _mm_or_si128(src_rb, src_ag);

                
                __m128i result = _mm_add_epi8(color_wide, src_pixel);

                
                _mm_store_si128(d, result);
                s++;
                d++;
                count -= 4;
            }
            src = reinterpret_cast<const SkPMColor*>(s);
            dst = reinterpret_cast<SkPMColor*>(d);
        }

        while (count > 0) {
            *dst = color + SkAlphaMulQ(*src, scale);
            src += 1;
            dst += 1;
            count--;
        }
    }
}

void SkARGB32_A8_BlitMask_SSE2(void* device, size_t dstRB, const void* maskPtr,
                               size_t maskRB, SkColor origColor,
                               int width, int height) {
    SkPMColor color = SkPreMultiplyColor(origColor);
    size_t dstOffset = dstRB - (width << 2);
    size_t maskOffset = maskRB - width;
    SkPMColor* dst = (SkPMColor *)device;
    const uint8_t* mask = (const uint8_t*)maskPtr;
    do {
        int count = width;
        if (count >= 4) {
            while (((size_t)dst & 0x0F) != 0 && (count > 0)) {
                *dst = SkBlendARGB32(color, *dst, *mask);
                mask++;
                dst++;
                count--;
            }
            __m128i *d = reinterpret_cast<__m128i*>(dst);
            __m128i rb_mask = _mm_set1_epi32(0x00FF00FF);
            __m128i c_256 = _mm_set1_epi16(256);
            __m128i c_1 = _mm_set1_epi16(1);
            __m128i src_pixel = _mm_set1_epi32(color);
            while (count >= 4) {
                
                __m128i dst_pixel = _mm_load_si128(d);

                
                __m128i src_scale_wide =  _mm_set_epi8(0, *(mask+3),\
                                0, *(mask+3),0, \
                                *(mask+2),0, *(mask+2),\
                                0,*(mask+1), 0,*(mask+1),\
                                0, *mask,0,*mask);

                
                src_scale_wide = _mm_add_epi16(src_scale_wide, c_1);

                
                __m128i dst_rb = _mm_and_si128(rb_mask, dst_pixel);
                __m128i src_rb = _mm_and_si128(rb_mask, src_pixel);

                
                __m128i dst_ag = _mm_srli_epi16(dst_pixel, 8);
                __m128i src_ag = _mm_srli_epi16(src_pixel, 8);

                
                __m128i dst_alpha = _mm_shufflehi_epi16(src_ag, 0xF5);
                dst_alpha = _mm_shufflelo_epi16(dst_alpha, 0xF5);

                
                dst_alpha = _mm_mullo_epi16(dst_alpha, src_scale_wide);

                
                dst_alpha = _mm_srli_epi16(dst_alpha, 8);

                
                dst_alpha = _mm_sub_epi16(c_256, dst_alpha);
                
                dst_rb = _mm_mullo_epi16(dst_rb, dst_alpha);
                
                dst_ag = _mm_mullo_epi16(dst_ag, dst_alpha);

                
                src_rb = _mm_mullo_epi16(src_rb, src_scale_wide);
                
                src_ag = _mm_mullo_epi16(src_ag, src_scale_wide);
                
                dst_rb = _mm_srli_epi16(dst_rb, 8);
                src_rb = _mm_srli_epi16(src_rb, 8);

                
                dst_ag = _mm_andnot_si128(rb_mask, dst_ag);
                src_ag = _mm_andnot_si128(rb_mask, src_ag);

                
                dst_pixel = _mm_or_si128(dst_rb, dst_ag);
                __m128i tmp_src_pixel = _mm_or_si128(src_rb, src_ag);

                
                __m128i result = _mm_add_epi8(tmp_src_pixel, dst_pixel);
                _mm_store_si128(d, result);
                
                mask = mask + 4;
                d++;
                count -= 4;
            }
            dst = reinterpret_cast<SkPMColor *>(d);
        }
        while (count > 0) {
            *dst= SkBlendARGB32(color, *dst, *mask);
            dst += 1;
            mask++;
            count --;
        }
        dst = (SkPMColor *)((char*)dst + dstOffset);
        mask += maskOffset;
    } while (--height != 0);
}




#define SK_R16x5_R32x5_SHIFT (SK_R32_SHIFT - SK_R16_SHIFT - SK_R16_BITS + 5)
#define SK_G16x5_G32x5_SHIFT (SK_G32_SHIFT - SK_G16_SHIFT - SK_G16_BITS + 5)
#define SK_B16x5_B32x5_SHIFT (SK_B32_SHIFT - SK_B16_SHIFT - SK_B16_BITS + 5)

#if SK_R16x5_R32x5_SHIFT == 0
    #define SkPackedR16x5ToUnmaskedR32x5_SSE2(x) (x)
#elif SK_R16x5_R32x5_SHIFT > 0
    #define SkPackedR16x5ToUnmaskedR32x5_SSE2(x) (_mm_slli_epi32(x, SK_R16x5_R32x5_SHIFT))
#else
    #define SkPackedR16x5ToUnmaskedR32x5_SSE2(x) (_mm_srli_epi32(x, -SK_R16x5_R32x5_SHIFT))
#endif

#if SK_G16x5_G32x5_SHIFT == 0
    #define SkPackedG16x5ToUnmaskedG32x5_SSE2(x) (x)
#elif SK_G16x5_G32x5_SHIFT > 0
    #define SkPackedG16x5ToUnmaskedG32x5_SSE2(x) (_mm_slli_epi32(x, SK_G16x5_G32x5_SHIFT))
#else
    #define SkPackedG16x5ToUnmaskedG32x5_SSE2(x) (_mm_srli_epi32(x, -SK_G16x5_G32x5_SHIFT))
#endif

#if SK_B16x5_B32x5_SHIFT == 0
    #define SkPackedB16x5ToUnmaskedB32x5_SSE2(x) (x)
#elif SK_B16x5_B32x5_SHIFT > 0
    #define SkPackedB16x5ToUnmaskedB32x5_SSE2(x) (_mm_slli_epi32(x, SK_B16x5_B32x5_SHIFT))
#else
    #define SkPackedB16x5ToUnmaskedB32x5_SSE2(x) (_mm_srli_epi32(x, -SK_B16x5_B32x5_SHIFT))
#endif

static __m128i SkBlendLCD16_SSE2(__m128i &src, __m128i &dst,
                                 __m128i &mask, __m128i &srcA) {
    
    
    
    
    
    

    
    
    
    
    
    
    
    

    
    
    __m128i r = _mm_and_si128(SkPackedR16x5ToUnmaskedR32x5_SSE2(mask),
                              _mm_set1_epi32(0x1F << SK_R32_SHIFT));

    
    __m128i g = _mm_and_si128(SkPackedG16x5ToUnmaskedG32x5_SSE2(mask),
                              _mm_set1_epi32(0x1F << SK_G32_SHIFT));

    
    __m128i b = _mm_and_si128(SkPackedB16x5ToUnmaskedB32x5_SSE2(mask),
                              _mm_set1_epi32(0x1F << SK_B32_SHIFT));

    
    
    
    
    
    mask = _mm_or_si128(_mm_or_si128(r, g), b);

    
    
    
    __m128i maskLo, maskHi;
    
    maskLo = _mm_unpacklo_epi8(mask, _mm_setzero_si128());
    
    maskHi = _mm_unpackhi_epi8(mask, _mm_setzero_si128());

    
    
    
    
    maskLo = _mm_add_epi16(maskLo, _mm_srli_epi16(maskLo, 4));
    maskHi = _mm_add_epi16(maskHi, _mm_srli_epi16(maskHi, 4));

    
    maskLo = _mm_mullo_epi16(maskLo, srcA);
    maskHi = _mm_mullo_epi16(maskHi, srcA);

    
    maskLo = _mm_srli_epi16(maskLo, 8);
    maskHi = _mm_srli_epi16(maskHi, 8);

    
    
    __m128i dstLo = _mm_unpacklo_epi8(dst, _mm_setzero_si128());
    
    __m128i dstHi = _mm_unpackhi_epi8(dst, _mm_setzero_si128());

    
    maskLo = _mm_mullo_epi16(maskLo, _mm_sub_epi16(src, dstLo));
    maskHi = _mm_mullo_epi16(maskHi, _mm_sub_epi16(src, dstHi));

    
    maskLo = _mm_srai_epi16(maskLo, 5);
    maskHi = _mm_srai_epi16(maskHi, 5);

    
    
    __m128i resultLo = _mm_add_epi16(dstLo, maskLo);
    __m128i resultHi = _mm_add_epi16(dstHi, maskHi);

    
    
    
    
    return _mm_packus_epi16(resultLo, resultHi);
}

static __m128i SkBlendLCD16Opaque_SSE2(__m128i &src, __m128i &dst,
                                       __m128i &mask) {
    
    
    
    
    
    

    
    
    
    
    
    

    
    
    __m128i r = _mm_and_si128(SkPackedR16x5ToUnmaskedR32x5_SSE2(mask),
                              _mm_set1_epi32(0x1F << SK_R32_SHIFT));

    
    __m128i g = _mm_and_si128(SkPackedG16x5ToUnmaskedG32x5_SSE2(mask),
                              _mm_set1_epi32(0x1F << SK_G32_SHIFT));

    
    __m128i b = _mm_and_si128(SkPackedB16x5ToUnmaskedB32x5_SSE2(mask),
                              _mm_set1_epi32(0x1F << SK_B32_SHIFT));

    
    
    
    
    
    mask = _mm_or_si128(_mm_or_si128(r, g), b);

    
    
    
    __m128i maskLo, maskHi;
    
    maskLo = _mm_unpacklo_epi8(mask, _mm_setzero_si128());
    
    maskHi = _mm_unpackhi_epi8(mask, _mm_setzero_si128());

    
    
    
    
    maskLo = _mm_add_epi16(maskLo, _mm_srli_epi16(maskLo, 4));
    maskHi = _mm_add_epi16(maskHi, _mm_srli_epi16(maskHi, 4));

    
    
    __m128i dstLo = _mm_unpacklo_epi8(dst, _mm_setzero_si128());
    
    __m128i dstHi = _mm_unpackhi_epi8(dst, _mm_setzero_si128());

    
    maskLo = _mm_mullo_epi16(maskLo, _mm_sub_epi16(src, dstLo));
    maskHi = _mm_mullo_epi16(maskHi, _mm_sub_epi16(src, dstHi));

    
    maskLo = _mm_srai_epi16(maskLo, 5);
    maskHi = _mm_srai_epi16(maskHi, 5);

    
    
    __m128i resultLo = _mm_add_epi16(dstLo, maskLo);
    __m128i resultHi = _mm_add_epi16(dstHi, maskHi);

    
    
    
    
    return _mm_or_si128(_mm_packus_epi16(resultLo, resultHi),
                        _mm_set1_epi32(SK_A32_MASK << SK_A32_SHIFT));
}

void SkBlitLCD16Row_SSE2(SkPMColor dst[], const uint16_t mask[],
                         SkColor src, int width, SkPMColor) {
    if (width <= 0) {
        return;
    }

    int srcA = SkColorGetA(src);
    int srcR = SkColorGetR(src);
    int srcG = SkColorGetG(src);
    int srcB = SkColorGetB(src);

    srcA = SkAlpha255To256(srcA);

    if (width >= 4) {
        SkASSERT(((size_t)dst & 0x03) == 0);
        while (((size_t)dst & 0x0F) != 0) {
            *dst = SkBlendLCD16(srcA, srcR, srcG, srcB, *dst, *mask);
            mask++;
            dst++;
            width--;
        }

        __m128i *d = reinterpret_cast<__m128i*>(dst);
        
        __m128i src_sse = _mm_set1_epi32(SkPackARGB32(0xFF, srcR, srcG, srcB));
        
        src_sse = _mm_unpacklo_epi8(src_sse, _mm_setzero_si128());
        
        
        __m128i srcA_sse = _mm_set1_epi16(srcA);
        while (width >= 4) {
            
            __m128i dst_sse = _mm_load_si128(d);
            
            __m128i mask_sse = _mm_loadl_epi64(
                                   reinterpret_cast<const __m128i*>(mask));

            
            
            
            int pack_cmp = _mm_movemask_epi8(_mm_cmpeq_epi16(mask_sse,
                                             _mm_setzero_si128()));

            
            if (pack_cmp != 0xFFFF) {
                
                
                
                mask_sse = _mm_unpacklo_epi16(mask_sse,
                                              _mm_setzero_si128());

                
                __m128i result = SkBlendLCD16_SSE2(src_sse, dst_sse,
                                                   mask_sse, srcA_sse);
                _mm_store_si128(d, result);
            }

            d++;
            mask += 4;
            width -= 4;
        }

        dst = reinterpret_cast<SkPMColor*>(d);
    }

    while (width > 0) {
        *dst = SkBlendLCD16(srcA, srcR, srcG, srcB, *dst, *mask);
        mask++;
        dst++;
        width--;
    }
}

void SkBlitLCD16OpaqueRow_SSE2(SkPMColor dst[], const uint16_t mask[],
                               SkColor src, int width, SkPMColor opaqueDst) {
    if (width <= 0) {
        return;
    }

    int srcR = SkColorGetR(src);
    int srcG = SkColorGetG(src);
    int srcB = SkColorGetB(src);

    if (width >= 4) {
        SkASSERT(((size_t)dst & 0x03) == 0);
        while (((size_t)dst & 0x0F) != 0) {
            *dst = SkBlendLCD16Opaque(srcR, srcG, srcB, *dst, *mask, opaqueDst);
            mask++;
            dst++;
            width--;
        }

        __m128i *d = reinterpret_cast<__m128i*>(dst);
        
        __m128i src_sse = _mm_set1_epi32(SkPackARGB32(0xFF, srcR, srcG, srcB));
        
        
        src_sse = _mm_unpacklo_epi8(src_sse, _mm_setzero_si128());
        while (width >= 4) {
            
            __m128i dst_sse = _mm_load_si128(d);
            
            __m128i mask_sse = _mm_loadl_epi64(
                                   reinterpret_cast<const __m128i*>(mask));

            
            
            
            int pack_cmp = _mm_movemask_epi8(_mm_cmpeq_epi16(mask_sse,
                                             _mm_setzero_si128()));

            
            if (pack_cmp != 0xFFFF) {
                
                
                
                mask_sse = _mm_unpacklo_epi16(mask_sse,
                                              _mm_setzero_si128());

                
                __m128i result = SkBlendLCD16Opaque_SSE2(src_sse, dst_sse,
                                                         mask_sse);
                _mm_store_si128(d, result);
            }

            d++;
            mask += 4;
            width -= 4;
        }

        dst = reinterpret_cast<SkPMColor*>(d);
    }

    while (width > 0) {
        *dst = SkBlendLCD16Opaque(srcR, srcG, srcB, *dst, *mask, opaqueDst);
        mask++;
        dst++;
        width--;
    }
}




void S32_D565_Opaque_SSE2(uint16_t* SK_RESTRICT dst,
                          const SkPMColor* SK_RESTRICT src, int count,
                          U8CPU alpha, int , int ) {
    SkASSERT(255 == alpha);

    if (count <= 0) {
        return;
    }

    if (count >= 8) {
        while (((size_t)dst & 0x0F) != 0) {
            SkPMColor c = *src++;
            SkPMColorAssert(c);

            *dst++ = SkPixel32ToPixel16_ToU16(c);
            count--;
        }

        const __m128i* s = reinterpret_cast<const __m128i*>(src);
        __m128i* d = reinterpret_cast<__m128i*>(dst);
        __m128i r16_mask = _mm_set1_epi32(SK_R16_MASK);
        __m128i g16_mask = _mm_set1_epi32(SK_G16_MASK);
        __m128i b16_mask = _mm_set1_epi32(SK_B16_MASK);

        while (count >= 8) {
            
            __m128i src_pixel1 = _mm_loadu_si128(s++);
            __m128i src_pixel2 = _mm_loadu_si128(s++);

            
            __m128i r1 = _mm_srli_epi32(src_pixel1,
                                        SK_R32_SHIFT + (8 - SK_R16_BITS));
            r1 = _mm_and_si128(r1, r16_mask);
            __m128i r2 = _mm_srli_epi32(src_pixel2,
                                        SK_R32_SHIFT + (8 - SK_R16_BITS));
            r2 = _mm_and_si128(r2, r16_mask);
            __m128i r = _mm_packs_epi32(r1, r2);

            
            __m128i g1 = _mm_srli_epi32(src_pixel1,
                                        SK_G32_SHIFT + (8 - SK_G16_BITS));
            g1 = _mm_and_si128(g1, g16_mask);
            __m128i g2 = _mm_srli_epi32(src_pixel2,
                                        SK_G32_SHIFT + (8 - SK_G16_BITS));
            g2 = _mm_and_si128(g2, g16_mask);
            __m128i g = _mm_packs_epi32(g1, g2);

            
            __m128i b1 = _mm_srli_epi32(src_pixel1,
                                        SK_B32_SHIFT + (8 - SK_B16_BITS));
            b1 = _mm_and_si128(b1, b16_mask);
            __m128i b2 = _mm_srli_epi32(src_pixel2,
                                        SK_B32_SHIFT + (8 - SK_B16_BITS));
            b2 = _mm_and_si128(b2, b16_mask);
            __m128i b = _mm_packs_epi32(b1, b2);

            
            __m128i d_pixel = SkPackRGB16_SSE2(r, g, b);
            _mm_store_si128(d++, d_pixel);
            count -= 8;
        }
        src = reinterpret_cast<const SkPMColor*>(s);
        dst = reinterpret_cast<uint16_t*>(d);
    }

    if (count > 0) {
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            *dst++ = SkPixel32ToPixel16_ToU16(c);
        } while (--count != 0);
    }
}




void S32A_D565_Opaque_SSE2(uint16_t* SK_RESTRICT dst,
                           const SkPMColor* SK_RESTRICT src,
                           int count, U8CPU alpha, int , int ) {
    SkASSERT(255 == alpha);

    if (count <= 0) {
        return;
    }

    if (count >= 8) {
        
        while (((size_t)dst & 0x0F) != 0) {
            SkPMColor c = *src++;
            if (c) {
              *dst = SkSrcOver32To16(c, *dst);
            }
            dst += 1;
            count--;
        }

        const __m128i* s = reinterpret_cast<const __m128i*>(src);
        __m128i* d = reinterpret_cast<__m128i*>(dst);
        __m128i var255 = _mm_set1_epi16(255);
        __m128i r16_mask = _mm_set1_epi16(SK_R16_MASK);
        __m128i g16_mask = _mm_set1_epi16(SK_G16_MASK);
        __m128i b16_mask = _mm_set1_epi16(SK_B16_MASK);

        while (count >= 8) {
            
            __m128i src_pixel1 = _mm_loadu_si128(s++);
            __m128i src_pixel2 = _mm_loadu_si128(s++);

            
            
            
            int src_cmp1 = _mm_movemask_epi8(_mm_cmpeq_epi16(src_pixel1,
                                             _mm_setzero_si128()));
            int src_cmp2 = _mm_movemask_epi8(_mm_cmpeq_epi16(src_pixel2,
                                             _mm_setzero_si128()));
            if (src_cmp1 == 0xFFFF && src_cmp2 == 0xFFFF) {
                d++;
                count -= 8;
                continue;
            }

            
            __m128i dst_pixel = _mm_load_si128(d);

            
            __m128i sa1 = _mm_slli_epi32(src_pixel1, (24 - SK_A32_SHIFT));
            sa1 = _mm_srli_epi32(sa1, 24);
            __m128i sa2 = _mm_slli_epi32(src_pixel2, (24 - SK_A32_SHIFT));
            sa2 = _mm_srli_epi32(sa2, 24);
            __m128i sa = _mm_packs_epi32(sa1, sa2);

            
            __m128i sr1 = _mm_slli_epi32(src_pixel1, (24 - SK_R32_SHIFT));
            sr1 = _mm_srli_epi32(sr1, 24);
            __m128i sr2 = _mm_slli_epi32(src_pixel2, (24 - SK_R32_SHIFT));
            sr2 = _mm_srli_epi32(sr2, 24);
            __m128i sr = _mm_packs_epi32(sr1, sr2);

            
            __m128i sg1 = _mm_slli_epi32(src_pixel1, (24 - SK_G32_SHIFT));
            sg1 = _mm_srli_epi32(sg1, 24);
            __m128i sg2 = _mm_slli_epi32(src_pixel2, (24 - SK_G32_SHIFT));
            sg2 = _mm_srli_epi32(sg2, 24);
            __m128i sg = _mm_packs_epi32(sg1, sg2);

            
            __m128i sb1 = _mm_slli_epi32(src_pixel1, (24 - SK_B32_SHIFT));
            sb1 = _mm_srli_epi32(sb1, 24);
            __m128i sb2 = _mm_slli_epi32(src_pixel2, (24 - SK_B32_SHIFT));
            sb2 = _mm_srli_epi32(sb2, 24);
            __m128i sb = _mm_packs_epi32(sb1, sb2);

            
            __m128i dr = _mm_srli_epi16(dst_pixel, SK_R16_SHIFT);
            dr = _mm_and_si128(dr, r16_mask);
            __m128i dg = _mm_srli_epi16(dst_pixel, SK_G16_SHIFT);
            dg = _mm_and_si128(dg, g16_mask);
            __m128i db = _mm_srli_epi16(dst_pixel, SK_B16_SHIFT);
            db = _mm_and_si128(db, b16_mask);

            __m128i isa = _mm_sub_epi16(var255, sa); 

            
            
            dr = _mm_add_epi16(sr, SkMul16ShiftRound_SSE2(dr, isa, SK_R16_BITS));
            dr = _mm_srli_epi16(dr, 8 - SK_R16_BITS);
            dg = _mm_add_epi16(sg, SkMul16ShiftRound_SSE2(dg, isa, SK_G16_BITS));
            dg = _mm_srli_epi16(dg, 8 - SK_G16_BITS);
            db = _mm_add_epi16(sb, SkMul16ShiftRound_SSE2(db, isa, SK_B16_BITS));
            db = _mm_srli_epi16(db, 8 - SK_B16_BITS);

            
            __m128i d_pixel = SkPackRGB16_SSE2(dr, dg, db);

            
            _mm_store_si128(d++, d_pixel);
            count -= 8;
        }

        src = reinterpret_cast<const SkPMColor*>(s);
        dst = reinterpret_cast<uint16_t*>(d);
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

void S32_D565_Opaque_Dither_SSE2(uint16_t* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src,
                                 int count, U8CPU alpha, int x, int y) {
    SkASSERT(255 == alpha);

    if (count <= 0) {
        return;
    }

    if (count >= 8) {
        while (((size_t)dst & 0x0F) != 0) {
            DITHER_565_SCAN(y);
            SkPMColor c = *src++;
            SkPMColorAssert(c);

            unsigned dither = DITHER_VALUE(x);
            *dst++ = SkDitherRGB32To565(c, dither);
            DITHER_INC_X(x);
            count--;
        }

        unsigned short dither_value[8];
        __m128i dither;
#ifdef ENABLE_DITHER_MATRIX_4X4
        const uint8_t* dither_scan = gDitherMatrix_3Bit_4X4[(y) & 3];
        dither_value[0] = dither_value[4] = dither_scan[(x) & 3];
        dither_value[1] = dither_value[5] = dither_scan[(x + 1) & 3];
        dither_value[2] = dither_value[6] = dither_scan[(x + 2) & 3];
        dither_value[3] = dither_value[7] = dither_scan[(x + 3) & 3];
#else
        const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
        dither_value[0] = dither_value[4] = (dither_scan
                                             >> (((x) & 3) << 2)) & 0xF;
        dither_value[1] = dither_value[5] = (dither_scan
                                             >> (((x + 1) & 3) << 2)) & 0xF;
        dither_value[2] = dither_value[6] = (dither_scan
                                             >> (((x + 2) & 3) << 2)) & 0xF;
        dither_value[3] = dither_value[7] = (dither_scan
                                             >> (((x + 3) & 3) << 2)) & 0xF;
#endif
        dither = _mm_loadu_si128((__m128i*) dither_value);

        const __m128i* s = reinterpret_cast<const __m128i*>(src);
        __m128i* d = reinterpret_cast<__m128i*>(dst);

        while (count >= 8) {
            
            __m128i src_pixel1 = _mm_loadu_si128(s++);
            __m128i src_pixel2 = _mm_loadu_si128(s++);

            
            __m128i sr1 = _mm_slli_epi32(src_pixel1, (24 - SK_R32_SHIFT));
            sr1 = _mm_srli_epi32(sr1, 24);
            __m128i sr2 = _mm_slli_epi32(src_pixel2, (24 - SK_R32_SHIFT));
            sr2 = _mm_srli_epi32(sr2, 24);
            __m128i sr = _mm_packs_epi32(sr1, sr2);

            
            __m128i sr_offset = _mm_srli_epi16(sr, 5);
            sr = _mm_add_epi16(sr, dither);
            sr = _mm_sub_epi16(sr, sr_offset);
            sr = _mm_srli_epi16(sr, SK_R32_BITS - SK_R16_BITS);

            
            __m128i sg1 = _mm_slli_epi32(src_pixel1, (24 - SK_G32_SHIFT));
            sg1 = _mm_srli_epi32(sg1, 24);
            __m128i sg2 = _mm_slli_epi32(src_pixel2, (24 - SK_G32_SHIFT));
            sg2 = _mm_srli_epi32(sg2, 24);
            __m128i sg = _mm_packs_epi32(sg1, sg2);

            
            __m128i sg_offset = _mm_srli_epi16(sg, 6);
            sg = _mm_add_epi16(sg, _mm_srli_epi16(dither, 1));
            sg = _mm_sub_epi16(sg, sg_offset);
            sg = _mm_srli_epi16(sg, SK_G32_BITS - SK_G16_BITS);

            
            __m128i sb1 = _mm_slli_epi32(src_pixel1, (24 - SK_B32_SHIFT));
            sb1 = _mm_srli_epi32(sb1, 24);
            __m128i sb2 = _mm_slli_epi32(src_pixel2, (24 - SK_B32_SHIFT));
            sb2 = _mm_srli_epi32(sb2, 24);
            __m128i sb = _mm_packs_epi32(sb1, sb2);

            
            __m128i sb_offset = _mm_srli_epi16(sb, 5);
            sb = _mm_add_epi16(sb, dither);
            sb = _mm_sub_epi16(sb, sb_offset);
            sb = _mm_srli_epi16(sb, SK_B32_BITS - SK_B16_BITS);

            
            __m128i d_pixel = SkPackRGB16_SSE2(sr, sg, sb);
            _mm_store_si128(d++, d_pixel);

            count -= 8;
            x += 8;
        }

        src = reinterpret_cast<const SkPMColor*>(s);
        dst = reinterpret_cast<uint16_t*>(d);
    }

    if (count > 0) {
        DITHER_565_SCAN(y);
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);

            unsigned dither = DITHER_VALUE(x);
            *dst++ = SkDitherRGB32To565(c, dither);
            DITHER_INC_X(x);
        } while (--count != 0);
    }
}




void S32A_D565_Opaque_Dither_SSE2(uint16_t* SK_RESTRICT dst,
                                  const SkPMColor* SK_RESTRICT src,
                                  int count, U8CPU alpha, int x, int y) {
    SkASSERT(255 == alpha);

    if (count <= 0) {
        return;
    }

    if (count >= 8) {
        while (((size_t)dst & 0x0F) != 0) {
            DITHER_565_SCAN(y);
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            if (c) {
                unsigned a = SkGetPackedA32(c);

                int d = SkAlphaMul(DITHER_VALUE(x), SkAlpha255To256(a));

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
            count--;
        }

        unsigned short dither_value[8];
        __m128i dither, dither_cur;
#ifdef ENABLE_DITHER_MATRIX_4X4
        const uint8_t* dither_scan = gDitherMatrix_3Bit_4X4[(y) & 3];
        dither_value[0] = dither_value[4] = dither_scan[(x) & 3];
        dither_value[1] = dither_value[5] = dither_scan[(x + 1) & 3];
        dither_value[2] = dither_value[6] = dither_scan[(x + 2) & 3];
        dither_value[3] = dither_value[7] = dither_scan[(x + 3) & 3];
#else
        const uint16_t dither_scan = gDitherMatrix_3Bit_16[(y) & 3];
        dither_value[0] = dither_value[4] = (dither_scan
                                             >> (((x) & 3) << 2)) & 0xF;
        dither_value[1] = dither_value[5] = (dither_scan
                                             >> (((x + 1) & 3) << 2)) & 0xF;
        dither_value[2] = dither_value[6] = (dither_scan
                                             >> (((x + 2) & 3) << 2)) & 0xF;
        dither_value[3] = dither_value[7] = (dither_scan
                                             >> (((x + 3) & 3) << 2)) & 0xF;
#endif
        dither = _mm_loadu_si128((__m128i*) dither_value);

        const __m128i* s = reinterpret_cast<const __m128i*>(src);
        __m128i* d = reinterpret_cast<__m128i*>(dst);
        __m128i var256 = _mm_set1_epi16(256);
        __m128i r16_mask = _mm_set1_epi16(SK_R16_MASK);
        __m128i g16_mask = _mm_set1_epi16(SK_G16_MASK);
        __m128i b16_mask = _mm_set1_epi16(SK_B16_MASK);

        while (count >= 8) {
            
            __m128i src_pixel1 = _mm_loadu_si128(s++);
            __m128i src_pixel2 = _mm_loadu_si128(s++);
            __m128i dst_pixel = _mm_load_si128(d);

            
            __m128i sa1 = _mm_slli_epi32(src_pixel1, (24 - SK_A32_SHIFT));
            sa1 = _mm_srli_epi32(sa1, 24);
            __m128i sa2 = _mm_slli_epi32(src_pixel2, (24 - SK_A32_SHIFT));
            sa2 = _mm_srli_epi32(sa2, 24);
            __m128i sa = _mm_packs_epi32(sa1, sa2);

            
            dither_cur = _mm_mullo_epi16(dither,
                                         _mm_add_epi16(sa, _mm_set1_epi16(1)));
            dither_cur = _mm_srli_epi16(dither_cur, 8);

            
            __m128i sr1 = _mm_slli_epi32(src_pixel1, (24 - SK_R32_SHIFT));
            sr1 = _mm_srli_epi32(sr1, 24);
            __m128i sr2 = _mm_slli_epi32(src_pixel2, (24 - SK_R32_SHIFT));
            sr2 = _mm_srli_epi32(sr2, 24);
            __m128i sr = _mm_packs_epi32(sr1, sr2);

            
            __m128i sr_offset = _mm_srli_epi16(sr, 5);
            sr = _mm_add_epi16(sr, dither_cur);
            sr = _mm_sub_epi16(sr, sr_offset);

            
            sr = _mm_slli_epi16(sr, 2);

            
            __m128i sg1 = _mm_slli_epi32(src_pixel1, (24 - SK_G32_SHIFT));
            sg1 = _mm_srli_epi32(sg1, 24);
            __m128i sg2 = _mm_slli_epi32(src_pixel2, (24 - SK_G32_SHIFT));
            sg2 = _mm_srli_epi32(sg2, 24);
            __m128i sg = _mm_packs_epi32(sg1, sg2);

            
            __m128i sg_offset = _mm_srli_epi16(sg, 6);
            sg = _mm_add_epi16(sg, _mm_srli_epi16(dither_cur, 1));
            sg = _mm_sub_epi16(sg, sg_offset);

            
            sg = _mm_slli_epi16(sg, 3);

            
            __m128i sb1 = _mm_slli_epi32(src_pixel1, (24 - SK_B32_SHIFT));
            sb1 = _mm_srli_epi32(sb1, 24);
            __m128i sb2 = _mm_slli_epi32(src_pixel2, (24 - SK_B32_SHIFT));
            sb2 = _mm_srli_epi32(sb2, 24);
            __m128i sb = _mm_packs_epi32(sb1, sb2);

            
            __m128i sb_offset = _mm_srli_epi16(sb, 5);
            sb = _mm_add_epi16(sb, dither_cur);
            sb = _mm_sub_epi16(sb, sb_offset);

            
            sb = _mm_slli_epi16(sb, 2);

            
            __m128i dr = _mm_srli_epi16(dst_pixel, SK_R16_SHIFT);
            dr = _mm_and_si128(dr, r16_mask);
            __m128i dg = _mm_srli_epi16(dst_pixel, SK_G16_SHIFT);
            dg = _mm_and_si128(dg, g16_mask);
            __m128i db = _mm_srli_epi16(dst_pixel, SK_B16_SHIFT);
            db = _mm_and_si128(db, b16_mask);

            
            __m128i isa = _mm_sub_epi16(var256, sa);
            isa = _mm_srli_epi16(isa, 3);

            dr = _mm_mullo_epi16(dr, isa);
            dr = _mm_add_epi16(dr, sr);
            dr = _mm_srli_epi16(dr, 5);

            dg = _mm_mullo_epi16(dg, isa);
            dg = _mm_add_epi16(dg, sg);
            dg = _mm_srli_epi16(dg, 5);

            db = _mm_mullo_epi16(db, isa);
            db = _mm_add_epi16(db, sb);
            db = _mm_srli_epi16(db, 5);

            
            __m128i d_pixel = SkPackRGB16_SSE2(dr, dg, db);
            _mm_store_si128(d++, d_pixel);

            count -= 8;
            x += 8;
        }

        src = reinterpret_cast<const SkPMColor*>(s);
        dst = reinterpret_cast<uint16_t*>(d);
    }

    if (count > 0) {
        DITHER_565_SCAN(y);
        do {
            SkPMColor c = *src++;
            SkPMColorAssert(c);
            if (c) {
                unsigned a = SkGetPackedA32(c);

                int d = SkAlphaMul(DITHER_VALUE(x), SkAlpha255To256(a));

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
