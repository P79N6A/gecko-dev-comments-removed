






































#include "imgStatusTracker.h"

#include "imgRequest.h"
#include "imgIContainer.h"
#include "imgRequestProxy.h"

static nsresult
GetResultFromImageStatus(PRUint32 aStatus)
{
  if (aStatus & imgIRequest::STATUS_ERROR)
    return NS_IMAGELIB_ERROR_FAILURE;
  if (aStatus & imgIRequest::STATUS_LOAD_COMPLETE)
    return NS_IMAGELIB_SUCCESS_LOAD_FINISHED;
  return NS_OK;
}

imgStatusTracker::imgStatusTracker(imgIContainer* aImage)
  : mImage(aImage),
    mState(0),
    mImageStatus(imgIRequest::STATUS_NONE),
    mHadLastPart(PR_FALSE)
{}

PRBool
imgStatusTracker::IsLoading() const
{
  
  
  
  return !(mState & stateRequestStopped);
}

PRUint32
imgStatusTracker::GetImageStatus() const
{
  return mImageStatus;
}

void
imgStatusTracker::Notify(imgRequestProxy* proxy)
{
  nsCOMPtr<imgIRequest> kungFuDeathGrip(proxy);

  
  if (mState & stateRequestStarted)
    proxy->OnStartRequest();

  
  if (mState & stateHasSize)
    proxy->OnStartContainer(mImage);

  
  if (mState & stateDecodeStarted)
    proxy->OnStartDecode();

  
  PRUint32 nframes = 0;
  mImage->GetNumFrames(&nframes);

  if (nframes > 0) {
    PRUint32 frame;
    mImage->GetCurrentFrameIndex(&frame);
    proxy->OnStartFrame(frame);

    
    
    
    nsIntRect r;
    mImage->GetCurrentFrameRect(r);
    proxy->OnDataAvailable(frame, &r);

    if (mState & stateFrameStopped)
      proxy->OnStopFrame(frame);
  }

  
  
  
  if (mState & stateDecodeStopped)
    proxy->OnStopContainer(mImage);

  if (mState & stateRequestStopped) {
    proxy->OnStopDecode(GetResultFromImageStatus(mImageStatus), nsnull);
    proxy->OnStopRequest(mHadLastPart);
  }
}

void
imgStatusTracker::EmulateRequestFinished(imgRequestProxy* aProxy, nsresult aStatus,
                                                PRBool aOnlySendStopRequest)
{
  nsCOMPtr<imgIRequest> kungFuDeathGrip(aProxy);

  if (!aOnlySendStopRequest) {
    
    if (!(mState & stateDecodeStopped)) {
      aProxy->OnStopContainer(mImage);
    }

    if (!(mState & stateRequestStopped)) {
      aProxy->OnStopDecode(aStatus, nsnull);
    }
  }

  if (!(mState & stateRequestStopped)) {
    aProxy->OnStopRequest(PR_TRUE);
  }
}

void
imgStatusTracker::RecordCancel()
{
  if (!(mImageStatus & imgIRequest::STATUS_LOAD_PARTIAL))
    mImageStatus |= imgIRequest::STATUS_ERROR;
}

void
imgStatusTracker::RecordLoaded()
{
  mState |= stateRequestStarted | stateHasSize | stateRequestStopped;
  mImageStatus |= imgIRequest::STATUS_SIZE_AVAILABLE | imgIRequest::STATUS_LOAD_COMPLETE;
  mHadLastPart = PR_TRUE;
}

void
imgStatusTracker::RecordDecoded()
{
  mState |= stateDecodeStarted | stateDecodeStopped | stateFrameStopped;
  mImageStatus |= imgIRequest::STATUS_FRAME_COMPLETE | imgIRequest::STATUS_DECODE_COMPLETE;
}


void
imgStatusTracker::RecordStartDecode()
{
  mState |= stateDecodeStarted;
}

void
imgStatusTracker::SendStartDecode(imgRequestProxy* aProxy)
{
  aProxy->OnStartDecode();
}

void
imgStatusTracker::RecordStartContainer(imgIContainer* aContainer)
{
  mState |= stateHasSize;
  mImageStatus |= imgIRequest::STATUS_SIZE_AVAILABLE;
}

