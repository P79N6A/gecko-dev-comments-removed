






#include <emmintrin.h>
#include "SkBitmap.h"
#include "SkBitmapFilter_opts_SSE2.h"
#include "SkBitmapProcState.h"
#include "SkColor.h"
#include "SkColorPriv.h"
#include "SkConvolver.h"
#include "SkShader.h"
#include "SkUnPreMultiply.h"

#if 0
static inline void print128i(__m128i value) {
    int *v = (int*) &value;
    printf("% .11d % .11d % .11d % .11d\n", v[0], v[1], v[2], v[3]);
}

static inline void print128i_16(__m128i value) {
    short *v = (short*) &value;
    printf("% .5d % .5d % .5d % .5d % .5d % .5d % .5d % .5d\n", v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
}

static inline void print128i_8(__m128i value) {
    unsigned char *v = (unsigned char*) &value;
    printf("%.3u %.3u %.3u %.3u %.3u %.3u %.3u %.3u %.3u %.3u %.3u %.3u %.3u %.3u %.3u %.3u\n",
           v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7],
           v[8], v[9], v[10], v[11], v[12], v[13], v[14], v[15]
           );
}

static inline void print128f(__m128 value) {
    float *f = (float*) &value;
    printf("%3.4f %3.4f %3.4f %3.4f\n", f[0], f[1], f[2], f[3]);
}
#endif




void highQualityFilter_SSE2(const SkBitmapProcState& s, int x, int y,
                            SkPMColor* SK_RESTRICT colors, int count) {

    const int maxX = s.fBitmap->width() - 1;
    const int maxY = s.fBitmap->height() - 1;

    while (count-- > 0) {
        SkPoint srcPt;
        s.fInvProc(s.fInvMatrix, SkIntToScalar(x),
                    SkIntToScalar(y), &srcPt);
        srcPt.fX -= SK_ScalarHalf;
        srcPt.fY -= SK_ScalarHalf;

        int sx = SkScalarFloorToInt(srcPt.fX);
        int sy = SkScalarFloorToInt(srcPt.fY);

        __m128 weight = _mm_setzero_ps();
        __m128 accum = _mm_setzero_ps();

        int y0 = SkTMax(0, int(ceil(sy-s.getBitmapFilter()->width() + 0.5f)));
        int y1 = SkTMin(maxY, int(floor(sy+s.getBitmapFilter()->width() + 0.5f)));
        int x0 = SkTMax(0, int(ceil(sx-s.getBitmapFilter()->width() + 0.5f)));
        int x1 = SkTMin(maxX, int(floor(sx+s.getBitmapFilter()->width() + 0.5f)));

        for (int src_y = y0; src_y <= y1; src_y++) {
            float yweight = SkScalarToFloat(s.getBitmapFilter()->lookupScalar(srcPt.fY - src_y));

            for (int src_x = x0; src_x <= x1 ; src_x++) {
                float xweight = SkScalarToFloat(s.getBitmapFilter()->lookupScalar(srcPt.fX - src_x));

                float combined_weight = xweight * yweight;

                SkPMColor color = *s.fBitmap->getAddr32(src_x, src_y);

                __m128i c = _mm_cvtsi32_si128( color );
                c = _mm_unpacklo_epi8(c, _mm_setzero_si128());
                c = _mm_unpacklo_epi16(c, _mm_setzero_si128());

                __m128 cfloat = _mm_cvtepi32_ps( c );

                __m128 weightVector = _mm_set1_ps(combined_weight);

                accum = _mm_add_ps(accum, _mm_mul_ps(cfloat, weightVector));
                weight = _mm_add_ps( weight, weightVector );
            }
        }

        accum = _mm_div_ps(accum, weight);
        accum = _mm_add_ps(accum, _mm_set1_ps(0.5f));

        __m128i accumInt = _mm_cvtps_epi32( accum );

        int localResult[4];
        _mm_storeu_si128((__m128i *) (localResult), accumInt);
        int a = SkClampMax(localResult[0], 255);
        int r = SkClampMax(localResult[1], a);
        int g = SkClampMax(localResult[2], a);
        int b = SkClampMax(localResult[3], a);

        *colors++ = SkPackARGB32(a, r, g, b);

        x++;
    }
}

