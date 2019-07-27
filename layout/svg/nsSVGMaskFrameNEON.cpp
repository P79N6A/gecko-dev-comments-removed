




#include "nsSVGMaskFrameNEON.h"
#include "nsSVGMaskFrame.h"
#include <arm_neon.h>

void
ComputesRGBLuminanceMask_NEON(uint8_t *aData,
                              int32_t aStride,
                              const IntSize &aSize,
                              float aOpacity)
{
  int32_t redFactor = 55 * aOpacity; 
  int32_t greenFactor = 183 * aOpacity; 
  int32_t blueFactor = 18 * aOpacity; 
  uint8_t *pixel = aData;
  int32_t offset = aStride - 4 * aSize.width;

  
  for (int32_t y = 0; y < aSize.height; y++) {
    for (int32_t x = 0; x < aSize.width; x++) {
      if (!pixel[GFX_ARGB32_OFFSET_A]) {
        memset(pixel, 0, 4);
      }
      pixel += 4;
    }
    pixel += offset;
  }

  pixel = aData;
  int32_t remainderWidth = aSize.width % 8;
  int32_t roundedWidth = aSize.width - remainderWidth;
  uint16x8_t temp;
  uint8x8_t gray;
  uint8x8x4_t result;
  uint8x8_t redVec = vdup_n_u8(redFactor);
  uint8x8_t greenVec = vdup_n_u8(greenFactor);
  uint8x8_t blueVec = vdup_n_u8(blueFactor);
  for (int32_t y = 0; y < aSize.height; y++) {
    
    for (int32_t x = 0; x < roundedWidth; x += 8) {
      uint8x8x4_t argb  = vld4_u8(pixel);
      temp = vmull_u8(argb.val[GFX_ARGB32_OFFSET_R], redVec); 
      temp = vmlal_u8(temp, argb.val[GFX_ARGB32_OFFSET_G], greenVec); 
      temp = vmlal_u8(temp, argb.val[GFX_ARGB32_OFFSET_B], blueVec); 
      gray = vshrn_n_u16(temp, 8); 

      
      result.val[0] = gray;
      result.val[1] = gray;
      result.val[2] = gray;
      result.val[3] = gray;
      vst4_u8(pixel, result);
      pixel += 8 * 4;
    }

    
    for (int32_t x = 0; x < remainderWidth; x++) {
      pixel[0] = (redFactor * pixel[GFX_ARGB32_OFFSET_R] +
                  greenFactor * pixel[GFX_ARGB32_OFFSET_G] +
                  blueFactor * pixel[GFX_ARGB32_OFFSET_B]) >> 8;
      memset(pixel + 1, pixel[0], 3);
      pixel += 4;
    }
    pixel += offset;
  }
}

