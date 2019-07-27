




#include "nsSVGMaskFrameNEON.h"
#include "nsSVGMaskFrame.h"
#include <arm_neon.h>

void
ComputesRGBLuminanceMask_NEON(const uint8_t *aSourceData,
                              int32_t aSourceStride,
                              uint8_t *aDestData,
                              int32_t aDestStride,
                              const IntSize &aSize,
                              float aOpacity)
{
  int32_t redFactor = 55 * aOpacity; 
  int32_t greenFactor = 183 * aOpacity; 
  int32_t blueFactor = 18 * aOpacity; 
  const uint8_t *sourcePixel = aSourceData;
  int32_t sourceOffset = aSourceStride - 4 * aSize.width;
  uint8_t *destPixel = aDestData;
  int32_t destOffset = aDestStride - aSize.width;

  sourcePixel = aSourceData;
  int32_t remainderWidth = aSize.width % 8;
  int32_t roundedWidth = aSize.width - remainderWidth;
  uint16x8_t temp;
  uint8x8_t gray;
  uint8x8_t redVector = vdup_n_u8(redFactor);
  uint8x8_t greenVector = vdup_n_u8(greenFactor);
  uint8x8_t blueVector = vdup_n_u8(blueFactor);
  uint8x8_t zeroVector = vdup_n_u8(0);
  uint8x8_t oneVector = vdup_n_u8(1);
  for (int32_t y = 0; y < aSize.height; y++) {
    
    for (int32_t x = 0; x < roundedWidth; x += 8) {
      uint8x8x4_t argb  = vld4_u8(sourcePixel);
      temp = vmull_u8(argb.val[GFX_ARGB32_OFFSET_R], redVector); 
      temp = vmlal_u8(temp, argb.val[GFX_ARGB32_OFFSET_G], greenVector); 
      temp = vmlal_u8(temp, argb.val[GFX_ARGB32_OFFSET_B], blueVector); 
      gray = vshrn_n_u16(temp, 8); 

      
      uint8x8_t alphaVector = vcgt_u8(argb.val[GFX_ARGB32_OFFSET_A], zeroVector);
      gray = vmul_u8(gray, vand_u8(alphaVector, oneVector));

      
      vst1_u8(destPixel, gray);
      sourcePixel += 8 * 4;
      destPixel += 8;
    }

    
    for (int32_t x = 0; x < remainderWidth; x++) {
      if (sourcePixel[GFX_ARGB32_OFFSET_A] > 0) {
        *destPixel = (redFactor * sourcePixel[GFX_ARGB32_OFFSET_R]+
                      greenFactor * sourcePixel[GFX_ARGB32_OFFSET_G] +
                      blueFactor * sourcePixel[GFX_ARGB32_OFFSET_B]) >> 8;
      } else {
        *destPixel = 0;
      }
      sourcePixel += 4;
      destPixel++;
    }
    sourcePixel += sourceOffset;
    destPixel += destOffset;
  }
}

