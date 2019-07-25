






































#include "imgStatusTracker.h"

#include "imgRequest.h"
#include "imgIContainer.h"
#include "imgRequestProxy.h"
#include "Image.h"
#include "ImageLogging.h"
#include "RasterImage.h"

using namespace mozilla::imagelib;

static nsresult
GetResultFromImageStatus(PRUint32 aStatus)
{
  if (aStatus & imgIRequest::STATUS_ERROR)
    return NS_IMAGELIB_ERROR_FAILURE;
  if (aStatus & imgIRequest::STATUS_LOAD_COMPLETE)
    return NS_IMAGELIB_SUCCESS_LOAD_FINISHED;
  return NS_OK;
}

imgStatusTracker::imgStatusTracker(Image* aImage)
  : mImage(aImage),
    mState(0),
    mImageStatus(imgIRequest::STATUS_NONE),
    mHadLastPart(false)
{}

imgStatusTracker::imgStatusTracker(const imgStatusTracker& aOther)
  : mImage(aOther.mImage),
    mState(aOther.mState),
    mImageStatus(aOther.mImageStatus),
    mHadLastPart(aOther.mHadLastPart)
    
    
    
{}

void
imgStatusTracker::SetImage(Image* aImage)
{
  NS_ABORT_IF_FALSE(aImage, "Setting null image");
  NS_ABORT_IF_FALSE(!mImage, "Setting image when we already have one");
  mImage = aImage;
}

bool
imgStatusTracker::IsLoading() const
{
  
  
  
  return !(mState & stateRequestStopped);
}

PRUint32
imgStatusTracker::GetImageStatus() const
{
  return mImageStatus;
}


class imgRequestNotifyRunnable : public nsRunnable
{
  public:
    imgRequestNotifyRunnable(imgRequest* request, imgRequestProxy* requestproxy)
      : mRequest(request)
    {
      mProxies.AppendElement(requestproxy);
    }

    NS_IMETHOD Run()
    {
      imgStatusTracker& statusTracker = mRequest->GetStatusTracker();

      for (PRUint32 i = 0; i < mProxies.Length(); ++i) {
        mProxies[i]->SetNotificationsDeferred(false);
        statusTracker.SyncNotify(mProxies[i]);
      }

      statusTracker.mRequestRunnable = nsnull;
      return NS_OK;
    }

    void AddProxy(imgRequestProxy* aRequestProxy)
    {
      mProxies.AppendElement(aRequestProxy);
    }

  private:
    friend class imgStatusTracker;

    nsRefPtr<imgRequest> mRequest;
    nsTArray<nsRefPtr<imgRequestProxy> > mProxies;
};

void
imgStatusTracker::Notify(imgRequest* request, imgRequestProxy* proxy)
{
#ifdef PR_LOGGING
  nsCOMPtr<nsIURI> uri;
  request->GetURI(getter_AddRefs(uri));
  nsCAutoString spec;
  uri->GetSpec(spec);
  LOG_FUNC_WITH_PARAM(gImgLog, "imgStatusTracker::Notify async", "uri", spec.get());
#endif

  proxy->SetNotificationsDeferred(true);

  
  
  
  imgRequestNotifyRunnable* runnable = static_cast<imgRequestNotifyRunnable*>(mRequestRunnable.get());
  if (runnable && runnable->mRequest == request) {
    runnable->AddProxy(proxy);
  } else {
    
    
    
    mRequestRunnable = new imgRequestNotifyRunnable(request, proxy);
    NS_DispatchToCurrentThread(mRequestRunnable);
  }
}



class imgStatusNotifyRunnable : public nsRunnable
{
  public:
    imgStatusNotifyRunnable(imgStatusTracker& status,
                            imgRequestProxy* requestproxy)
      : mStatus(status), mImage(status.mImage), mProxy(requestproxy)
    {}

    NS_IMETHOD Run()
    {
      mProxy->SetNotificationsDeferred(false);

      mStatus.SyncNotify(mProxy);
      return NS_OK;
    }