void highQualityFilter_ScaleOnly_SSE2(const SkBitmapProcState &s, int x, int y,
                             SkPMColor *SK_RESTRICT colors, int count) {
    const int maxX = s.fBitmap->width() - 1;
    const int maxY = s.fBitmap->height() - 1;

    SkPoint srcPt;
    s.fInvProc(s.fInvMatrix, SkIntToScalar(x),
                SkIntToScalar(y), &srcPt);
    srcPt.fY -= SK_ScalarHalf;
    int sy = SkScalarFloorToInt(srcPt.fY);

    int y0 = SkTMax(0, int(ceil(sy-s.getBitmapFilter()->width() + 0.5f)));
    int y1 = SkTMin(maxY, int(floor(sy+s.getBitmapFilter()->width() + 0.5f)));

    while (count-- > 0) {
        srcPt.fX -= SK_ScalarHalf;
        srcPt.fY -= SK_ScalarHalf;

        int sx = SkScalarFloorToInt(srcPt.fX);

        float weight = 0;
        __m128 accum = _mm_setzero_ps();

        int x0 = SkTMax(0, int(ceil(sx-s.getBitmapFilter()->width() + 0.5f)));
        int x1 = SkTMin(maxX, int(floor(sx+s.getBitmapFilter()->width() + 0.5f)));

        for (int src_y = y0; src_y <= y1; src_y++) {
            float yweight = SkScalarToFloat(s.getBitmapFilter()->lookupScalar(srcPt.fY - src_y));

            for (int src_x = x0; src_x <= x1 ; src_x++) {
                float xweight = SkScalarToFloat(s.getBitmapFilter()->lookupScalar(srcPt.fX - src_x));

                float combined_weight = xweight * yweight;

                SkPMColor color = *s.fBitmap->getAddr32(src_x, src_y);

                __m128 c = _mm_set_ps((float)SkGetPackedB32(color),
                                      (float)SkGetPackedG32(color),
                                      (float)SkGetPackedR32(color),
                                      (float)SkGetPackedA32(color));

                __m128 weightVector = _mm_set1_ps(combined_weight);

                accum = _mm_add_ps(accum, _mm_mul_ps(c, weightVector));
                weight += combined_weight;
            }
        }

        __m128 totalWeightVector = _mm_set1_ps(weight);
        accum = _mm_div_ps(accum, totalWeightVector);
        accum = _mm_add_ps(accum, _mm_set1_ps(0.5f));

        float localResult[4];
        _mm_storeu_ps(localResult, accum);
        int a = SkClampMax(int(localResult[0]), 255);
        int r = SkClampMax(int(localResult[1]), a);
        int g = SkClampMax(int(localResult[2]), a);
        int b = SkClampMax(int(localResult[3]), a);

        *colors++ = SkPackARGB32(a, r, g, b);

        x++;

        s.fInvProc(s.fInvMatrix, SkIntToScalar(x),
                    SkIntToScalar(y), &srcPt);
    }
}



