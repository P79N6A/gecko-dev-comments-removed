



#ifndef BASE_GFX_PNG_DECODER_H_
#define BASE_GFX_PNG_DECODER_H_

#include <vector>

#include "base/basictypes.h"

class SkBitmap;







class PNGDecoder {
 public:
  enum ColorFormat {
    
    
    FORMAT_RGB,

    
    FORMAT_RGBA,

    
    
    FORMAT_BGRA
  };

  
  
  
  
  
  
  
  
  static bool Decode(const unsigned char* input, size_t input_size,
                     ColorFormat format, std::vector<unsigned char>* output,
                     int* w, int* h);

  
  
  
  
  
  static bool Decode(const std::vector<unsigned char>* data, SkBitmap* icon);

  
  
  static SkBitmap* CreateSkBitmapFromBGRAFormat(
      std::vector<unsigned char>& bgra, int width, int height);

 private:
  DISALLOW_COPY_AND_ASSIGN(PNGDecoder);
};

#endif
