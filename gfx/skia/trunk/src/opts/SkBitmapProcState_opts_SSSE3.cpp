






#include "SkBitmapProcState_opts_SSSE3.h"
#include "SkPaint.h"
#include "SkUtils.h"





#if SK_CPU_SSE_LEVEL >= SK_CPU_SSE_LEVEL_SSSE3

#include <tmmintrin.h>  






namespace {






















inline void PrepareConstantsTwoPixelPairs(const uint32_t* xy,
                                          const __m128i& mask_3FFF,
                                          const __m128i& mask_000F,
                                          const __m128i& sixteen_8bit,
                                          const __m128i& mask_dist_select,
                                          __m128i* all_x_result,
                                          __m128i* sixteen_minus_x,
                                          int* x0,
                                          int* x1) {
    const __m128i xx = _mm_loadu_si128(reinterpret_cast<const __m128i *>(xy));

    
    
    const __m128i x0_wide = _mm_srli_epi32(xx, 18);
    
    const __m128i x1_wide = _mm_and_si128(xx, mask_3FFF);

    _mm_storeu_si128(reinterpret_cast<__m128i *>(x0), x0_wide);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(x1), x1_wide);

    __m128i all_x = _mm_and_si128(_mm_srli_epi32(xx, 14), mask_000F);

    
    all_x = _mm_shuffle_epi8(all_x, mask_dist_select);

    *all_x_result = all_x;
    
    *sixteen_minus_x = _mm_sub_epi8(sixteen_8bit, all_x);
}

















inline void PrepareConstantsTwoPixelPairsDXDY(const uint32_t* xy,
                                              const __m128i& mask_3FFF,
                                              const __m128i& mask_000F,
                                              const __m128i& sixteen_8bit,
                                              const __m128i& mask_dist_select,
                                              __m128i* all_xy_result,
                                              __m128i* sixteen_minus_xy,
                                              int* xy0, int* xy1) {
    const __m128i xy_wide =
                        _mm_loadu_si128(reinterpret_cast<const __m128i *>(xy));

    
    __m128i xy0_wide = _mm_srli_epi32(xy_wide, 18);
    
    xy0_wide =  _mm_shuffle_epi32(xy0_wide, _MM_SHUFFLE(2, 0, 3, 1));
    
    __m128i xy1_wide = _mm_and_si128(xy_wide, mask_3FFF);
    
    xy1_wide = _mm_shuffle_epi32(xy1_wide, _MM_SHUFFLE(2, 0, 3, 1));

    _mm_storeu_si128(reinterpret_cast<__m128i *>(xy0), xy0_wide);
    _mm_storeu_si128(reinterpret_cast<__m128i *>(xy1), xy1_wide);

    
    __m128i all_xy = _mm_and_si128(_mm_srli_epi32(xy_wide, 14), mask_000F);
    
    all_xy = _mm_shuffle_epi32(all_xy, _MM_SHUFFLE(2, 0, 3, 1));
    
    all_xy = _mm_shuffle_epi8(all_xy, mask_dist_select);

    *all_xy_result = all_xy;
    
    *sixteen_minus_xy = _mm_sub_epi8(sixteen_8bit, all_xy);
}








inline __m128i ProcessPixelPairHelper(uint32_t pixel0,
                                      uint32_t pixel1,
                                      uint32_t pixel2,
                                      uint32_t pixel3,
                                      const __m128i& scale_x) {
    __m128i a0, a1, a2, a3;
    
    a0 = _mm_cvtsi32_si128(pixel0);
    a1 = _mm_cvtsi32_si128(pixel1);

    
    
    a0 = _mm_unpacklo_epi8(a0, a1);

    a2 = _mm_cvtsi32_si128(pixel2);
    a3 = _mm_cvtsi32_si128(pixel3);
    
    a2 = _mm_unpacklo_epi8(a2, a3);

    
    
    
    a0 = _mm_unpacklo_epi64(a0, a2);

    
    
    
    
    
    
    return _mm_maddubs_epi16(a0, scale_x);
}