void convolveHorizontally_SSE2(const unsigned char* src_data,
                               const SkConvolutionFilter1D& filter,
                               unsigned char* out_row,
                               bool ) {
    int num_values = filter.numValues();

    int filter_offset, filter_length;
    __m128i zero = _mm_setzero_si128();
    __m128i mask[4];
    
    
    
    mask[1] = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, -1);
    mask[2] = _mm_set_epi16(0, 0, 0, 0, 0, 0, -1, -1);
    mask[3] = _mm_set_epi16(0, 0, 0, 0, 0, -1, -1, -1);

    
    for (int out_x = 0; out_x < num_values; out_x++) {
        const SkConvolutionFilter1D::ConvolutionFixed* filter_values =
            filter.FilterForValue(out_x, &filter_offset, &filter_length);

        __m128i accum = _mm_setzero_si128();

        
        
        const __m128i* row_to_filter =
            reinterpret_cast<const __m128i*>(&src_data[filter_offset << 2]);

        
        for (int filter_x = 0; filter_x < filter_length >> 2; filter_x++) {

            
            __m128i coeff, coeff16;
            
            coeff = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(filter_values));
            
            coeff16 = _mm_shufflelo_epi16(coeff, _MM_SHUFFLE(1, 1, 0, 0));
            
            coeff16 = _mm_unpacklo_epi16(coeff16, coeff16);

            
            
            
            __m128i src8 = _mm_loadu_si128(row_to_filter);
            
            __m128i src16 = _mm_unpacklo_epi8(src8, zero);
            __m128i mul_hi = _mm_mulhi_epi16(src16, coeff16);
            __m128i mul_lo = _mm_mullo_epi16(src16, coeff16);
            
            __m128i t = _mm_unpacklo_epi16(mul_lo, mul_hi);
            accum = _mm_add_epi32(accum, t);
            
            t = _mm_unpackhi_epi16(mul_lo, mul_hi);
            accum = _mm_add_epi32(accum, t);

            
            
            
            
            coeff16 = _mm_shufflelo_epi16(coeff, _MM_SHUFFLE(3, 3, 2, 2));
            
            coeff16 = _mm_unpacklo_epi16(coeff16, coeff16);
            
            src16 = _mm_unpackhi_epi8(src8, zero);
            mul_hi = _mm_mulhi_epi16(src16, coeff16);
            mul_lo = _mm_mullo_epi16(src16, coeff16);
            
            t = _mm_unpacklo_epi16(mul_lo, mul_hi);
            accum = _mm_add_epi32(accum, t);
            
            t = _mm_unpackhi_epi16(mul_lo, mul_hi);
            accum = _mm_add_epi32(accum, t);

            
            row_to_filter += 1;
            filter_values += 4;
        }

        
        
        
        
        int r = filter_length&3;
        if (r) {
            
            __m128i coeff, coeff16;
            coeff = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(filter_values));
            
            coeff = _mm_and_si128(coeff, mask[r]);
            coeff16 = _mm_shufflelo_epi16(coeff, _MM_SHUFFLE(1, 1, 0, 0));
            coeff16 = _mm_unpacklo_epi16(coeff16, coeff16);

            
            
            __m128i src8 = _mm_loadu_si128(row_to_filter);
            __m128i src16 = _mm_unpacklo_epi8(src8, zero);
            __m128i mul_hi = _mm_mulhi_epi16(src16, coeff16);
            __m128i mul_lo = _mm_mullo_epi16(src16, coeff16);
            __m128i t = _mm_unpacklo_epi16(mul_lo, mul_hi);
            accum = _mm_add_epi32(accum, t);
            t = _mm_unpackhi_epi16(mul_lo, mul_hi);
            accum = _mm_add_epi32(accum, t);

            src16 = _mm_unpackhi_epi8(src8, zero);
            coeff16 = _mm_shufflelo_epi16(coeff, _MM_SHUFFLE(3, 3, 2, 2));
            coeff16 = _mm_unpacklo_epi16(coeff16, coeff16);
            mul_hi = _mm_mulhi_epi16(src16, coeff16);
            mul_lo = _mm_mullo_epi16(src16, coeff16);
            t = _mm_unpacklo_epi16(mul_lo, mul_hi);
            accum = _mm_add_epi32(accum, t);
        }

        
        accum = _mm_srai_epi32(accum, SkConvolutionFilter1D::kShiftBits);

        
        accum = _mm_packs_epi32(accum, zero);
        
        accum = _mm_packus_epi16(accum, zero);

        
        *(reinterpret_cast<int*>(out_row)) = _mm_cvtsi128_si32(accum);
        out_row += 4;
    }
}





