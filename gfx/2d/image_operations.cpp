



























#include "base/basictypes.h"

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <limits>

#include "image_operations.h"

#include "base/stack_container.h"
#include "convolver.h"
#include "skia/SkColorPriv.h"
#include "skia/SkBitmap.h"
#include "skia/SkRect.h"
#include "skia/SkFontHost.h"

namespace skia {

namespace {


inline int CeilInt(float val) {
  return static_cast<int>(ceil(val));
}
inline int FloorInt(float val) {
  return static_cast<int>(floor(val));
}




float EvalBox(float x) {
  return (x >= -0.5f && x < 0.5f) ? 1.0f : 0.0f;
}










float EvalLanczos(int filter_size, float x) {
  if (x <= -filter_size || x >= filter_size)
    return 0.0f;  
  if (x > -std::numeric_limits<float>::epsilon() &&
      x < std::numeric_limits<float>::epsilon())
    return 1.0f;  
  float xpi = x * static_cast<float>(M_PI);
  return (sin(xpi) / xpi) *  
          sin(xpi / filter_size) / (xpi / filter_size);  
}
















float EvalHamming(int filter_size, float x) {
  if (x <= -filter_size || x >= filter_size)
    return 0.0f;  
  if (x > -std::numeric_limits<float>::epsilon() &&
      x < std::numeric_limits<float>::epsilon())
    return 1.0f;  
  const float xpi = x * static_cast<float>(M_PI);

  return ((sin(xpi) / xpi) *  
          (0.54f + 0.46f * cos(xpi / filter_size)));  
}





class ResizeFilter {
 public:
  ResizeFilter(ImageOperations::ResizeMethod method,
               int src_full_width, int src_full_height,
               int dest_width, int dest_height,
               const SkIRect& dest_subset);

  
  const ConvolutionFilter1D& x_filter() { return x_filter_; }
  const ConvolutionFilter1D& y_filter() { return y_filter_; }

 private:
  
  
  float GetFilterSupport(float scale) {
    switch (method_) {
      case ImageOperations::RESIZE_BOX:
        
        return 0.5f;  
      case ImageOperations::RESIZE_HAMMING1:
        
        
        return 1.0f;
      case ImageOperations::RESIZE_LANCZOS2:
        
        
        return 2.0f;
      case ImageOperations::RESIZE_LANCZOS3:
        
        
        return 3.0f;
      default:
        return 1.0f;
    }
  }

  
  
  
  
  
  
  
  
  
  
  void ComputeFilters(int src_size,
                      int dest_subset_lo, int dest_subset_size,
                      float scale, ConvolutionFilter1D* output);

  
  inline float ComputeFilter(float pos) {
    switch (method_) {
      case ImageOperations::RESIZE_BOX:
        return EvalBox(pos);
      case ImageOperations::RESIZE_HAMMING1:
        return EvalHamming(1, pos);
      case ImageOperations::RESIZE_LANCZOS2:
        return EvalLanczos(2, pos);
      case ImageOperations::RESIZE_LANCZOS3:
        return EvalLanczos(3, pos);
      default:
        return 0;
    }
  }

  ImageOperations::ResizeMethod method_;

  
  SkIRect out_bounds_;

  ConvolutionFilter1D x_filter_;
  ConvolutionFilter1D y_filter_;