void
imgStatusTracker::SendStartContainer(imgRequestProxy* aProxy, imgIContainer* aContainer)
{
  
  
  PRBool alreadySent = (mState & stateHasSize) != 0;
  if (!alreadySent)
    aProxy->OnStartContainer(aContainer);
}

void
imgStatusTracker::RecordStartFrame(PRUint32 aFrame)
{
  
  
}

void
imgStatusTracker::SendStartFrame(imgRequestProxy* aProxy, PRUint32 aFrame)
{
  aProxy->OnStartFrame(aFrame);
}

void
imgStatusTracker::RecordDataAvailable(PRBool aCurrentFrame, const nsIntRect* aRect)
{
  
  
}

void
imgStatusTracker::SendDataAvailable(imgRequestProxy* aProxy, PRBool aCurrentFrame,
                                         const nsIntRect* aRect)
{
  aProxy->OnDataAvailable(aCurrentFrame, aRect);
}


void
imgStatusTracker::RecordStopFrame(PRUint32 aFrame)
{
  mState |= stateFrameStopped;
  mImageStatus |= imgIRequest::STATUS_FRAME_COMPLETE;
}

void
imgStatusTracker::SendStopFrame(imgRequestProxy* aProxy, PRUint32 aFrame)
{
  aProxy->OnStopFrame(aFrame);
}

void
imgStatusTracker::RecordStopContainer(imgIContainer* aContainer)
{
  
}

void
imgStatusTracker::SendStopContainer(imgRequestProxy* aProxy, imgIContainer* aContainer)
{
  
}

void
imgStatusTracker::RecordStopDecode(nsresult aStatus, const PRUnichar* statusArg)
{
  mState |= stateDecodeStopped;

  if (NS_SUCCEEDED(aStatus))
    mImageStatus |= imgIRequest::STATUS_DECODE_COMPLETE;
  
  else
    mImageStatus = imgIRequest::STATUS_ERROR;
}

void
imgStatusTracker::SendStopDecode(imgRequestProxy* aProxy, nsresult aStatus,
                                 const PRUnichar* statusArg)
{
  
  
  
  aProxy->OnStopContainer(mImage);
}

void
imgStatusTracker::RecordDiscard()
{
  
  PRUint32 stateBitsToClear = stateDecodeStarted | stateDecodeStopped;
  mState &= ~stateBitsToClear;

  
  PRUint32 statusBitsToClear = imgIRequest::STATUS_FRAME_COMPLETE
                               | imgIRequest::STATUS_DECODE_COMPLETE;
  mImageStatus &= ~statusBitsToClear;
}

void
imgStatusTracker::SendDiscard(imgRequestProxy* aProxy)
{
  aProxy->OnDiscard();
}


void
imgStatusTracker::RecordFrameChanged(imgIContainer* aContainer, nsIntRect* aDirtyRect)
{
  
  
}

void
imgStatusTracker::SendFrameChanged(imgRequestProxy* aProxy, imgIContainer* aContainer,
                                   nsIntRect* aDirtyRect)
{
  aProxy->FrameChanged(aContainer, aDirtyRect);
}


void
imgStatusTracker::RecordStartRequest()
{
  
  
  mImageStatus &= ~imgIRequest::STATUS_LOAD_PARTIAL;
  mImageStatus &= ~imgIRequest::STATUS_LOAD_COMPLETE;
  mImageStatus &= ~imgIRequest::STATUS_FRAME_COMPLETE;
  mState &= ~stateRequestStarted;
  mState &= ~stateDecodeStarted;
  mState &= ~stateDecodeStopped;
  mState &= ~stateRequestStopped;

  mState |= stateRequestStarted;
}

void
imgStatusTracker::SendStartRequest(imgRequestProxy* aProxy)
{
  aProxy->OnStartRequest();
}

void
imgStatusTracker::RecordStopRequest(PRBool aLastPart, nsresult aStatus)
{
  mHadLastPart = aLastPart;
  mState |= stateRequestStopped;

  
  if (NS_SUCCEEDED(aStatus))
    mImageStatus |= imgIRequest::STATUS_LOAD_COMPLETE;
}

void
imgStatusTracker::SendStopRequest(imgRequestProxy* aProxy, PRBool aLastPart, nsresult aStatus)
{
  
  
  aProxy->OnStopDecode(GetResultFromImageStatus(mImageStatus), nsnull);
  aProxy->OnStopRequest(aLastPart);
}