void convolve4RowsHorizontally_SSE2(const unsigned char* src_data[4],
                                    const SkConvolutionFilter1D& filter,
                                    unsigned char* out_row[4]) {
    int num_values = filter.numValues();

    int filter_offset, filter_length;
    __m128i zero = _mm_setzero_si128();
    __m128i mask[4];
    
    
    
    mask[1] = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, -1);
    mask[2] = _mm_set_epi16(0, 0, 0, 0, 0, 0, -1, -1);
    mask[3] = _mm_set_epi16(0, 0, 0, 0, 0, -1, -1, -1);

    
    for (int out_x = 0; out_x < num_values; out_x++) {
        const SkConvolutionFilter1D::ConvolutionFixed* filter_values =
            filter.FilterForValue(out_x, &filter_offset, &filter_length);

        
        __m128i accum0 = _mm_setzero_si128();
        __m128i accum1 = _mm_setzero_si128();
        __m128i accum2 = _mm_setzero_si128();
        __m128i accum3 = _mm_setzero_si128();
        int start = (filter_offset<<2);
        
        for (int filter_x = 0; filter_x < (filter_length >> 2); filter_x++) {
            __m128i coeff, coeff16lo, coeff16hi;
            
            coeff = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(filter_values));
            
            coeff16lo = _mm_shufflelo_epi16(coeff, _MM_SHUFFLE(1, 1, 0, 0));
            
            coeff16lo = _mm_unpacklo_epi16(coeff16lo, coeff16lo);
            
            coeff16hi = _mm_shufflelo_epi16(coeff, _MM_SHUFFLE(3, 3, 2, 2));
            
            coeff16hi = _mm_unpacklo_epi16(coeff16hi, coeff16hi);

            __m128i src8, src16, mul_hi, mul_lo, t;

#define ITERATION(src, accum)                                                \
            src8 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src));   \
            src16 = _mm_unpacklo_epi8(src8, zero);                           \
            mul_hi = _mm_mulhi_epi16(src16, coeff16lo);                      \
            mul_lo = _mm_mullo_epi16(src16, coeff16lo);                      \
            t = _mm_unpacklo_epi16(mul_lo, mul_hi);                          \
            accum = _mm_add_epi32(accum, t);                                 \
            t = _mm_unpackhi_epi16(mul_lo, mul_hi);                          \
            accum = _mm_add_epi32(accum, t);                                 \
            src16 = _mm_unpackhi_epi8(src8, zero);                           \
            mul_hi = _mm_mulhi_epi16(src16, coeff16hi);                      \
            mul_lo = _mm_mullo_epi16(src16, coeff16hi);                      \
            t = _mm_unpacklo_epi16(mul_lo, mul_hi);                          \
            accum = _mm_add_epi32(accum, t);                                 \
            t = _mm_unpackhi_epi16(mul_lo, mul_hi);                          \
            accum = _mm_add_epi32(accum, t)

            ITERATION(src_data[0] + start, accum0);
            ITERATION(src_data[1] + start, accum1);
            ITERATION(src_data[2] + start, accum2);
            ITERATION(src_data[3] + start, accum3);

            start += 16;
            filter_values += 4;
        }

        int r = filter_length & 3;
        if (r) {
            
            __m128i coeff;
            coeff = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(filter_values));
            
            coeff = _mm_and_si128(coeff, mask[r]);

            __m128i coeff16lo = _mm_shufflelo_epi16(coeff, _MM_SHUFFLE(1, 1, 0, 0));
            
            coeff16lo = _mm_unpacklo_epi16(coeff16lo, coeff16lo);
            __m128i coeff16hi = _mm_shufflelo_epi16(coeff, _MM_SHUFFLE(3, 3, 2, 2));
            coeff16hi = _mm_unpacklo_epi16(coeff16hi, coeff16hi);

            __m128i src8, src16, mul_hi, mul_lo, t;

            ITERATION(src_data[0] + start, accum0);
            ITERATION(src_data[1] + start, accum1);
            ITERATION(src_data[2] + start, accum2);
            ITERATION(src_data[3] + start, accum3);
        }

        accum0 = _mm_srai_epi32(accum0, SkConvolutionFilter1D::kShiftBits);
        accum0 = _mm_packs_epi32(accum0, zero);
        accum0 = _mm_packus_epi16(accum0, zero);
        accum1 = _mm_srai_epi32(accum1, SkConvolutionFilter1D::kShiftBits);
        accum1 = _mm_packs_epi32(accum1, zero);
        accum1 = _mm_packus_epi16(accum1, zero);
        accum2 = _mm_srai_epi32(accum2, SkConvolutionFilter1D::kShiftBits);
        accum2 = _mm_packs_epi32(accum2, zero);
        accum2 = _mm_packus_epi16(accum2, zero);
        accum3 = _mm_srai_epi32(accum3, SkConvolutionFilter1D::kShiftBits);
        accum3 = _mm_packs_epi32(accum3, zero);
        accum3 = _mm_packus_epi16(accum3, zero);

        *(reinterpret_cast<int*>(out_row[0])) = _mm_cvtsi128_si32(accum0);
        *(reinterpret_cast<int*>(out_row[1])) = _mm_cvtsi128_si32(accum1);
        *(reinterpret_cast<int*>(out_row[2])) = _mm_cvtsi128_si32(accum2);
        *(reinterpret_cast<int*>(out_row[3])) = _mm_cvtsi128_si32(accum3);

        out_row[0] += 4;
        out_row[1] += 4;
        out_row[2] += 4;
        out_row[3] += 4;
    }
}







