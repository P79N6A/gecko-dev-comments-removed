







































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
  mState(iconStateStart),
  mNotifiedDone(PR_FALSE)
{
  
}

nsIconDecoder::~nsIconDecoder()
{ }


nsresult
nsIconDecoder::InitInternal()
{
  
  if (!IsSizeDecode() && mObserver)
    mObserver->OnStartDecode(nsnull);

  return NS_OK;
}

nsresult
nsIconDecoder::ShutdownInternal(PRUint32 aFlags)
{
  
  
  if (!(aFlags & CLOSE_FLAG_DONTNOTIFY) &&
      !IsSizeDecode() &&
      !mNotifiedDone)
    NotifyDone( PR_FALSE);

  return NS_OK;
}

nsresult
nsIconDecoder::WriteInternal(const char *aBuffer, PRUint32 aCount)
{
  nsresult rv;

  
  
  PRUint32 bytesToRead = 0;

  
  
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

        
        if (IsSizeDecode()) {
          mState = iconStateFinished;
          break;
        }

        
        rv = mImage->AppendFrame(0, 0, mWidth, mHeight,
                                 gfxASurface::ImageFormatARGB32,
                                 &mImageData, &mPixBytesTotal);
        if (NS_FAILED(rv)) {
          mState = iconStateError;
          return rv;
        }

        
        PostFrameStart();

        
        aBuffer++;
        aCount--;
        mState = iconStateReadPixels;
        break;

      case iconStateReadPixels:

        
        bytesToRead = PR_MIN(aCount, mPixBytesTotal - mPixBytesRead);

        
        memcpy(mImageData + mPixBytesRead, aBuffer, bytesToRead);

        
        rv = mImage->FrameUpdated(0, r);
        if (NS_FAILED(rv)) {
          mState = iconStateError;
          return rv;
        }
        if (mObserver)
          mObserver->OnDataAvailable(nsnull, PR_TRUE, &r);

        
        aBuffer += bytesToRead;
        aCount -= bytesToRead;
        mPixBytesRead += bytesToRead;

        
        if (mPixBytesRead == mPixBytesTotal) {
          NotifyDone( PR_TRUE);
          mState = iconStateFinished;
        }
        break;

      case iconStateFinished:

        
        aCount = 0;

        break;

      case iconStateError:
        return NS_IMAGELIB_ERROR_FAILURE;
        break;
    }
  }

  return NS_OK;
}

void
nsIconDecoder::NotifyDone(PRBool aSuccess)
{
  
  NS_ABORT_IF_FALSE(!mNotifiedDone, "Calling NotifyDone twice");

  
  PostFrameStop();
  if (aSuccess)
    mImage->DecodingComplete();
  if (mObserver) {
    mObserver->OnStopContainer(nsnull, mImage);
    mObserver->OnStopDecode(nsnull, aSuccess ? NS_OK : NS_ERROR_FAILURE,
                            nsnull);
  }

  
  mNotifiedDone = PR_TRUE;
}

} 
} 
