







































#ifndef nsPNGDecoder_h__
#define nsPNGDecoder_h__

#include "imgIDecoder.h"

#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "gfxASurface.h"

#include "nsCOMPtr.h"

#include "png.h"

#include "qcms.h"

#define NS_PNGDECODER_CID \
{ /* 36fa00c2-1dd2-11b2-be07-d16eeb4c50ed */         \
     0x36fa00c2,                                     \
     0x1dd2,                                         \
     0x11b2,                                         \
    {0xbe, 0x07, 0xd1, 0x6e, 0xeb, 0x4c, 0x50, 0xed} \
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
  nsCOMPtr<imgIContainer> mImage;
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
};

#endif 