  private:
    imgStatusTracker mStatus;
    
    
    nsRefPtr<Image> mImage;
    nsRefPtr<imgRequestProxy> mProxy;
};

void
imgStatusTracker::NotifyCurrentState(imgRequestProxy* proxy)
{
#ifdef PR_LOGGING
  nsCOMPtr<nsIURI> uri;
  proxy->GetURI(getter_AddRefs(uri));
  nsCAutoString spec;
  uri->GetSpec(spec);
  LOG_FUNC_WITH_PARAM(gImgLog, "imgStatusTracker::NotifyCurrentState", "uri", spec.get());
#endif

  proxy->SetNotificationsDeferred(true);

  
  nsCOMPtr<nsIRunnable> ev = new imgStatusNotifyRunnable(*this, proxy);
  NS_DispatchToCurrentThread(ev);
}

void
imgStatusTracker::SyncNotify(imgRequestProxy* proxy)
{
  NS_ABORT_IF_FALSE(!proxy->NotificationsDeferred(),
    "Calling imgStatusTracker::Notify() on a proxy that doesn't want notifications!");

#ifdef PR_LOGGING
  nsCOMPtr<nsIURI> uri;
  proxy->GetURI(getter_AddRefs(uri));
  nsCAutoString spec;
  uri->GetSpec(spec);
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgStatusTracker::SyncNotify", "uri", spec.get());
#endif

  nsCOMPtr<imgIRequest> kungFuDeathGrip(proxy);

  
  if (mState & stateRequestStarted)
    proxy->OnStartRequest();

  
  if (mState & stateHasSize)
    proxy->OnStartContainer(mImage);

  
  if (mState & stateDecodeStarted)
    proxy->OnStartDecode();

  if (mImage) {
    PRInt16 imageType = mImage->GetType();
    
    if (imageType == imgIContainer::TYPE_VECTOR ||
        static_cast<RasterImage*>(mImage)->GetNumFrames() > 0) {

      PRUint32 frame = (imageType == imgIContainer::TYPE_VECTOR) ?
        0 : static_cast<RasterImage*>(mImage)->GetCurrentFrameIndex();

      proxy->OnStartFrame(frame);

      
      
      
      nsIntRect r;
      mImage->GetCurrentFrameRect(r);
      proxy->OnDataAvailable(frame, &r);

      if (mState & stateFrameStopped)
        proxy->OnStopFrame(frame);
    }
  }

  
  
  
  if (mState & stateDecodeStopped) {
    NS_ABORT_IF_FALSE(mImage, "stopped decoding without ever having an image?");
    proxy->OnStopContainer(mImage);
  }

  if (mState & stateRequestStopped) {
    proxy->OnStopDecode(GetResultFromImageStatus(mImageStatus), nsnull);
    proxy->OnStopRequest(mHadLastPart);
  }
}

void
imgStatusTracker::EmulateRequestFinished(imgRequestProxy* aProxy, nsresult aStatus,
                                                bool aOnlySendStopRequest)
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
    aProxy->OnStopRequest(true);
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
  NS_ABORT_IF_FALSE(mImage, "RecordLoaded called before we have an Image");
  mState |= stateRequestStarted | stateHasSize | stateRequestStopped;
  mImageStatus |= imgIRequest::STATUS_SIZE_AVAILABLE | imgIRequest::STATUS_LOAD_COMPLETE;
  mHadLastPart = true;
}

void
imgStatusTracker::RecordDecoded()
{
  NS_ABORT_IF_FALSE(mImage, "RecordDecoded called before we have an Image");
  mState |= stateDecodeStarted | stateDecodeStopped | stateFrameStopped;
  mImageStatus |= imgIRequest::STATUS_FRAME_COMPLETE | imgIRequest::STATUS_DECODE_COMPLETE;
}


void
imgStatusTracker::RecordStartDecode()
{
  NS_ABORT_IF_FALSE(mImage, "RecordStartDecode without an Image");
  mState |= stateDecodeStarted;
}

