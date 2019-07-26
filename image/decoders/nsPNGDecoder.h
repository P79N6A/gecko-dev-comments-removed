





#ifndef nsPNGDecoder_h__
#define nsPNGDecoder_h__

#include "Decoder.h"

#include "gfxASurface.h"

#include "nsCOMPtr.h"

#include "png.h"

#include "qcms.h"

namespace mozilla {
namespace image {
class RasterImage;

class nsPNGDecoder : public Decoder
{
public:
  nsPNGDecoder(RasterImage &aImage);
  virtual ~nsPNGDecoder();

  virtual void InitInternal();
  virtual void WriteInternal(const char* aBuffer, uint32_t aCount);
  virtual Telemetry::ID SpeedHistogram();

  void CreateFrame(png_uint_32 x_offset, png_uint_32 y_offset,
                   int32_t width, int32_t height,
                   gfxASurface::gfxImageFormat format);
  void EndImageFrame();

  
  
  bool IsValidICO() const
  {
    
    
    
    
    if (setjmp(png_jmpbuf(mPNG))) {
      
      return false;
    }

    png_uint_32
        png_width,  
        png_height; 

    int png_bit_depth,
        png_color_type;

    if (png_get_IHDR(mPNG, mInfo, &png_width, &png_height, &png_bit_depth,
                     &png_color_type, NULL, NULL, NULL)) {

      return ((png_color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
               png_color_type == PNG_COLOR_TYPE_RGB) &&
              png_bit_depth == 8);
    } else {
      return false;
    }
  }

public:
  png_structp mPNG;
  png_infop mInfo;
  nsIntRect mFrameRect;
  uint8_t *mCMSLine;
  uint8_t *interlacebuf;
  qcms_profile *mInProfile;
  qcms_transform *mTransform;

  gfxASurface::gfxImageFormat format;

  
  uint8_t *mHeaderBuf;
  uint32_t mHeaderBytesRead;

  uint8_t mChannels;
  bool mFrameHasNoAlpha;
  bool mFrameIsHidden;

  
  uint32_t mCMSMode;
  bool mDisablePremultipliedAlpha;
  
  




  static void PNGAPI info_callback(png_structp png_ptr, png_infop info_ptr);
  static void PNGAPI row_callback(png_structp png_ptr, png_bytep new_row,
                                  png_uint_32 row_num, int pass);
#ifdef PNG_APNG_SUPPORTED
  static void PNGAPI frame_info_callback(png_structp png_ptr,
                                         png_uint_32 frame_num);
#endif
  static void PNGAPI end_callback(png_structp png_ptr, png_infop info_ptr);
  static void PNGAPI error_callback(png_structp png_ptr,
                                    png_const_charp error_msg);
  static void PNGAPI warning_callback(png_structp png_ptr,
                                      png_const_charp warning_msg);

  
  
  static const uint8_t pngSignatureBytes[];
};

} 
} 

#endif 