template<bool has_alpha, int scale>
inline __m128i ScaleFourPixels(__m128i* pixels,
                               const __m128i& alpha) {
    
    *pixels = _mm_srli_epi16(*pixels, scale);

    if (has_alpha) {
        
        *pixels = _mm_mullo_epi16(*pixels, alpha);

        
        *pixels = _mm_srli_epi16(*pixels, 8);
    }
    return *pixels;
}















template<bool has_alpha>
inline __m128i ProcessPixelPairZeroSubY(uint32_t pixel0,
                                        uint32_t pixel1,
                                        uint32_t pixel2,
                                        uint32_t pixel3,
                                        const __m128i& scale_x,
                                        const __m128i& alpha) {
    __m128i sum = ProcessPixelPairHelper(pixel0, pixel1, pixel2, pixel3,
                                         scale_x);
    return ScaleFourPixels<has_alpha, 4>(&sum, alpha);
}






template<bool has_alpha>
inline __m128i ProcessOnePixelZeroSubY(uint32_t pixel0,
                                       uint32_t pixel1,
                                       __m128i scale_x,
                                       __m128i alpha) {
    __m128i a0 = _mm_cvtsi32_si128(pixel0);
    __m128i a1 = _mm_cvtsi32_si128(pixel1);

    
    a0 = _mm_unpacklo_epi8(a0, a1);

    
    __m128i sum = _mm_maddubs_epi16(a0, scale_x);

    return ScaleFourPixels<has_alpha, 4>(&sum, alpha);
}










inline __m128i ProcessPixelPair(uint32_t pixel0,
                                uint32_t pixel1,
                                uint32_t pixel2,
                                uint32_t pixel3,
                                const __m128i& scale_x,
                                const __m128i& y) {
    __m128i sum = ProcessPixelPairHelper(pixel0, pixel1, pixel2, pixel3,
                                         scale_x);

    
    
    
    
    
    sum = _mm_mullo_epi16(sum, y);

    return sum;
}




















template<bool has_alpha>
inline __m128i ProcessTwoPixelPairs(const uint32_t* row0,
                                    const uint32_t* row1,
                                    const int* x0,
                                    const int* x1,
                                    const __m128i& scale_x,
                                    const __m128i& all_y,
                                    const __m128i& neg_y,
                                    const __m128i& alpha) {
    __m128i sum0 = ProcessPixelPair(
        row0[x0[0]], row0[x1[0]], row0[x0[1]], row0[x1[1]],
        scale_x, neg_y);
    __m128i sum1 = ProcessPixelPair(
        row1[x0[0]], row1[x1[0]], row1[x0[1]], row1[x1[1]],
        scale_x, all_y);

    
    
    
    
    
    
    
    sum0 = _mm_add_epi16(sum0, sum1);

    return ScaleFourPixels<has_alpha, 8>(&sum0, alpha);
}


template<bool has_alpha>
inline __m128i ProcessTwoPixelPairsDXDY(const uint32_t* row00,
                                        const uint32_t* row01,
                                        const uint32_t* row10,
                                        const uint32_t* row11,
                                        const int* xy0,
                                        const int* xy1,
                                        const __m128i& scale_x,
                                        const __m128i& all_y,
                                        const __m128i& neg_y,
                                        const __m128i& alpha) {
    
    __m128i sum0 = ProcessPixelPair(
        row00[xy0[0]], row00[xy1[0]], row10[xy0[1]], row10[xy1[1]],
        scale_x, neg_y);
    
    __m128i sum1 = ProcessPixelPair(
        row01[xy0[0]], row01[xy1[0]], row11[xy0[1]], row11[xy1[1]],
        scale_x, all_y);

    
    
    
    
    
    
    
    sum0 = _mm_add_epi16(sum0, sum1);

    return ScaleFourPixels<has_alpha, 8>(&sum0, alpha);
}




inline __m128i ProcessOnePixel(uint32_t pixel0, uint32_t pixel1,
                               const __m128i& scale_x, const __m128i& y) {
    __m128i a0 = _mm_cvtsi32_si128(pixel0);
    __m128i a1 = _mm_cvtsi32_si128(pixel1);

    
    
    a0 = _mm_unpacklo_epi8(a0, a1);

    
    a0 = _mm_maddubs_epi16(a0, scale_x);

    
    return _mm_mullo_epi16(a0, y);
}


























