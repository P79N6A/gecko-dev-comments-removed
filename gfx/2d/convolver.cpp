



























#include "convolver.h"

#include <algorithm>
#include "nsAlgorithm.h"

#include "skia/SkTypes.h"



#if defined(SIMD_SSE2)
#include <emmintrin.h>  
#endif

#if defined(SK_CPU_LENDIAN)
#define R_OFFSET_IDX 0
#define G_OFFSET_IDX 1
#define B_OFFSET_IDX 2
#define A_OFFSET_IDX 3
#else
#define R_OFFSET_IDX 3
#define G_OFFSET_IDX 2
#define B_OFFSET_IDX 1
#define A_OFFSET_IDX 0
#endif

namespace skia {

namespace {



inline unsigned char ClampTo8(int a) {
  if (static_cast<unsigned>(a) < 256)
    return a;  
  if (a < 0)
    return 0;
  return 255;
}




class CircularRowBuffer {
 public:
  
  
  
  
  
  
  CircularRowBuffer(int dest_row_pixel_width, int max_y_filter_size,
                    int first_input_row)
      : row_byte_width_(dest_row_pixel_width * 4),
        num_rows_(max_y_filter_size),
        next_row_(0),
        next_row_coordinate_(first_input_row) {
    buffer_.resize(row_byte_width_ * max_y_filter_size);
    row_addresses_.resize(num_rows_);
  }

  
  
  unsigned char* AdvanceRow() {
    unsigned char* row = &buffer_[next_row_ * row_byte_width_];
    next_row_coordinate_++;

    
    next_row_++;
    if (next_row_ == num_rows_)
      next_row_ = 0;
    return row;
  }

  
  
  
  
  
  
  unsigned char* const* GetRowAddresses(int* first_row_index) {
    
    
    
    
    
    
    
    
    
    
    *first_row_index = next_row_coordinate_ - num_rows_;

    int cur_row = next_row_;
    for (int i = 0; i < num_rows_; i++) {
      row_addresses_[i] = &buffer_[cur_row * row_byte_width_];

      
      cur_row++;
      if (cur_row == num_rows_)
        cur_row = 0;
    }
    return &row_addresses_[0];
  }

 private:
  
  std::vector<unsigned char> buffer_;

  
  int row_byte_width_;

  
  int num_rows_;

  
  
  int next_row_;

  
  
