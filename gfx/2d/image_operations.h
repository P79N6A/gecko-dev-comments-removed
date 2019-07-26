



#ifndef SKIA_EXT_IMAGE_OPERATIONS_H_
#define SKIA_EXT_IMAGE_OPERATIONS_H_

#include "skia/SkTypes.h"
#include "Types.h"

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

}  

#endif