template<bool has_alpha>
void S32_generic_D32_filter_DX_SSSE3(const SkBitmapProcState& s,
                                     const uint32_t* xy,
                                     int count, uint32_t* colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fFilterLevel != SkPaint::kNone_FilterLevel);
    SkASSERT(kN32_SkColorType == s.fBitmap->colorType());
    if (has_alpha) {
        SkASSERT(s.fAlphaScale < 256);
    } else {
        SkASSERT(s.fAlphaScale == 256);
    }

    const uint8_t* src_addr =
            static_cast<const uint8_t*>(s.fBitmap->getPixels());
    const size_t rb = s.fBitmap->rowBytes();
    const uint32_t XY = *xy++;
    const unsigned y0 = XY >> 14;
    const uint32_t* row0 =
            reinterpret_cast<const uint32_t*>(src_addr + (y0 >> 4) * rb);
    const uint32_t* row1 =
            reinterpret_cast<const uint32_t*>(src_addr + (XY & 0x3FFF) * rb);
    const unsigned sub_y = y0 & 0xF;

    
    const __m128i mask_dist_select = _mm_set_epi8(12, 12, 12, 12,
                                                  8,  8,  8,  8,
                                                  4,  4,  4,  4,
                                                  0,  0,  0,  0);
    const __m128i mask_3FFF = _mm_set1_epi32(0x3FFF);
    const __m128i mask_000F = _mm_set1_epi32(0x000F);
    const __m128i sixteen_8bit = _mm_set1_epi8(16);
    
    const __m128i zero = _mm_setzero_si128();

    __m128i alpha = _mm_setzero_si128();
    if (has_alpha) {
        
        alpha = _mm_set1_epi16(s.fAlphaScale);
    }

    if (sub_y == 0) {
        
        while (count > 3) {
            count -= 4;

            int x0[4];
            int x1[4];
            __m128i all_x, sixteen_minus_x;
            PrepareConstantsTwoPixelPairs(xy, mask_3FFF, mask_000F,
                                          sixteen_8bit, mask_dist_select,
                                          &all_x, &sixteen_minus_x, x0, x1);
            xy += 4;

            
            
            __m128i scale_x;
            scale_x = _mm_unpacklo_epi8(sixteen_minus_x, all_x);

            __m128i sum0 = ProcessPixelPairZeroSubY<has_alpha>(
                row0[x0[0]], row0[x1[0]], row0[x0[1]], row0[x1[1]],
                scale_x, alpha);

            
            
            scale_x = _mm_unpackhi_epi8(sixteen_minus_x, all_x);

            __m128i sum1 = ProcessPixelPairZeroSubY<has_alpha>(
                row0[x0[2]], row0[x1[2]], row0[x0[3]], row0[x1[3]],
                scale_x, alpha);

            
            sum0 = _mm_packus_epi16(sum0, sum1);

            
            _mm_storeu_si128(reinterpret_cast<__m128i *>(colors), sum0);

            colors += 4;
        }

        
        while (count-- > 0) {
            uint32_t xx = *xy++;  
            unsigned x0 = xx >> 18;
            unsigned x1 = xx & 0x3FFF;

            
            const __m128i all_x = _mm_set1_epi8((xx >> 14) & 0x0F);

            
            __m128i scale_x = _mm_sub_epi8(sixteen_8bit, all_x);

            scale_x = _mm_unpacklo_epi8(scale_x, all_x);

            __m128i sum = ProcessOnePixelZeroSubY<has_alpha>(
                row0[x0], row0[x1],
                scale_x, alpha);

            
            sum = _mm_packus_epi16(sum, zero);

            
            *colors++ = _mm_cvtsi128_si32(sum);
        }
    } else {  
        
        const __m128i sixteen_16bit = _mm_set1_epi16(16);

        
        const __m128i all_y = _mm_set1_epi16(sub_y);

        
        const __m128i neg_y = _mm_sub_epi16(sixteen_16bit, all_y);

        
        while (count > 3) {
            count -= 4;

            int x0[4];
            int x1[4];
            __m128i all_x, sixteen_minus_x;
            PrepareConstantsTwoPixelPairs(xy, mask_3FFF, mask_000F,
                                          sixteen_8bit, mask_dist_select,
                                          &all_x, &sixteen_minus_x, x0, x1);
            xy += 4;

            
            
            __m128i scale_x;
            scale_x = _mm_unpacklo_epi8(sixteen_minus_x, all_x);

            __m128i sum0 = ProcessTwoPixelPairs<has_alpha>(
                row0, row1, x0, x1,
                scale_x, all_y, neg_y, alpha);

            
            
            scale_x = _mm_unpackhi_epi8(sixteen_minus_x, all_x);

            __m128i sum1 = ProcessTwoPixelPairs<has_alpha>(
                row0, row1, x0 + 2, x1 + 2,
                scale_x, all_y, neg_y, alpha);

            

            
            sum0 = _mm_packus_epi16(sum0, sum1);

            
            _mm_storeu_si128(reinterpret_cast<__m128i *>(colors), sum0);

            colors += 4;
        }

        
        while (count-- > 0) {
            const uint32_t xx = *xy++;  
            const unsigned x0 = xx >> 18;
            const unsigned x1 = xx & 0x3FFF;

            
            const __m128i all_x = _mm_set1_epi8((xx >> 14) & 0x0F);

            
            __m128i scale_x = _mm_sub_epi8(sixteen_8bit, all_x);

            
            scale_x = _mm_unpacklo_epi8(scale_x, all_x);

            
            __m128i sum0 = ProcessOnePixel(row0[x0], row0[x1], scale_x, neg_y);
            
            __m128i sum1 = ProcessOnePixel(row1[x0], row1[x1], scale_x, all_y);

            
            sum0 = _mm_add_epi16(sum0, sum1);

            sum0 = ScaleFourPixels<has_alpha, 8>(&sum0, alpha);

            
            sum0 = _mm_packus_epi16(sum0, zero);

            
            *colors++ = _mm_cvtsi128_si32(sum0);
        }
    }
}