template<bool has_alpha>
void convolveVertically_SSE2(const SkConvolutionFilter1D::ConvolutionFixed* filter_values,
                             int filter_length,
                             unsigned char* const* source_data_rows,
                             int pixel_width,
                             unsigned char* out_row) {
    int width = pixel_width & ~3;

    __m128i zero = _mm_setzero_si128();
    __m128i accum0, accum1, accum2, accum3, coeff16;
    const __m128i* src;
    
    for (int out_x = 0; out_x < width; out_x += 4) {

        
        accum0 = _mm_setzero_si128();
        accum1 = _mm_setzero_si128();
        accum2 = _mm_setzero_si128();
        accum3 = _mm_setzero_si128();

        
        for (int filter_y = 0; filter_y < filter_length; filter_y++) {

            
            
            coeff16 = _mm_set1_epi16(filter_values[filter_y]);

            
            
            src = reinterpret_cast<const __m128i*>(
                &source_data_rows[filter_y][out_x << 2]);
            __m128i src8 = _mm_loadu_si128(src);

            
            
            
            __m128i src16 = _mm_unpacklo_epi8(src8, zero);
            __m128i mul_hi = _mm_mulhi_epi16(src16, coeff16);
            __m128i mul_lo = _mm_mullo_epi16(src16, coeff16);
            
            __m128i t = _mm_unpacklo_epi16(mul_lo, mul_hi);
            accum0 = _mm_add_epi32(accum0, t);
            
            t = _mm_unpackhi_epi16(mul_lo, mul_hi);
            accum1 = _mm_add_epi32(accum1, t);

            
            
            
            src16 = _mm_unpackhi_epi8(src8, zero);
            mul_hi = _mm_mulhi_epi16(src16, coeff16);
            mul_lo = _mm_mullo_epi16(src16, coeff16);
            
            t = _mm_unpacklo_epi16(mul_lo, mul_hi);
            accum2 = _mm_add_epi32(accum2, t);
            
            t = _mm_unpackhi_epi16(mul_lo, mul_hi);
            accum3 = _mm_add_epi32(accum3, t);
        }

        
        accum0 = _mm_srai_epi32(accum0, SkConvolutionFilter1D::kShiftBits);
        accum1 = _mm_srai_epi32(accum1, SkConvolutionFilter1D::kShiftBits);
        accum2 = _mm_srai_epi32(accum2, SkConvolutionFilter1D::kShiftBits);
        accum3 = _mm_srai_epi32(accum3, SkConvolutionFilter1D::kShiftBits);

        
        
        accum0 = _mm_packs_epi32(accum0, accum1);
        
        accum2 = _mm_packs_epi32(accum2, accum3);

        
        
        accum0 = _mm_packus_epi16(accum0, accum2);

        if (has_alpha) {
            
            
            __m128i a = _mm_srli_epi32(accum0, 8);
            
            __m128i b = _mm_max_epu8(a, accum0);  
            
            a = _mm_srli_epi32(accum0, 16);
            
            b = _mm_max_epu8(a, b);  
            
            b = _mm_slli_epi32(b, 24);

            
            
            accum0 = _mm_max_epu8(b, accum0);
        } else {
            
            __m128i mask = _mm_set1_epi32(0xff000000);
            accum0 = _mm_or_si128(accum0, mask);
        }

        
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out_row), accum0);
        out_row += 16;
    }

    
    
    if (pixel_width & 3) {
        accum0 = _mm_setzero_si128();
        accum1 = _mm_setzero_si128();
        accum2 = _mm_setzero_si128();
        for (int filter_y = 0; filter_y < filter_length; ++filter_y) {
            coeff16 = _mm_set1_epi16(filter_values[filter_y]);
            
            src = reinterpret_cast<const __m128i*>(
                &source_data_rows[filter_y][width<<2]);
            __m128i src8 = _mm_loadu_si128(src);
            
            __m128i src16 = _mm_unpacklo_epi8(src8, zero);
            __m128i mul_hi = _mm_mulhi_epi16(src16, coeff16);
            __m128i mul_lo = _mm_mullo_epi16(src16, coeff16);
            
            __m128i t = _mm_unpacklo_epi16(mul_lo, mul_hi);
            accum0 = _mm_add_epi32(accum0, t);
            
            t = _mm_unpackhi_epi16(mul_lo, mul_hi);
            accum1 = _mm_add_epi32(accum1, t);
            
            src16 = _mm_unpackhi_epi8(src8, zero);
            mul_hi = _mm_mulhi_epi16(src16, coeff16);
            mul_lo = _mm_mullo_epi16(src16, coeff16);
            
            t = _mm_unpacklo_epi16(mul_lo, mul_hi);
            accum2 = _mm_add_epi32(accum2, t);
        }

        accum0 = _mm_srai_epi32(accum0, SkConvolutionFilter1D::kShiftBits);
        accum1 = _mm_srai_epi32(accum1, SkConvolutionFilter1D::kShiftBits);
        accum2 = _mm_srai_epi32(accum2, SkConvolutionFilter1D::kShiftBits);
        
        accum0 = _mm_packs_epi32(accum0, accum1);
        
        accum2 = _mm_packs_epi32(accum2, zero);
        
        accum0 = _mm_packus_epi16(accum0, accum2);
        if (has_alpha) {
            
            __m128i a = _mm_srli_epi32(accum0, 8);
            
            __m128i b = _mm_max_epu8(a, accum0);  
            
            a = _mm_srli_epi32(accum0, 16);
            
            b = _mm_max_epu8(a, b);  
            
            b = _mm_slli_epi32(b, 24);
            accum0 = _mm_max_epu8(b, accum0);
        } else {
            __m128i mask = _mm_set1_epi32(0xff000000);
            accum0 = _mm_or_si128(accum0, mask);
        }

        for (int out_x = width; out_x < pixel_width; out_x++) {
            *(reinterpret_cast<int*>(out_row)) = _mm_cvtsi128_si32(accum0);
            accum0 = _mm_srli_si128(accum0, 4);
            out_row += 4;
        }
    }
}

void convolveVertically_SSE2(const SkConvolutionFilter1D::ConvolutionFixed* filter_values,
                             int filter_length,
                             unsigned char* const* source_data_rows,
                             int pixel_width,
                             unsigned char* out_row,
                             bool has_alpha) {
    if (has_alpha) {
        convolveVertically_SSE2<true>(filter_values,
                                      filter_length,
                                      source_data_rows,
                                      pixel_width,
                                      out_row);
    } else {
        convolveVertically_SSE2<false>(filter_values,
                                       filter_length,
                                       source_data_rows,
                                       pixel_width,
                                       out_row);
    }
}

void applySIMDPadding_SSE2(SkConvolutionFilter1D *filter) {
    
    
    
    
    
    for (int i = 0; i < 8; ++i) {
        filter->addFilterValue(static_cast<SkConvolutionFilter1D::ConvolutionFixed>(0));
    }
}