  DISALLOW_COPY_AND_ASSIGN(ResizeFilter);
};

ResizeFilter::ResizeFilter(ImageOperations::ResizeMethod method,
                           int src_full_width, int src_full_height,
                           int dest_width, int dest_height,
                           const SkIRect& dest_subset)
    : method_(method),
      out_bounds_(dest_subset) {
  
  SkASSERT((ImageOperations::RESIZE_FIRST_ALGORITHM_METHOD <= method) &&
           (method <= ImageOperations::RESIZE_LAST_ALGORITHM_METHOD));

  float scale_x = static_cast<float>(dest_width) /
                  static_cast<float>(src_full_width);
  float scale_y = static_cast<float>(dest_height) /
                  static_cast<float>(src_full_height);

  ComputeFilters(src_full_width, dest_subset.fLeft, dest_subset.width(),
                 scale_x, &x_filter_);
  ComputeFilters(src_full_height, dest_subset.fTop, dest_subset.height(),
                 scale_y, &y_filter_);
}












void ResizeFilter::ComputeFilters(int src_size,
                                  int dest_subset_lo, int dest_subset_size,
                                  float scale, ConvolutionFilter1D* output) {
  int dest_subset_hi = dest_subset_lo + dest_subset_size;  

  
  
  
  
  
  float clamped_scale = std::min(1.0f, scale);

  float src_support = GetFilterSupport(clamped_scale) / clamped_scale;

  
  float inv_scale = 1.0f / scale;

  StackVector<float, 64> filter_values;
  StackVector<int16_t, 64> fixed_filter_values;

  
  
  
  for (int dest_subset_i = dest_subset_lo; dest_subset_i < dest_subset_hi;
       dest_subset_i++) {
    
    
    filter_values->clear();
    fixed_filter_values->clear();

    
    
    
    
    
    
    float src_pixel = (static_cast<float>(dest_subset_i) + 0.5f) * inv_scale;

    
    int src_begin = std::max(0, FloorInt(src_pixel - src_support));
    int src_end = std::min(src_size - 1, CeilInt(src_pixel + src_support));

    
    
    float filter_sum = 0.0f;  
    for (int cur_filter_pixel = src_begin; cur_filter_pixel <= src_end;
         cur_filter_pixel++) {
      
      
      
      
      
      
      float src_filter_dist =
           ((static_cast<float>(cur_filter_pixel) + 0.5f) - src_pixel);

      
      float dest_filter_dist = src_filter_dist * clamped_scale;

      
      float filter_value = ComputeFilter(dest_filter_dist);
      filter_values->push_back(filter_value);

      filter_sum += filter_value;
    }

    
    
    int16_t fixed_sum = 0;
    for (size_t i = 0; i < filter_values->size(); i++) {
      int16_t cur_fixed = output->FloatToFixed(filter_values[i] / filter_sum);
      fixed_sum += cur_fixed;
      fixed_filter_values->push_back(cur_fixed);
    }

    
    
    
    
    
    int16_t leftovers = output->FloatToFixed(1.0f) - fixed_sum;
    fixed_filter_values[fixed_filter_values->size() / 2] += leftovers;

    
    output->AddFilter(src_begin, &fixed_filter_values[0],
                      static_cast<int>(fixed_filter_values->size()));
  }

  output->PaddingForSIMD(8);
}

ImageOperations::ResizeMethod ResizeMethodToAlgorithmMethod(
    ImageOperations::ResizeMethod method) {
  
  if (method >= ImageOperations::RESIZE_FIRST_ALGORITHM_METHOD &&
      method <= ImageOperations::RESIZE_LAST_ALGORITHM_METHOD) {
    return method;
  }
  
  
  
  switch (method) {
    
    
    
    
    case ImageOperations::RESIZE_GOOD:
      
      
      
      
      
      
    case ImageOperations::RESIZE_BETTER:
      return ImageOperations::RESIZE_HAMMING1;
    default:
      return ImageOperations::RESIZE_LANCZOS3;
  }
}

}  




SkBitmap ImageOperations::Resize(const SkBitmap& source,
                                 ResizeMethod method,
                                 int dest_width, int dest_height,
                                 const SkIRect& dest_subset,
                                 void* dest_pixels ) {
  if (method == ImageOperations::RESIZE_SUBPIXEL)
    return ResizeSubpixel(source, dest_width, dest_height, dest_subset);
  else
    return ResizeBasic(source, method, dest_width, dest_height, dest_subset,
                       dest_pixels);
}


SkBitmap ImageOperations::ResizeSubpixel(const SkBitmap& source,
                                         int dest_width, int dest_height,
                                         const SkIRect& dest_subset) {
  
  
#if defined(XP_UNIX)
  
  const SkFontHost::LCDOrder order = SkFontHost::GetSubpixelOrder();
  const SkFontHost::LCDOrientation orientation =
      SkFontHost::GetSubpixelOrientation();

  
  int w = 1;
  int h = 1;
  switch (orientation) {
    case SkFontHost::kHorizontal_LCDOrientation:
      w = dest_width < source.width() ? 3 : 1;
      break;
    case SkFontHost::kVertical_LCDOrientation:
      h = dest_height < source.height() ? 3 : 1;
      break;
  }

  
  const int width = dest_width * w;
  const int height = dest_height * h;
  SkIRect subset = { dest_subset.fLeft, dest_subset.fTop,
                     dest_subset.fLeft + dest_subset.width() * w,
                     dest_subset.fTop + dest_subset.height() * h };
  SkBitmap img = ResizeBasic(source, ImageOperations::RESIZE_LANCZOS3, width,
                             height, subset);
  const int row_words = img.rowBytes() / 4;
  if (w == 1 && h == 1)
    return img;

  
  SkBitmap result;
  SkImageInfo info = SkImageInfo::Make(dest_subset.width(),
                                       dest_subset.height(),
                                       kBGRA_8888_SkColorType,
                                       kPremul_SkAlphaType);


  result.allocPixels(info);
  if (!result.readyToDraw())
    return img;

  SkAutoLockPixels locker(img);
  if (!img.readyToDraw())
    return img;

  uint32_t* src_row = img.getAddr32(0, 0);
  uint32_t* dst_row = result.getAddr32(0, 0);
  for (int y = 0; y < dest_subset.height(); y++) {
    uint32_t* src = src_row;
    uint32_t* dst = dst_row;
    for (int x = 0; x < dest_subset.width(); x++, src += w, dst++) {
      uint8_t r = 0, g = 0, b = 0, a = 0;
      switch (order) {
        case SkFontHost::kRGB_LCDOrder:
          switch (orientation) {
            case SkFontHost::kHorizontal_LCDOrientation:
              r = SkGetPackedR32(src[0]);
              g = SkGetPackedG32(src[1]);
              b = SkGetPackedB32(src[2]);
              a = SkGetPackedA32(src[1]);
              break;
            case SkFontHost::kVertical_LCDOrientation:
              r = SkGetPackedR32(src[0 * row_words]);
              g = SkGetPackedG32(src[1 * row_words]);
              b = SkGetPackedB32(src[2 * row_words]);
              a = SkGetPackedA32(src[1 * row_words]);
              break;
          }
          break;
        case SkFontHost::kBGR_LCDOrder:
          switch (orientation) {
            case SkFontHost::kHorizontal_LCDOrientation:
              b = SkGetPackedB32(src[0]);
              g = SkGetPackedG32(src[1]);
              r = SkGetPackedR32(src[2]);
              a = SkGetPackedA32(src[1]);
              break;
            case SkFontHost::kVertical_LCDOrientation:
              b = SkGetPackedB32(src[0 * row_words]);
              g = SkGetPackedG32(src[1 * row_words]);
              r = SkGetPackedR32(src[2 * row_words]);
              a = SkGetPackedA32(src[1 * row_words]);
              break;
          }
          break;
        case SkFontHost::kNONE_LCDOrder:
          break;
      }
      
      a = a > r ? a : r;
      a = a > g ? a : g;
      a = a > b ? a : b;
      *dst = SkPackARGB32(a, r, g, b);
    }
    src_row += h * row_words;
    dst_row += result.rowBytes() / 4;
  }
  result.setAlphaType(img.alphaType());
  return result;
#else
  return SkBitmap();
#endif  
}


SkBitmap ImageOperations::ResizeBasic(const SkBitmap& source,
                                      ResizeMethod method,
                                      int dest_width, int dest_height,
                                      const SkIRect& dest_subset,
                                      void* dest_pixels ) {
  
  SkASSERT(((RESIZE_FIRST_QUALITY_METHOD <= method) &&
            (method <= RESIZE_LAST_QUALITY_METHOD)) ||
           ((RESIZE_FIRST_ALGORITHM_METHOD <= method) &&
            (method <= RESIZE_LAST_ALGORITHM_METHOD)));

  
  
  if (source.width() < 1 || source.height() < 1 ||
      dest_width < 1 || dest_height < 1)
    return SkBitmap();

  method = ResizeMethodToAlgorithmMethod(method);
  
  SkASSERT((ImageOperations::RESIZE_FIRST_ALGORITHM_METHOD <= method) &&
           (method <= ImageOperations::RESIZE_LAST_ALGORITHM_METHOD));

  SkAutoLockPixels locker(source);
  if (!source.readyToDraw())
      return SkBitmap();

  ResizeFilter filter(method, source.width(), source.height(),
                      dest_width, dest_height, dest_subset);

  
  
  
  const uint8_t* source_subset =
      reinterpret_cast<const uint8_t*>(source.getPixels());

  
  SkBitmap result;
  SkImageInfo info = SkImageInfo::Make(dest_subset.width(),
                                       dest_subset.height(),
                                       kBGRA_8888_SkColorType,
                                       kPremul_SkAlphaType);

  if (dest_pixels) {
    result.installPixels(info, dest_pixels, info.minRowBytes());
  } else {
    result.allocPixels(info);
  }

  if (!result.readyToDraw())
    return SkBitmap();

  BGRAConvolve2D(source_subset, static_cast<int>(source.rowBytes()),
                 !source.isOpaque(), filter.x_filter(), filter.y_filter(),
                 static_cast<int>(result.rowBytes()),
                 static_cast<unsigned char*>(result.getPixels()));

  
  result.setAlphaType(source.alphaType());

  return result;
}


SkBitmap ImageOperations::Resize(const SkBitmap& source,
                                 ResizeMethod method,
                                 int dest_width, int dest_height,
                                 void* dest_pixels ) {
  SkIRect dest_subset = { 0, 0, dest_width, dest_height };
  return Resize(source, method, dest_width, dest_height, dest_subset,
                dest_pixels);
}

}  