  int next_row_coordinate_;

  
  std::vector<unsigned char*> row_addresses_;
};



template<bool has_alpha>
void ConvolveHorizontally(const unsigned char* src_data,
                          const ConvolutionFilter1D& filter,
                          unsigned char* out_row) {
  
  int num_values = filter.num_values();
  for (int out_x = 0; out_x < num_values; out_x++) {
    
    int filter_offset, filter_length;
    const ConvolutionFilter1D::Fixed* filter_values =
        filter.FilterForValue(out_x, &filter_offset, &filter_length);

    
    
    const unsigned char* row_to_filter = &src_data[filter_offset * 4];

    
    int accum[4] = {0};
    for (int filter_x = 0; filter_x < filter_length; filter_x++) {
      ConvolutionFilter1D::Fixed cur_filter = filter_values[filter_x];
      accum[0] += cur_filter * row_to_filter[filter_x * 4 + R_OFFSET_IDX];
      accum[1] += cur_filter * row_to_filter[filter_x * 4 + G_OFFSET_IDX];
      accum[2] += cur_filter * row_to_filter[filter_x * 4 + B_OFFSET_IDX];
      if (has_alpha)
        accum[3] += cur_filter * row_to_filter[filter_x * 4 + A_OFFSET_IDX];
    }

    
    
    accum[0] >>= ConvolutionFilter1D::kShiftBits;
    accum[1] >>= ConvolutionFilter1D::kShiftBits;
    accum[2] >>= ConvolutionFilter1D::kShiftBits;
    if (has_alpha)
      accum[3] >>= ConvolutionFilter1D::kShiftBits;

    
    out_row[out_x * 4 + R_OFFSET_IDX] = ClampTo8(accum[0]);
    out_row[out_x * 4 + G_OFFSET_IDX] = ClampTo8(accum[1]);
    out_row[out_x * 4 + B_OFFSET_IDX] = ClampTo8(accum[2]);
    if (has_alpha)
      out_row[out_x * 4 + A_OFFSET_IDX] = ClampTo8(accum[3]);
  }
}







template<bool has_alpha>
void ConvolveVertically(const ConvolutionFilter1D::Fixed* filter_values,
                        int filter_length,
                        unsigned char* const* source_data_rows,
                        int pixel_width,
                        unsigned char* out_row) {
  
  
  for (int out_x = 0; out_x < pixel_width; out_x++) {
    
    
    int byte_offset = out_x * 4;

    
    int accum[4] = {0};
    for (int filter_y = 0; filter_y < filter_length; filter_y++) {
      ConvolutionFilter1D::Fixed cur_filter = filter_values[filter_y];
      accum[0] += cur_filter 
	* source_data_rows[filter_y][byte_offset + R_OFFSET_IDX];
      accum[1] += cur_filter 
	* source_data_rows[filter_y][byte_offset + G_OFFSET_IDX];
      accum[2] += cur_filter 
	* source_data_rows[filter_y][byte_offset + B_OFFSET_IDX];
      if (has_alpha)
        accum[3] += cur_filter 
	  * source_data_rows[filter_y][byte_offset + A_OFFSET_IDX];
    }

    
    
    accum[0] >>= ConvolutionFilter1D::kShiftBits;
    accum[1] >>= ConvolutionFilter1D::kShiftBits;
    accum[2] >>= ConvolutionFilter1D::kShiftBits;
    if (has_alpha)
      accum[3] >>= ConvolutionFilter1D::kShiftBits;

    
    out_row[byte_offset + R_OFFSET_IDX] = ClampTo8(accum[0]);
    out_row[byte_offset + G_OFFSET_IDX] = ClampTo8(accum[1]);
    out_row[byte_offset + B_OFFSET_IDX] = ClampTo8(accum[2]);
    if (has_alpha) {
      unsigned char alpha = ClampTo8(accum[3]);

      
      
      
      
      
      
      
      int max_color_channel = std::max(out_row[byte_offset + R_OFFSET_IDX],
          std::max(out_row[byte_offset + G_OFFSET_IDX], out_row[byte_offset + B_OFFSET_IDX]));
      if (alpha < max_color_channel)
        out_row[byte_offset + A_OFFSET_IDX] = max_color_channel;
      else
        out_row[byte_offset + A_OFFSET_IDX] = alpha;
    } else {
      
      out_row[byte_offset + A_OFFSET_IDX] = 0xff;
    }
  }
}




void ConvolveHorizontally_SSE2(const unsigned char* src_data,
                               const ConvolutionFilter1D& filter,
                               unsigned char* out_row) {
#if defined(SIMD_SSE2)
  int num_values = filter.num_values();

  int filter_offset, filter_length;
  __m128i zero = _mm_setzero_si128();
  __m128i mask[4];
  
  
  
  mask[1] = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, -1);
  mask[2] = _mm_set_epi16(0, 0, 0, 0, 0, 0, -1, -1);
  mask[3] = _mm_set_epi16(0, 0, 0, 0, 0, -1, -1, -1);

  
  for (int out_x = 0; out_x < num_values; out_x++) {
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

    
    accum = _mm_srai_epi32(accum, ConvolutionFilter1D::kShiftBits);

    
    accum = _mm_packs_epi32(accum, zero);
    
    accum = _mm_packus_epi16(accum, zero);

    
    *(reinterpret_cast<int*>(out_row)) = _mm_cvtsi128_si32(accum);
    out_row += 4;
  }
#endif
}





void ConvolveHorizontally4_SSE2(const unsigned char* src_data[4],
                                const ConvolutionFilter1D& filter,
                                unsigned char* out_row[4]) {
#if defined(SIMD_SSE2)
  int num_values = filter.num_values();

  int filter_offset, filter_length;
  __m128i zero = _mm_setzero_si128();
  __m128i mask[4];
  
  
  
  mask[1] = _mm_set_epi16(0, 0, 0, 0, 0, 0, 0, -1);
  mask[2] = _mm_set_epi16(0, 0, 0, 0, 0, 0, -1, -1);
  mask[3] = _mm_set_epi16(0, 0, 0, 0, 0, -1, -1, -1);

  
  for (int out_x = 0; out_x < num_values; out_x++) {
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
#endif
}







template<bool has_alpha>
void ConvolveVertically_SSE2(const ConvolutionFilter1D::Fixed* filter_values,
                             int filter_length,
                             unsigned char* const* source_data_rows,
                             int pixel_width,
                             unsigned char* out_row) {
#if defined(SIMD_SSE2)
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

    for (int out_x = width; out_x < pixel_width; out_x++) {
      *(reinterpret_cast<int*>(out_row)) = _mm_cvtsi128_si32(accum0);
      accum0 = _mm_srli_si128(accum0, 4);
      out_row += 4;
    }
  }
#endif
}

}  



ConvolutionFilter1D::ConvolutionFilter1D()
    : max_filter_(0) {
}

ConvolutionFilter1D::~ConvolutionFilter1D() {
}

void ConvolutionFilter1D::AddFilter(int filter_offset,
                                    const float* filter_values,
                                    int filter_length) {
  SkASSERT(filter_length > 0);

  std::vector<Fixed> fixed_values;
  fixed_values.reserve(filter_length);

  for (int i = 0; i < filter_length; ++i)
    fixed_values.push_back(FloatToFixed(filter_values[i]));

  AddFilter(filter_offset, &fixed_values[0], filter_length);
}

