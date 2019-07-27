



























#include "2D.h"
#include "convolver.h"

#include <algorithm>

#include "skia/SkTypes.h"


#if defined(USE_SSE2)
#include "convolverSSE2.h"
#endif

using mozilla::gfx::Factory;

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

#if defined(__GNUC__) && defined(MOZ_GCC_VERSION_AT_LEAST)
#if MOZ_GCC_VERSION_AT_LEAST(4, 5, 0) && !MOZ_GCC_VERSION_AT_LEAST(4, 6, 0)
__attribute__((optimize("-O1")))
#endif
#endif
void ConvolveHorizontally(const unsigned char* src_data,
                          int begin, int end,
                          const ConvolutionFilter1D& filter,
                          unsigned char* out_row) {
  
  for (int out_x = begin; out_x < end; out_x++) {
    
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
                        int begin, int end, unsigned char* out_row) {
  
  
  for (int out_x = begin; out_x < end; out_x++) {
    
    
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
                    unsigned char* output) {
  bool use_sse2 = Factory::HasSSE2();

#if !defined(USE_SSE2)
  
  
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
  int pixel_width = filter_x.num_values();

  
  
  int last_filter_offset, last_filter_length;
  filter_y.FilterForValue(num_output_rows - 1, &last_filter_offset,
                          &last_filter_length);

  for (int out_y = 0; out_y < num_output_rows; out_y++) {
    filter_values = filter_y.FilterForValue(out_y,
                                            &filter_offset, &filter_length);

    
    if (use_sse2) {
#if defined(USE_SSE2)
      
      
      while (next_x_row < filter_offset + filter_length) {
        if (next_x_row + 3 < last_filter_offset + last_filter_length - 3) {
          const unsigned char* src[4];
          unsigned char* out_row[4];
          for (int i = 0; i < 4; ++i) {
            src[i] = &source_data[(next_x_row + i) * source_byte_row_stride];
            out_row[i] = row_buffer.AdvanceRow();
          }
          ConvolveHorizontally4_SSE2(src, 0, pixel_width, filter_x, out_row);
          next_x_row += 4;
        } else {
          unsigned char* buffer = row_buffer.AdvanceRow();

          
          
          int simd_width = pixel_width & ~3;
          if (simd_width) {
            ConvolveHorizontally_SSE2(
                &source_data[next_x_row * source_byte_row_stride],
                0, simd_width, filter_x, buffer);
          }

          if (pixel_width > simd_width) {
            if (source_has_alpha) {
              ConvolveHorizontally<true>(
                  &source_data[next_x_row * source_byte_row_stride],
                  simd_width, pixel_width, filter_x, buffer);
            } else {
              ConvolveHorizontally<false>(
                  &source_data[next_x_row * source_byte_row_stride],
                  simd_width, pixel_width, filter_x, buffer);
            }
          }
          next_x_row++;
        }
      }
#endif
    } else {
      while (next_x_row < filter_offset + filter_length) {
        if (source_has_alpha) {
          ConvolveHorizontally<true>(
              &source_data[next_x_row * source_byte_row_stride],
              0, pixel_width, filter_x, row_buffer.AdvanceRow());
        } else {
          ConvolveHorizontally<false>(
              &source_data[next_x_row * source_byte_row_stride],
              0, pixel_width, filter_x, row_buffer.AdvanceRow());
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

    int processed = 0;
#if defined(USE_SSE2)
    int simd_width = pixel_width & ~3;
    if (use_sse2 && simd_width) {
        ConvolveVertically_SSE2(filter_values, filter_length, first_row_for_filter,
                                0, simd_width, cur_output_row, source_has_alpha);
        processed = simd_width;
    }
#endif
    if (source_has_alpha) {
      ConvolveVertically<true>(filter_values, filter_length,
                               first_row_for_filter,
                               processed, pixel_width, cur_output_row);
    } else {
      ConvolveVertically<false>(filter_values, filter_length,
                               first_row_for_filter,
                               processed, pixel_width, cur_output_row);
    }
  }
}

}  
