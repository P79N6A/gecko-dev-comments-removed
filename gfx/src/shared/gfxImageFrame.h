






































#include "gfxIImageFrame.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

#include "nsIImage.h"

#include "nsPoint.h"
#include "nsSize.h"

#include "nsCOMPtr.h"

#define GFX_IMAGEFRAME_CID \
{ /* aa699204-1dd1-11b2-84a9-a280c268e4fb */         \
     0xaa699204,                                     \
     0x1dd1,                                         \
     0x11b2,                                         \
    {0x84, 0xa9, 0xa2, 0x80, 0xc2, 0x68, 0xe4, 0xfb} \
}

class gfxImageFrame : public gfxIImageFrame,
                      public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_GFXIIMAGEFRAME
  NS_DECL_NSIINTERFACEREQUESTOR

  gfxImageFrame();
  virtual ~gfxImageFrame();

protected:
  nsIntSize mSize;

private:
  PRUint32 PaletteDataLength() const {
    return ((1 << mDepth) * sizeof(gfx_color));
  }

  PRUint32 ImageDataLength() const {
    return (mImage ? mImage->GetLineStride() : mSize.width) * mSize.height;
  }

  nsCOMPtr<nsIImage> mImage;
  PRUint8*     mImageData;

  PRInt32      mTimeout; 
  nsIntPoint   mOffset;
  PRInt32      mDisposalMethod;

  gfx_format   mFormat;
  gfx_depth    mDepth;
  PRInt8       mBlendMethod;
  PRPackedBool mInitialized;
  PRPackedBool mMutable;
};