template<bool has_alpha>
void S32_generic_D32_filter_DXDY_SSSE3(const SkBitmapProcState& s,
                                       const uint32_t* xy,
                                       int count, uint32_t* colors) {
    SkASSERT(count > 0 && colors != NULL);
    SkASSERT(s.fFilterLevel != SkPaint::kNone_FilterLevel);
    SkASSERT(kN32_SkColorType == s.fBitmap->colorType());
    if (has_alpha) {
        SkASSERT(s.fAlphaScale < 256);
    } else {
        SkASSERT(s.fAlphaScale == 256);
    }

    const uint8_t* src_addr =
                        static_cast<const uint8_t*>(s.fBitmap->getPixels());
    const size_t rb = s.fBitmap->rowBytes();

    
    const __m128i mask_dist_select = _mm_set_epi8(12, 12, 12, 12,
                                                  8,  8,  8,  8,
                                                  4,  4,  4,  4,
                                                  0,  0,  0,  0);
    const __m128i mask_3FFF = _mm_set1_epi32(0x3FFF);
    const __m128i mask_000F = _mm_set1_epi32(0x000F);
    const __m128i sixteen_8bit = _mm_set1_epi8(16);

    __m128i alpha;
    if (has_alpha) {
        
        alpha = _mm_set1_epi16(s.fAlphaScale);
    }

    
    while (count >= 2) {
        int xy0[4];
        int xy1[4];
        __m128i all_xy, sixteen_minus_xy;
        PrepareConstantsTwoPixelPairsDXDY(xy, mask_3FFF, mask_000F,
                                          sixteen_8bit, mask_dist_select,
                                         &all_xy, &sixteen_minus_xy, xy0, xy1);

        
        __m128i scale_x = _mm_unpacklo_epi8(sixteen_minus_xy, all_xy);
        
        __m128i all_y = _mm_unpackhi_epi8(all_xy, _mm_setzero_si128());
        __m128i neg_y = _mm_sub_epi16(_mm_set1_epi16(16), all_y);

        const uint32_t* row00 =
                    reinterpret_cast<const uint32_t*>(src_addr + xy0[2] * rb);
        const uint32_t* row01 =
                    reinterpret_cast<const uint32_t*>(src_addr + xy1[2] * rb);
        const uint32_t* row10 =
                    reinterpret_cast<const uint32_t*>(src_addr + xy0[3] * rb);
        const uint32_t* row11 =
                    reinterpret_cast<const uint32_t*>(src_addr + xy1[3] * rb);

        __m128i sum0 = ProcessTwoPixelPairsDXDY<has_alpha>(
                                        row00, row01, row10, row11, xy0, xy1,
                                        scale_x, all_y, neg_y, alpha);

        
        sum0 = _mm_packus_epi16(sum0, _mm_setzero_si128());

        
        _mm_storel_epi64(reinterpret_cast<__m128i *>(colors), sum0);

        xy += 4;
        colors += 2;
        count -= 2;
    }

    
    while (count-- > 0) {
        uint32_t data = *xy++;
        unsigned y0 = data >> 14;
        unsigned y1 = data & 0x3FFF;
        unsigned subY = y0 & 0xF;
        y0 >>= 4;

        data = *xy++;
        unsigned x0 = data >> 14;
        unsigned x1 = data & 0x3FFF;
        unsigned subX = x0 & 0xF;
        x0 >>= 4;

        const uint32_t* row0 =
                        reinterpret_cast<const uint32_t*>(src_addr + y0 * rb);
        const uint32_t* row1 =
                        reinterpret_cast<const uint32_t*>(src_addr + y1 * rb);

        
        const __m128i all_x = _mm_set1_epi8(subX);

        
        __m128i scale_x = _mm_sub_epi8(sixteen_8bit, all_x);

        
        scale_x = _mm_unpacklo_epi8(scale_x, all_x);

        
        const __m128i sixteen_16bit = _mm_set1_epi16(16);

        
        const __m128i all_y = _mm_set1_epi16(subY);

        
        const __m128i neg_y = _mm_sub_epi16(sixteen_16bit, all_y);

        
        __m128i sum0 = ProcessOnePixel(row0[x0], row0[x1], scale_x, neg_y);
        
        __m128i sum1 = ProcessOnePixel(row1[x0], row1[x1], scale_x, all_y);

        
        sum0 = _mm_add_epi16(sum0, sum1);

        sum0 = ScaleFourPixels<has_alpha, 8>(&sum0, alpha);

        
        sum0 = _mm_packus_epi16(sum0, _mm_setzero_si128());

        
        *colors++ = _mm_cvtsi128_si32(sum0);
    }
}
}  

