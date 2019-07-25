







































#include "nsIconDecoder.h"
#include "nsIInputStream.h"
#include "RasterImage.h"
#include "imgIContainerObserver.h"
#include "nspr.h"
#include "nsRect.h"

#include "ImageErrors.h"

namespace mozilla {
namespace imagelib {

nsIconDecoder::nsIconDecoder() :
  mWidth(-1),
  mHeight(-1),
  mPixBytesRead(0),
  mPixBytesTotal(0),
  mImageData(nsnull),
  mState(iconStateStart)
{
  
}

nsIconDecoder::~nsIconDecoder()
{ }

void
nsIconDecoder::WriteInternal(const char *aBuffer, PRUint32 aCount)
{
  NS_ABORT_IF_FALSE(!HasError(), "Shouldn't call WriteInternal after error!");

  
  
  PRUint32 bytesToRead = 0;
  nsresult rv;

  
  
  nsIntRect r(0, 0, mWidth, mHeight);

  
  while (aCount > 0) {
    switch (mState) {
      case iconStateStart:

        
        mWidth = (PRUint8)*aBuffer;

        
        aBuffer++;
        aCount--;
        mState = iconStateHaveHeight;
        break;

      case iconStateHaveHeight:

        
        mHeight = (PRUint8)*aBuffer;

        
        PostSize(mWidth, mHeight);
        if (HasError()) {
          
          
          mState = iconStateFinished;
          return;
        }

        
        if (IsSizeDecode()) {
          mState = iconStateFinished;
          break;
        }

        
        rv = mImage->AppendFrame(0, 0, mWidth, mHeight,
                                 gfxASurface::ImageFormatARGB32,
                                 &mImageData, &mPixBytesTotal);
        if (NS_FAILED(rv)) {
          PostDecoderError(rv);
          return;
        }

        
        PostFrameStart();

        
        aBuffer++;
        aCount--;
        mState = iconStateReadPixels;
        break;

      case iconStateReadPixels:

        
        bytesToRead = PR_MIN(aCount, mPixBytesTotal - mPixBytesRead);

        
        memcpy(mImageData + mPixBytesRead, aBuffer, bytesToRead);

        
        PostInvalidation(r);

        
        aBuffer += bytesToRead;
        aCount -= bytesToRead;
        mPixBytesRead += bytesToRead;

        
        if (mPixBytesRead == mPixBytesTotal) {
          PostFrameStop();
          PostDecodeDone();
          mState = iconStateFinished;
        }
        break;

      case iconStateFinished:

        
        aCount = 0;

        break;
    }
  }
}

} 
} 