void ConvolutionFilter1D::AddFilter(int filter_offset,
                                    const Fixed* filter_values,
                                    int filter_length) {
  
  
  
  
  int first_non_zero = 0;
  while (first_non_zero < filter_length && filter_values[first_non_zero] == 0)
    first_non_zero++;

  if (first_non_zero < filter_length) {
    
    int last_non_zero = filter_length - 1;
    while (last_non_zero >= 0 && filter_values[last_non_zero] == 0)
      last_non_zero--;

    filter_offset += first_non_zero;
    filter_length = last_non_zero + 1 - first_non_zero;
    SkASSERT(filter_length > 0);

    for (int i = first_non_zero; i <= last_non_zero; i++)
      filter_values_.push_back(filter_values[i]);
  } else {
    
    filter_length = 0;
  }

  FilterInstance instance;

  
  instance.data_location = (static_cast<int>(filter_values_.size()) -
                            filter_length);
  instance.offset = filter_offset;
  instance.length = filter_length;
  filters_.push_back(instance);

  max_filter_ = std::max(max_filter_, filter_length);
}

void BGRAConvolve2D(const unsigned char* source_data,
                    int source_byte_row_stride,
                    bool source_has_alpha,
                    const ConvolutionFilter1D& filter_x,
                    const ConvolutionFilter1D& filter_y,
                    int output_byte_row_stride,
                    unsigned char* output,
                    bool use_sse2) {
#if !defined(SIMD_SSE2)
  
  
  use_sse2 = false;
#endif

  int max_y_filter_size = filter_y.max_filter();

  
  
  
  
  
  int filter_offset, filter_length;
  const ConvolutionFilter1D::Fixed* filter_values =
      filter_y.FilterForValue(0, &filter_offset, &filter_length);
  int next_x_row = filter_offset;

  
  
  
  
  
  
  
  
  
  
  int row_buffer_width = (filter_x.num_values() + 15) & ~0xF;
  int row_buffer_height = max_y_filter_size + (use_sse2 ? 4 : 0);
  CircularRowBuffer row_buffer(row_buffer_width,
                               row_buffer_height,
                               filter_offset);

  
  
  SkASSERT(output_byte_row_stride >= filter_x.num_values() * 4);
  int num_output_rows = filter_y.num_values();

  
  
  int last_filter_offset, last_filter_length;
  filter_y.FilterForValue(num_output_rows - 1, &last_filter_offset,
                          &last_filter_length);

  for (int out_y = 0; out_y < num_output_rows; out_y++) {
    filter_values = filter_y.FilterForValue(out_y,
                                            &filter_offset, &filter_length);

    
    if (use_sse2) {
      while (next_x_row < filter_offset + filter_length) {
        if (next_x_row + 3 < last_filter_offset + last_filter_length - 1) {
          const unsigned char* src[4];
          unsigned char* out_row[4];
          for (int i = 0; i < 4; ++i) {
            src[i] = &source_data[(next_x_row + i) * source_byte_row_stride];
            out_row[i] = row_buffer.AdvanceRow();
          }
          ConvolveHorizontally4_SSE2(src, filter_x, out_row);
          next_x_row += 4;
        } else {
          
          
          if (next_x_row == last_filter_offset + last_filter_length - 1) {
            if (source_has_alpha) {
              ConvolveHorizontally<true>(
                  &source_data[next_x_row * source_byte_row_stride],
                  filter_x, row_buffer.AdvanceRow());
            } else {
              ConvolveHorizontally<false>(
                  &source_data[next_x_row * source_byte_row_stride],
                  filter_x, row_buffer.AdvanceRow());
            }
          } else {
            ConvolveHorizontally_SSE2(
                &source_data[next_x_row * source_byte_row_stride],
                filter_x, row_buffer.AdvanceRow());
          }
          next_x_row++;
        }
      }
    } else {
      while (next_x_row < filter_offset + filter_length) {
        if (source_has_alpha) {
          ConvolveHorizontally<true>(
              &source_data[next_x_row * source_byte_row_stride],
              filter_x, row_buffer.AdvanceRow());
        } else {
          ConvolveHorizontally<false>(
              &source_data[next_x_row * source_byte_row_stride],
              filter_x, row_buffer.AdvanceRow());
        }
        next_x_row++;
      }
    }

    
    unsigned char* cur_output_row = &output[out_y * output_byte_row_stride];

    
    int first_row_in_circular_buffer;
    unsigned char* const* rows_to_convolve =
        row_buffer.GetRowAddresses(&first_row_in_circular_buffer);

    
    
    unsigned char* const* first_row_for_filter =
        &rows_to_convolve[filter_offset - first_row_in_circular_buffer];

    if (source_has_alpha) {
      if (use_sse2) {
        ConvolveVertically_SSE2<true>(filter_values, filter_length,
                                      first_row_for_filter,
                                      filter_x.num_values(), cur_output_row);
      } else {
        ConvolveVertically<true>(filter_values, filter_length,
                                 first_row_for_filter,
                                 filter_x.num_values(), cur_output_row);
      }
    } else {
      if (use_sse2) {
        ConvolveVertically_SSE2<false>(filter_values, filter_length,
                                       first_row_for_filter,
                                       filter_x.num_values(), cur_output_row);
      } else {
        ConvolveVertically<false>(filter_values, filter_length,
                                 first_row_for_filter,
                                 filter_x.num_values(), cur_output_row);
      }
    }
  }
}

}  
