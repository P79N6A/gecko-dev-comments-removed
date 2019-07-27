



























#ifndef SKIA_EXT_CONVOLVER_H_
#define SKIA_EXT_CONVOLVER_H_

#include <cmath>
#include <vector>

#include "base/basictypes.h"
#include "base/cpu.h"
#include "mozilla/Assertions.h"
#include "skia/SkTypes.h"


#if defined(__APPLE__)
#undef FloatToFixed
#undef FixedToFloat
#endif

namespace skia {









class ConvolutionFilter1D {
 public:
  typedef short Fixed;

  
  enum { kShiftBits = 14 };

  ConvolutionFilter1D();
  ~ConvolutionFilter1D();

  
  static Fixed FloatToFixed(float f) {
    return static_cast<Fixed>(f * (1 << kShiftBits));
  }
  static unsigned char FixedToChar(Fixed x) {
    return static_cast<unsigned char>(x >> kShiftBits);
  }
  static float FixedToFloat(Fixed x) {
    
    
    
    static_assert(sizeof(Fixed) == 2,
                  "fixed type should fit in float mantissa");
    float raw = static_cast<float>(x);
    return ldexpf(raw, -kShiftBits);
  }

  
  int max_filter() const { return max_filter_; }

  
  
  int num_values() const { return static_cast<int>(filters_.size()); }

  
  
  
  
  
  
  
  
  
  
  
  
  void AddFilter(int filter_offset,
                        const float* filter_values,
                        int filter_length);

  
  void AddFilter(int filter_offset,
                 const Fixed* filter_values,
                 int filter_length);

  
  
  
  
  
  inline const Fixed* FilterForValue(int value_offset,
                                     int* filter_offset,
                                     int* filter_length) const {
    const FilterInstance& filter = filters_[value_offset];
    *filter_offset = filter.offset;
    *filter_length = filter.length;
    if (filter.length == 0) {
      return NULL;
    }
    return &filter_values_[filter.data_location];
  }


  inline void PaddingForSIMD(int padding_count) {
    
    
    
    
    
    for (int i = 0; i < padding_count; ++i)
      filter_values_.push_back(static_cast<Fixed>(0));
  }

 private:
  struct FilterInstance {
    
    int data_location;

    
    int offset;

    
    int length;
  };

  
  std::vector<FilterInstance> filters_;

  
  
  
  std::vector<Fixed> filter_values_;

  
  int max_filter_;
};



















void BGRAConvolve2D(const unsigned char* source_data,
                    int source_byte_row_stride,
                    bool source_has_alpha,
                    const ConvolutionFilter1D& xfilter,
                    const ConvolutionFilter1D& yfilter,
                    int output_byte_row_stride,
                    unsigned char* output);

}  

#endif  
