







































#include "nsIconDecoder.h"
#include "nsIInputStream.h"
#include "imgIContainer.h"
#include "imgIContainerObserver.h"
#include "nspr.h"
#include "nsIComponentManager.h"
#include "nsRect.h"
#include "nsComponentManagerUtils.h"

#include "nsIInterfaceRequestorUtils.h"
#include "ImageErrors.h"

NS_IMPL_THREADSAFE_ADDREF(nsIconDecoder)
NS_IMPL_THREADSAFE_RELEASE(nsIconDecoder)

NS_INTERFACE_MAP_BEGIN(nsIconDecoder)
   NS_INTERFACE_MAP_ENTRY(imgIDecoder)
NS_INTERFACE_MAP_END_THREADSAFE


nsIconDecoder::nsIconDecoder() :
  mImage(nsnull),
  mObserver(nsnull),
  mFlags(imgIDecoder::DECODER_FLAG_NONE),
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




NS_IMETHODIMP nsIconDecoder::Init(imgIContainer *aImage,
                                  imgIDecoderObserver *aObserver,
                                  PRUint32 aFlags)
{

  
  mImage = aImage;
  mObserver = aObserver;
  mFlags = aFlags;

  
  if (!(mFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) && mObserver)
    mObserver->OnStartDecode(nsnull);

  return NS_OK;
}

NS_IMETHODIMP nsIconDecoder::Close(PRUint32 aFlags)
{
  
  
  if (!(aFlags & CLOSE_FLAG_DONTNOTIFY) &&
      !(mFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) &&
      !mNotifiedDone)
    NotifyDone( PR_FALSE);

  mImage = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsIconDecoder::Flush()
{
  return NS_OK;
}

NS_IMETHODIMP
nsIconDecoder::Write(const char *aBuffer, PRUint32 aCount)
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

        
        mImage->SetSize(mWidth, mHeight);
        if (mObserver)
          mObserver->OnStartContainer(nsnull, mImage);

        
        if (mFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) {
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
        if (mObserver)
         mObserver->OnStartFrame(nsnull, 0);

        
        aBuffer++;
        aCount--;
        mState = iconStateReadPixels;
        break;

      case iconStateReadPixels:

        
        bytesToRead = PR_MAX(aCount, mPixBytesTotal - mPixBytesRead);

        
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

  
  if (mObserver)
    mObserver->OnStopFrame(nsnull, 0);
  if (aSuccess)
    mImage->DecodingComplete();
  if (mObserver) {
    mObserver->OnStopContainer(nsnull, mImage);
    mObserver->OnStopDecode(nsnull, aSuccess ? NS_OK : NS_ERROR_FAILURE,
                            nsnull);
  }

  
  mNotifiedDone = PR_TRUE;
}

