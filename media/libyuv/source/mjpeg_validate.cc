









#include "libyuv/mjpeg_decoder.h"

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif



LIBYUV_BOOL ValidateJpeg(const uint8* sample, size_t sample_size) {
  size_t i;
  if (sample_size < 64) {
    
    return LIBYUV_FALSE;
  }
  if (sample[0] != 0xff || sample[1] != 0xd8) {  
    
    return LIBYUV_FALSE;
  }
  for (i = sample_size - 2; i > 1;) {
    if (sample[i] != 0xd9) {
      if (sample[i] == 0xff && sample[i + 1] == 0xd9) {  
        return LIBYUV_TRUE;  
      }
      --i;
    }
    --i;
  }
  
  return LIBYUV_FALSE;
}

#ifdef __cplusplus
}  
}  
#endif

