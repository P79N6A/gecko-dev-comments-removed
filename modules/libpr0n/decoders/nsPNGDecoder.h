







































#ifndef nsPNGDecoder_h__
#define nsPNGDecoder_h__

#include "Decoder.h"

#include "imgIDecoderObserver.h"
#include "gfxASurface.h"

#include "nsCOMPtr.h"

#include "png.h"

#include "qcms.h"

namespace mozilla {
namespace imagelib {
class RasterImage;

class nsPNGDecoder : public Decoder
{
public:
  nsPNGDecoder(RasterImage *aImage, imgIDecoderObserver* aObserver);
  virtual ~nsPNGDecoder();

  virtual void InitInternal();
  virtual void WriteInternal(const char* aBuffer, PRUint32 aCount);
  virtual Telemetry::ID SpeedHistogram();

  void CreateFrame(png_uint_32 x_offset, png_uint_32 y_offset,
                   PRInt32 width, PRInt32 height,
                   gfxASurface::gfxImageFormat format);
  void SetAnimFrameInfo();

  void EndImageFrame();

  
  bool HasValidInfo() const 
  {
    return mInfo && mInfo->valid;
  }

  
  PRInt32 GetPixelDepth() const
  {
    if (!mInfo) {
      return 0;
    }
    return mInfo->pixel_depth;
  }

public:
  png_structp mPNG;
  png_infop mInfo;
  nsIntRect mFrameRect;
  PRUint8 *mCMSLine;
  PRUint8 *interlacebuf;
  PRUint8 *mImageData;
  qcms_profile *mInProfile;
  qcms_transform *mTransform;

  gfxASurface::gfxImageFormat format;

  
  PRUint8 *mHeaderBuf;
  PRUint32 mHeaderBytesRead;

  PRUint8 mChannels;
  PRPackedBool mFrameHasNoAlpha;
  PRPackedBool mFrameIsHidden;

  
  PRUint32 mCMSMode;
  PRPackedBool mDisablePremultipliedAlpha;
  
  




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

  
  
  static const PRUint8 pngSignatureBytes[];
};

} 
} 

#endif 
