



























#ifndef SKIA_EXT_IMAGE_OPERATIONS_H_
#define SKIA_EXT_IMAGE_OPERATIONS_H_

#include "skia/SkTypes.h"
#include "Types.h"
#include "convolver.h"
#include "skia/SkRect.h"

class SkBitmap;
struct SkIRect;

namespace skia {

class ImageOperations {
 public:
  enum ResizeMethod {
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    RESIZE_GOOD,

    
    
    
    
    
    
    
    
    RESIZE_BETTER,

    
    RESIZE_BEST,

    
    
    

    
    
    
    
    
    RESIZE_BOX,

    
    
    
    RESIZE_HAMMING1,

    
    
    
    RESIZE_LANCZOS2,

    
    
    RESIZE_LANCZOS3,

    
    
    RESIZE_SUBPIXEL,

    
    RESIZE_FIRST_QUALITY_METHOD = RESIZE_GOOD,
    RESIZE_LAST_QUALITY_METHOD = RESIZE_BEST,
    RESIZE_FIRST_ALGORITHM_METHOD = RESIZE_BOX,
    RESIZE_LAST_ALGORITHM_METHOD = RESIZE_SUBPIXEL,
  };

  
  
  
  
  
  
  
  
  static SkBitmap Resize(const SkBitmap& source,
                         ResizeMethod method,
                         int dest_width, int dest_height,
                         const SkIRect& dest_subset,
                         void* dest_pixels = nullptr);

  
  
  static SkBitmap Resize(const SkBitmap& source,
                         ResizeMethod method,
                         int dest_width, int dest_height,
                         void* dest_pixels = nullptr);

 private:
  ImageOperations();  

  
  static SkBitmap ResizeBasic(const SkBitmap& source,
                              ResizeMethod method,
                              int dest_width, int dest_height,
                              const SkIRect& dest_subset,
                              void* dest_pixels = nullptr);

  
  static SkBitmap ResizeSubpixel(const SkBitmap& source,
                                 int dest_width, int dest_height,
                                 const SkIRect& dest_subset);
};


inline int CeilInt(float val) {
  return static_cast<int>(ceil(val));
}
inline int FloorInt(float val) {
  return static_cast<int>(floor(val));
}




inline float EvalBox(float x) {
  return (x >= -0.5f && x < 0.5f) ? 1.0f : 0.0f;
}










inline float EvalLanczos(int filter_size, float x) {
  if (x <= -filter_size || x >= filter_size)
    return 0.0f;  
  if (x > -std::numeric_limits<float>::epsilon() &&
      x < std::numeric_limits<float>::epsilon())
    return 1.0f;  
  float xpi = x * static_cast<float>(M_PI);
  return (sin(xpi) / xpi) *  
          sin(xpi / filter_size) / (xpi / filter_size);  
}
















inline float EvalHamming(int filter_size, float x) {
  if (x <= -filter_size || x >= filter_size)
    return 0.0f;  
  if (x > -std::numeric_limits<float>::epsilon() &&
      x < std::numeric_limits<float>::epsilon())
    return 1.0f;  
  const float xpi = x * static_cast<float>(M_PI);

  return ((sin(xpi) / xpi) *  
          (0.54f + 0.46f * cos(xpi / filter_size)));  
}






namespace resize {

  
  
  inline float GetFilterSupport(ImageOperations::ResizeMethod method,
                                float scale) {
    switch (method) {
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

  
  
  
  
  
  
  
  
  
  
  void ComputeFilters(ImageOperations::ResizeMethod method,
                      int src_size, int dst_size,
                      int dest_subset_lo, int dest_subset_size,
                      ConvolutionFilter1D* output);

  
  inline float ComputeFilter(ImageOperations::ResizeMethod method, float pos) {
    switch (method) {
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
}

}  

#endif
