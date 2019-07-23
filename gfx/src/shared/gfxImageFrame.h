






































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
  nsresult  SetData(const PRUint8 *aData, PRUint32 aLength, 
                    PRInt32 aOffset, PRBool aSetAlpha);

  nsCOMPtr<nsIImage> mImage;

  PRPackedBool mInitialized;
  PRPackedBool mMutable;
  PRPackedBool mHasBackgroundColor;
  PRPackedBool mTopToBottom;
  gfx_format   mFormat;

  PRInt32 mTimeout; 
  nsIntPoint mOffset;

  gfx_color mBackgroundColor;

  PRInt32   mDisposalMethod;
};
