







































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

static nsresult
WriteIconData(nsIInputStream *aInStream, void *aClosure, const char *aFromSegment,
              PRUint32 aToOffset, PRUint32 aCount, PRUint32 *aWriteCount)
{
  nsresult rv;

  
  *aWriteCount = aCount;

  
  
  PRUint32 bytesToRead = 0;

  
  nsIconDecoder *decoder = static_cast<nsIconDecoder*>(aClosure);

  
  
  nsIntRect r(0, 0, decoder->mWidth, decoder->mHeight);

  
  while (aCount > 0) {
    switch (decoder->mState) {
      case iconStateStart:

        
        decoder->mWidth = (PRUint8)*aFromSegment;

        
        aFromSegment++;
        aCount--;
        decoder->mState = iconStateHaveHeight;
        break;

      case iconStateHaveHeight:

        
        decoder->mHeight = (PRUint8)*aFromSegment;

        
        decoder->mImage->SetSize(decoder->mWidth,
                                 decoder->mHeight);
        if (decoder->mObserver)
          decoder->mObserver->OnStartContainer(nsnull, decoder->mImage);

        
        if (decoder->mFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) {
          decoder->mState = iconStateFinished;
          break;
        }

        
        rv = decoder->mImage->AppendFrame(0, 0,
                                          decoder->mWidth,
                                          decoder->mHeight,
                                          gfxASurface::ImageFormatARGB32,
                                          &decoder->mImageData, 
                                          &decoder->mPixBytesTotal);
        if (NS_FAILED(rv)) {
          decoder->mState = iconStateError;
          return rv;
        }
        if (decoder->mObserver)
          decoder->mObserver->OnStartFrame(nsnull, 0);

        
        aFromSegment++;
        aCount--;
        decoder->mState = iconStateReadPixels;
        break;

      case iconStateReadPixels:

        
        bytesToRead = PR_MAX(aCount,
                             decoder->mPixBytesTotal - decoder->mPixBytesRead);

        
        memcpy(decoder->mImageData + decoder->mPixBytesRead,
               aFromSegment, bytesToRead);

        
        rv = decoder->mImage->FrameUpdated(0, r);
        if (NS_FAILED(rv)) {
          decoder->mState = iconStateError;
          return rv;
        }
        if (decoder->mObserver)
          decoder->mObserver->OnDataAvailable(nsnull, PR_TRUE, &r);

        
        aFromSegment += bytesToRead;
        aCount -= bytesToRead;
        decoder->mPixBytesRead += bytesToRead;

        
        if (decoder->mPixBytesRead == decoder->mPixBytesTotal) {
          decoder->NotifyDone( PR_TRUE);
          decoder->mState = iconStateFinished;
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


NS_IMETHODIMP nsIconDecoder::WriteFrom(nsIInputStream *inStr, PRUint32 count)
{
  
  nsresult rv = NS_OK;
  PRUint32 ignored;
  if (mState != iconStateError)
    rv = inStr->ReadSegments(WriteIconData, this, count, &ignored);
  if ((mState == iconStateError) || NS_FAILED(rv))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

