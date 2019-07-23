



#ifndef BASE_GFX_PNG_ENCODER_H_
#define BASE_GFX_PNG_ENCODER_H_

#include <vector>

#include "base/basictypes.h"

class SkBitmap;







class PNGEncoder {
 public:
  enum ColorFormat {
    
    
    FORMAT_RGB,

    
    FORMAT_RGBA,

    
    
    FORMAT_BGRA
  };

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  static bool Encode(const unsigned char* input, ColorFormat format,
                     int w, int h, int row_byte_width,
                     bool discard_transparency,
                     std::vector<unsigned char>* output);

  
  
  
  
  static bool EncodeBGRASkBitmap(const SkBitmap& input,
                                 bool discard_transparency,
                                 std::vector<unsigned char>* output);

 private:
  DISALLOW_COPY_AND_ASSIGN(PNGEncoder);
};

#endif
