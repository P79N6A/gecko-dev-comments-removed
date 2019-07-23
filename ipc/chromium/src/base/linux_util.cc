



#include "linux_util.h"

#include <stdlib.h>

namespace base {

uint8_t* BGRAToRGBA(const uint8_t* pixels, int width, int height, int stride) {
  if (stride == 0)
    stride = width * 4;

  uint8_t* new_pixels = static_cast<uint8_t*>(malloc(height * stride));

  
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      int idx = i * stride + j * 4;
      new_pixels[idx] = pixels[idx + 2];
      new_pixels[idx + 1] = pixels[idx + 1];
      new_pixels[idx + 2] = pixels[idx];
      new_pixels[idx + 3] = pixels[idx + 3];
    }
  }

  return new_pixels;
}

}  