void S32_opaque_D32_filter_DX_SSSE3(const SkBitmapProcState& s,
                                    const uint32_t* xy,
                                    int count, uint32_t* colors) {
    S32_generic_D32_filter_DX_SSSE3<false>(s, xy, count, colors);
}

void S32_alpha_D32_filter_DX_SSSE3(const SkBitmapProcState& s,
                                   const uint32_t* xy,
                                   int count, uint32_t* colors) {
    S32_generic_D32_filter_DX_SSSE3<true>(s, xy, count, colors);
}

void S32_opaque_D32_filter_DXDY_SSSE3(const SkBitmapProcState& s,
                                    const uint32_t* xy,
                                    int count, uint32_t* colors) {
    S32_generic_D32_filter_DXDY_SSSE3<false>(s, xy, count, colors);
}

void S32_alpha_D32_filter_DXDY_SSSE3(const SkBitmapProcState& s,
                                   const uint32_t* xy,
                                   int count, uint32_t* colors) {
    S32_generic_D32_filter_DXDY_SSSE3<true>(s, xy, count, colors);
}

#else 

void S32_opaque_D32_filter_DX_SSSE3(const SkBitmapProcState& s,
                                    const uint32_t* xy,
                                    int count, uint32_t* colors) {
    sk_throw();
}

void S32_alpha_D32_filter_DX_SSSE3(const SkBitmapProcState& s,
                                   const uint32_t* xy,
                                   int count, uint32_t* colors) {
    sk_throw();
}

void S32_opaque_D32_filter_DXDY_SSSE3(const SkBitmapProcState& s,
                                    const uint32_t* xy,
                                    int count, uint32_t* colors) {
    sk_throw();
}

void S32_alpha_D32_filter_DXDY_SSSE3(const SkBitmapProcState& s,
                                   const uint32_t* xy,
                                   int count, uint32_t* colors) {
    sk_throw();
}

#endif