void
imgStatusTracker::SendStartDecode(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartDecode();
}

void
imgStatusTracker::RecordStartContainer(imgIContainer* aContainer)
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordStartContainer called before we have an Image");
  NS_ABORT_IF_FALSE(mImage == aContainer,
                    "RecordStartContainer called with wrong Image");
  mState |= stateHasSize;
  mImageStatus |= imgIRequest::STATUS_SIZE_AVAILABLE;
}

void
imgStatusTracker::SendStartContainer(imgRequestProxy* aProxy, imgIContainer* aContainer)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartContainer(aContainer);
}

void
imgStatusTracker::RecordStartFrame(PRUint32 aFrame)
{
  NS_ABORT_IF_FALSE(mImage, "RecordStartFrame called before we have an Image");
  
  
}

void
imgStatusTracker::SendStartFrame(imgRequestProxy* aProxy, PRUint32 aFrame)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartFrame(aFrame);
}

void
imgStatusTracker::RecordDataAvailable(bool aCurrentFrame, const nsIntRect* aRect)
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordDataAvailable called before we have an Image");
  
  
}

void
imgStatusTracker::SendDataAvailable(imgRequestProxy* aProxy, bool aCurrentFrame,
                                         const nsIntRect* aRect)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnDataAvailable(aCurrentFrame, aRect);
}


void
imgStatusTracker::RecordStopFrame(PRUint32 aFrame)
{
  NS_ABORT_IF_FALSE(mImage, "RecordStopFrame called before we have an Image");
  mState |= stateFrameStopped;
  mImageStatus |= imgIRequest::STATUS_FRAME_COMPLETE;
}

void
imgStatusTracker::SendStopFrame(imgRequestProxy* aProxy, PRUint32 aFrame)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStopFrame(aFrame);
}

void
imgStatusTracker::RecordStopContainer(imgIContainer* aContainer)
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordStopContainer called before we have an Image");
  
}

void
imgStatusTracker::SendStopContainer(imgRequestProxy* aProxy, imgIContainer* aContainer)
{
  
}

void
imgStatusTracker::RecordStopDecode(nsresult aStatus, const PRUnichar* statusArg)
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordStopDecode called before we have an Image");
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
  
  
  
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStopContainer(mImage);
}

void
imgStatusTracker::RecordDiscard()
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordDiscard called before we have an Image");
  
  PRUint32 stateBitsToClear = stateDecodeStarted | stateDecodeStopped;
  mState &= ~stateBitsToClear;

  
  PRUint32 statusBitsToClear = imgIRequest::STATUS_FRAME_COMPLETE
                               | imgIRequest::STATUS_DECODE_COMPLETE;
  mImageStatus &= ~statusBitsToClear;
}

void
imgStatusTracker::SendDiscard(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnDiscard();
}


void
imgStatusTracker::RecordFrameChanged(imgIContainer* aContainer,
                                     const nsIntRect* aDirtyRect)
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordFrameChanged called before we have an Image");
  
  
}

void
imgStatusTracker::SendFrameChanged(imgRequestProxy* aProxy, imgIContainer* aContainer,
                                   const nsIntRect* aDirtyRect)
{
  if (!aProxy->NotificationsDeferred())
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
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartRequest();
}

void
imgStatusTracker::RecordStopRequest(bool aLastPart, nsresult aStatus)
{
  mHadLastPart = aLastPart;
  mState |= stateRequestStopped;

  
  if (NS_SUCCEEDED(aStatus))
    mImageStatus |= imgIRequest::STATUS_LOAD_COMPLETE;
}

void
imgStatusTracker::SendStopRequest(imgRequestProxy* aProxy, bool aLastPart, nsresult aStatus)
{
  
  
  if (!aProxy->NotificationsDeferred()) {
    aProxy->OnStopDecode(GetResultFromImageStatus(mImageStatus), nsnull);
    aProxy->OnStopRequest(aLastPart);
  }
}
