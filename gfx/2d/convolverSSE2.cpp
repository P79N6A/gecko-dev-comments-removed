



























#include "convolver.h"
#include <algorithm>
#include "skia/SkTypes.h"

#include <emmintrin.h>  

namespace skia {



void ConvolveHorizontally_SSE2(const unsigned char* src_data,
                               int begin, int end,
                               const ConvolutionFilter1D& filter,
                               unsigned char* out_row) {

  int filter_offset, filter_length;
  __m128i zero = _mm_setzero_si128();
  __m128i mask[3];
  
  
  mask[0] = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, -1);
  mask[1] = _mm_set_epi16(0, 0, 0, 0, 0, 0, -1, -1);
  mask[2] = _mm_set_epi16(0, 0, 0, 0, 0, -1, -1, -1);

  
  __m128i buffer;

  
  for (int out_x = begin; out_x < end; out_x++) {
    const ConvolutionFilter1D::Fixed* filter_values =
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

    
    
    
    
    int r = filter_length & 3;
    if (r) {
      memcpy(&buffer, row_to_filter, r * 4);
      
      __m128i coeff, coeff16;
      coeff = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(filter_values));
      
      coeff = _mm_and_si128(coeff, mask[r-1]);
      coeff16 = _mm_shufflelo_epi16(coeff, _MM_SHUFFLE(1, 1, 0, 0));
      coeff16 = _mm_unpacklo_epi16(coeff16, coeff16);

      
      
      __m128i src8 = _mm_loadu_si128(&buffer);
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

    
    accum = _mm_srai_epi32(accum, ConvolutionFilter1D::kShiftBits);

    
    accum = _mm_packs_epi32(accum, zero);
    
    accum = _mm_packus_epi16(accum, zero);

    
    *(reinterpret_cast<int*>(out_row)) = _mm_cvtsi128_si32(accum);
    out_row += 4;
  }
}





void ConvolveHorizontally4_SSE2(const unsigned char* src_data[4],
                                int begin, int end,
                                const ConvolutionFilter1D& filter,
                                unsigned char* out_row[4]) {
  int filter_offset, filter_length;
  __m128i zero = _mm_setzero_si128();
  __m128i mask[3];
  
  
  mask[0] = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, -1);
  mask[1] = _mm_set_epi16(0, 0, 0, 0, 0, 0, -1, -1);
  mask[2] = _mm_set_epi16(0, 0, 0, 0, 0, -1, -1, -1);

  
  for (int out_x = begin; out_x < end; out_x++) {
    const ConvolutionFilter1D::Fixed* filter_values =
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

#define ITERATION(src, accum)                                          \
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
      
      coeff = _mm_and_si128(coeff, mask[r-1]);

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

    accum0 = _mm_srai_epi32(accum0, ConvolutionFilter1D::kShiftBits);
    accum0 = _mm_packs_epi32(accum0, zero);
    accum0 = _mm_packus_epi16(accum0, zero);
    accum1 = _mm_srai_epi32(accum1, ConvolutionFilter1D::kShiftBits);
    accum1 = _mm_packs_epi32(accum1, zero);
    accum1 = _mm_packus_epi16(accum1, zero);
    accum2 = _mm_srai_epi32(accum2, ConvolutionFilter1D::kShiftBits);
    accum2 = _mm_packs_epi32(accum2, zero);
    accum2 = _mm_packus_epi16(accum2, zero);
    accum3 = _mm_srai_epi32(accum3, ConvolutionFilter1D::kShiftBits);
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
void ConvolveVertically_SSE2_impl(const ConvolutionFilter1D::Fixed* filter_values,
                                  int filter_length,
                                  unsigned char* const* source_data_rows,
                                  int begin, int end,
                                  unsigned char* out_row) {
  __m128i zero = _mm_setzero_si128();
  __m128i accum0, accum1, accum2, accum3, coeff16;
  const __m128i* src;
  int out_x;
  
  for (out_x = begin; out_x + 3 < end; out_x += 4) {

    
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

    
    accum0 = _mm_srai_epi32(accum0, ConvolutionFilter1D::kShiftBits);
    accum1 = _mm_srai_epi32(accum1, ConvolutionFilter1D::kShiftBits);
    accum2 = _mm_srai_epi32(accum2, ConvolutionFilter1D::kShiftBits);
    accum3 = _mm_srai_epi32(accum3, ConvolutionFilter1D::kShiftBits);

    
    
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

  
  
  int r = end - out_x;
  if (r > 0) {
    
    __m128i *buffer = &accum3;

    accum0 = _mm_setzero_si128();
    accum1 = _mm_setzero_si128();
    accum2 = _mm_setzero_si128();
    for (int filter_y = 0; filter_y < filter_length; ++filter_y) {
      coeff16 = _mm_set1_epi16(filter_values[filter_y]);
      
      src = reinterpret_cast<const __m128i*>(
          &source_data_rows[filter_y][out_x * 4]);
      memcpy(buffer, src, r * 4);
      __m128i src8 = _mm_loadu_si128(buffer);
      
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

    accum0 = _mm_srai_epi32(accum0, ConvolutionFilter1D::kShiftBits);
    accum1 = _mm_srai_epi32(accum1, ConvolutionFilter1D::kShiftBits);
    accum2 = _mm_srai_epi32(accum2, ConvolutionFilter1D::kShiftBits);
    
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

    for (; out_x < end; out_x++) {
      *(reinterpret_cast<int*>(out_row)) = _mm_cvtsi128_si32(accum0);
      accum0 = _mm_srli_si128(accum0, 4);
      out_row += 4;
    }
  }
}

void ConvolveVertically_SSE2(const ConvolutionFilter1D::Fixed* filter_values,
                             int filter_length,
                             unsigned char* const* source_data_rows,
                             int begin, int end,
                             unsigned char* out_row, bool has_alpha) {
  if (has_alpha) {
    ConvolveVertically_SSE2_impl<true>(filter_values, filter_length,
                                       source_data_rows, begin, end, out_row);
  } else {
    ConvolveVertically_SSE2_impl<false>(filter_values, filter_length,
                                       source_data_rows, begin, end, out_row);
  }
}

}  
