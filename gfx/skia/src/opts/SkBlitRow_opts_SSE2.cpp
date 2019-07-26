







#include "SkBlitRow_opts_SSE2.h"
#include "SkBitmapProcState_opts_SSE2.h"
#include "SkColorPriv.h"
#include "SkUtils.h"

#include <emmintrin.h>




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
        while(count > 0) {
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

static __m128i SkBlendLCD16_SSE2(__m128i &srci, __m128i &dst,
                                 __m128i &mask, __m128i &scale) {
    
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

    maskLo = _mm_mullo_epi16(maskLo, scale);
    maskHi = _mm_mullo_epi16(maskHi, scale);

    maskLo = _mm_srli_epi16(maskLo, 8);
    maskHi = _mm_srli_epi16(maskHi, 8);

    
    __m128i dstLo = _mm_unpacklo_epi8(dst, _mm_setzero_si128());
    __m128i dstHi = _mm_unpackhi_epi8(dst, _mm_setzero_si128());

    maskLo = _mm_mullo_epi16(maskLo, _mm_sub_epi16(srci, dstLo));
    maskHi = _mm_mullo_epi16(maskHi, _mm_sub_epi16(srci, dstHi));

    maskLo = _mm_srai_epi16(maskLo, 5);
    maskHi = _mm_srai_epi16(maskHi, 5);

    
    __m128i resultLo = _mm_add_epi16(dstLo, maskLo);
    __m128i resultHi = _mm_add_epi16(dstHi, maskHi);

    
    return _mm_packus_epi16(resultLo, resultHi);
}

static __m128i SkBlendLCD16Opaque_SSE2(__m128i &srci, __m128i &dst,
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

    maskLo = _mm_mullo_epi16(maskLo, _mm_sub_epi16(srci, dstLo));
    maskHi = _mm_mullo_epi16(maskHi, _mm_sub_epi16(srci, dstHi));

    maskLo = _mm_srai_epi16(maskLo, 5);
    maskHi = _mm_srai_epi16(maskHi, 5);

    
    __m128i resultLo = _mm_add_epi16(dstLo, maskLo);
    __m128i resultHi = _mm_add_epi16(dstHi, maskHi);

    
    return _mm_or_si128(_mm_packus_epi16(resultLo, resultHi),
                        _mm_set1_epi32(SK_A32_MASK << SK_A32_SHIFT));
}

void SkBlitLCD16Row_SSE2(SkPMColor dst[], const uint16_t src[],
                         SkColor color, int width, SkPMColor) {
    if (width <= 0) {
        return;
    }

    int srcA = SkColorGetA(color);
    int srcR = SkColorGetR(color);
    int srcG = SkColorGetG(color);
    int srcB = SkColorGetB(color);

    srcA = SkAlpha255To256(srcA);

    if (width >= 4) {
        SkASSERT(((size_t)dst & 0x03) == 0);
        while (((size_t)dst & 0x0F) != 0) {
            *dst = SkBlendLCD16(srcA, srcR, srcG, srcB, *dst, *src);
            src++;
            dst++;
            width--;
        }

        __m128i *d = reinterpret_cast<__m128i*>(dst);
        __m128i srci = _mm_set1_epi32(SkPackARGB32(0xFF, srcR, srcG, srcB));
        srci = _mm_unpacklo_epi8(srci, _mm_setzero_si128());
        __m128i scale = _mm_set1_epi16(srcA);
        while (width >= 4) {
            __m128i dst_pixel = _mm_load_si128(d);
            __m128i mask_pixel = _mm_loadl_epi64(
                                     reinterpret_cast<const __m128i*>(src));

            
            
            
            int pack_cmp = _mm_movemask_epi8(_mm_cmpeq_epi16(mask_pixel,
                                             _mm_setzero_si128()));

            
            if (pack_cmp != 0xFFFF) {
                
                
                mask_pixel = _mm_unpacklo_epi16(mask_pixel,
                                                _mm_setzero_si128());

                
                __m128i result = SkBlendLCD16_SSE2(srci, dst_pixel,
                                                   mask_pixel, scale);
                _mm_store_si128(d, result);
            }

            d++;
            src += 4;
            width -= 4;
        }

        dst = reinterpret_cast<SkPMColor*>(d);
    }

    while (width > 0) {
        *dst = SkBlendLCD16(srcA, srcR, srcG, srcB, *dst, *src);
        src++;
        dst++;
        width--;
    }
}

void SkBlitLCD16OpaqueRow_SSE2(SkPMColor dst[], const uint16_t src[],
                               SkColor color, int width, SkPMColor opaqueDst) {
    if (width <= 0) {
        return;
    }

    int srcR = SkColorGetR(color);
    int srcG = SkColorGetG(color);
    int srcB = SkColorGetB(color);

    if (width >= 4) {
        SkASSERT(((size_t)dst & 0x03) == 0);
        while (((size_t)dst & 0x0F) != 0) {
            *dst = SkBlendLCD16Opaque(srcR, srcG, srcB, *dst, *src, opaqueDst);
            src++;
            dst++;
            width--;
        }

        __m128i *d = reinterpret_cast<__m128i*>(dst);
        __m128i srci = _mm_set1_epi32(SkPackARGB32(0xFF, srcR, srcG, srcB));
        srci = _mm_unpacklo_epi8(srci, _mm_setzero_si128());
        while (width >= 4) {
            __m128i dst_pixel = _mm_load_si128(d);
            __m128i mask_pixel = _mm_loadl_epi64(
                                     reinterpret_cast<const __m128i*>(src));

            
            
            
            int pack_cmp = _mm_movemask_epi8(_mm_cmpeq_epi16(mask_pixel,
                                             _mm_setzero_si128()));

            
            if (pack_cmp != 0xFFFF) {
                
                
                mask_pixel = _mm_unpacklo_epi16(mask_pixel,
                                                _mm_setzero_si128());

                
                __m128i result = SkBlendLCD16Opaque_SSE2(srci, dst_pixel,
                                                         mask_pixel);
                _mm_store_si128(d, result);
            }

            d++;
            src += 4;
            width -= 4;
        }

        dst = reinterpret_cast<SkPMColor*>(d);
    }

    while (width > 0) {
        *dst = SkBlendLCD16Opaque(srcR, srcG, srcB, *dst, *src, opaqueDst);
        src++;
        dst++;
        width--;
    }
}
