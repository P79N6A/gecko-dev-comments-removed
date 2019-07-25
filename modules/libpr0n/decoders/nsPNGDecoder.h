







































#ifndef nsPNGDecoder_h__
#define nsPNGDecoder_h__

#include "imgIDecoder.h"

#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "gfxASurface.h"

#include "nsCOMPtr.h"

#include "png.h"

#include "qcms.h"

namespace mozilla {
namespace imagelib {
class RasterImage;
} 
} 

class nsPNGDecoder : public imgIDecoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODER

  nsPNGDecoder();
  virtual ~nsPNGDecoder();

  void CreateFrame(png_uint_32 x_offset, png_uint_32 y_offset,
                   PRInt32 width, PRInt32 height,
                   gfxASurface::gfxImageFormat format);
  void SetAnimFrameInfo();

  void EndImageFrame();
  void NotifyDone(PRBool aSuccess);

public:
  nsRefPtr<mozilla::imagelib::RasterImage> mImage;
  nsCOMPtr<imgIDecoderObserver> mObserver;
  PRUint32 mFlags;

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
  PRPackedBool mError;
  PRPackedBool mFrameHasNoAlpha;
  PRPackedBool mFrameIsHidden;
  PRPackedBool mNotifiedDone;

  




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
};

#endif 
