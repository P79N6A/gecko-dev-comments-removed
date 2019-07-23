



#ifndef BASE_GFX_JPEG_CODEC_H_
#define BASE_GFX_JPEG_CODEC_H_

#include <vector>

class SkBitmap;





class JPEGCodec {
 public:
  enum ColorFormat {
    
    
    FORMAT_RGB,

    
    FORMAT_RGBA,

    
    
    FORMAT_BGRA
  };

  
  
  
  
  
  
  
  
  
  
  static bool Encode(const unsigned char* input, ColorFormat format,
                     int w, int h, int row_byte_width,
                     int quality, std::vector<unsigned char>* output);

  
  
  
  
  static bool Decode(const unsigned char* input, size_t input_size,
                     ColorFormat format, std::vector<unsigned char>* output,
                     int* w, int* h);

  
  
  
  static SkBitmap* Decode(const unsigned char* input, size_t input_size);
};

#endif
