





#include "imgStatusTracker.h"

#include "imgRequest.h"
#include "imgIContainer.h"
#include "imgRequestProxy.h"
#include "Image.h"
#include "ImageLogging.h"
#include "RasterImage.h"
#include "nsIObserverService.h"

#include "mozilla/Util.h"
#include "mozilla/Assertions.h"
#include "mozilla/Services.h"

using namespace mozilla::image;

NS_IMPL_ISUPPORTS3(imgStatusTrackerObserver,
                   imgIDecoderObserver,
                   imgIContainerObserver,
                   nsISupportsWeakReference)




NS_IMETHODIMP imgStatusTrackerObserver::FrameChanged(const nsIntRect *dirtyRect)
{
  LOG_SCOPE(gImgLog, "imgStatusTrackerObserver::FrameChanged");
  NS_ABORT_IF_FALSE(mTracker->GetImage(),
                    "FrameChanged callback before we've created our image");

  mTracker->RecordFrameChanged(dirtyRect);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
  while (iter.HasMore()) {
    mTracker->SendFrameChanged(iter.GetNext(), dirtyRect);
  }

  return NS_OK;
}




NS_IMETHODIMP imgStatusTrackerObserver::OnStartDecode()
{
  LOG_SCOPE(gImgLog, "imgStatusTrackerObserver::OnStartDecode");
  NS_ABORT_IF_FALSE(mTracker->GetImage(),
                    "OnStartDecode callback before we've created our image");

  mTracker->RecordStartDecode();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
  while (iter.HasMore()) {
    mTracker->SendStartDecode(iter.GetNext());
  }

  if (!mTracker->GetRequest()->GetMultipart()) {
    MOZ_ASSERT(!mTracker->mBlockingOnload);
    mTracker->mBlockingOnload = true;

    mTracker->RecordBlockOnload();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
    while (iter.HasMore()) {
      mTracker->SendBlockOnload(iter.GetNext());
    }
  }

  




  mTracker->GetRequest()->ResetCacheEntry();

  return NS_OK;
}

NS_IMETHODIMP imgStatusTrackerObserver::OnStartRequest()
{
  NS_NOTREACHED("imgRequest(imgIDecoderObserver)::OnStartRequest");
  return NS_OK;
}


NS_IMETHODIMP imgStatusTrackerObserver::OnStartContainer()
{
  LOG_SCOPE(gImgLog, "imgStatusTrackerObserver::OnStartContainer");

  NS_ABORT_IF_FALSE(mTracker->GetImage(),
                    "OnStartContainer callback before we've created our image");
  mTracker->RecordStartContainer(mTracker->GetImage());

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
  while (iter.HasMore()) {
    mTracker->SendStartContainer(iter.GetNext());
  }

  return NS_OK;
}


NS_IMETHODIMP imgStatusTrackerObserver::OnStartFrame()
{
  LOG_SCOPE(gImgLog, "imgStatusTrackerObserver::OnStartFrame");
  NS_ABORT_IF_FALSE(mTracker->GetImage(),
                    "OnStartFrame callback before we've created our image");

  mTracker->RecordStartFrame();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
  while (iter.HasMore()) {
    mTracker->SendStartFrame(iter.GetNext());
  }

  return NS_OK;
}


NS_IMETHODIMP imgStatusTrackerObserver::OnDataAvailable(const nsIntRect * rect)
{
  LOG_SCOPE(gImgLog, "imgStatusTrackerObserver::OnDataAvailable");
  NS_ABORT_IF_FALSE(mTracker->GetImage(),
                    "OnDataAvailable callback before we've created our image");

  mTracker->RecordDataAvailable();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
  while (iter.HasMore()) {
    mTracker->SendDataAvailable(iter.GetNext(), rect);
  }

  return NS_OK;
}


NS_IMETHODIMP imgStatusTrackerObserver::OnStopFrame()
{
  LOG_SCOPE(gImgLog, "imgStatusTrackerObserver::OnStopFrame");
  NS_ABORT_IF_FALSE(mTracker->GetImage(),
                    "OnStopFrame callback before we've created our image");

  mTracker->RecordStopFrame();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
  while (iter.HasMore()) {
    mTracker->SendStopFrame(iter.GetNext());
  }

  mTracker->MaybeUnblockOnload();

  return NS_OK;
}

static void
FireFailureNotification(imgRequest* aRequest)
{
  
  

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    nsCOMPtr<nsIURI> uri;
    aRequest->GetURI(getter_AddRefs(uri));
    os->NotifyObservers(uri, "net:failed-to-process-uri-content", nullptr);
  }
}


NS_IMETHODIMP imgStatusTrackerObserver::OnStopDecode(nsresult aStatus)
{
  LOG_SCOPE(gImgLog, "imgStatusTrackerObserver::OnStopDecode");
  NS_ABORT_IF_FALSE(mTracker->GetImage(),
                    "OnStopDecode callback before we've created our image");

  
  
  mTracker->GetRequest()->UpdateCacheEntrySize();

  bool preexistingError = mTracker->GetImageStatus() == imgIRequest::STATUS_ERROR;

  mTracker->RecordStopDecode(aStatus);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
  while (iter.HasMore()) {
    mTracker->SendStopDecode(iter.GetNext(), aStatus);
  }

  
  
  mTracker->MaybeUnblockOnload();

  if (NS_FAILED(aStatus) && !preexistingError) {
    FireFailureNotification(mTracker->GetRequest());
  }

  return NS_OK;
}

NS_IMETHODIMP imgStatusTrackerObserver::OnStopRequest(bool aLastPart)
{
  NS_NOTREACHED("imgRequest(imgIDecoderObserver)::OnStopRequest");
  return NS_OK;
}


NS_IMETHODIMP imgStatusTrackerObserver::OnDiscard()
{
  NS_ABORT_IF_FALSE(mTracker->GetImage(),
                    "OnDiscard callback before we've created our image");

  mTracker->RecordDiscard();

  
  mTracker->GetRequest()->UpdateCacheEntrySize();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
  while (iter.HasMore()) {
    mTracker->SendDiscard(iter.GetNext());
  }

  return NS_OK;
}

NS_IMETHODIMP imgStatusTrackerObserver::OnImageIsAnimated()
{
  NS_ABORT_IF_FALSE(mTracker->GetImage(),
                    "OnImageIsAnimated callback before we've created our image");
  mTracker->RecordImageIsAnimated();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
  while (iter.HasMore()) {
    mTracker->SendImageIsAnimated(iter.GetNext());
  }

  return NS_OK;
}



imgStatusTracker::imgStatusTracker(Image* aImage, imgRequest* aRequest)
  : mImage(aImage),
    mRequest(aRequest),
    mState(0),
    mImageStatus(imgIRequest::STATUS_NONE),
    mHadLastPart(false),
    mBlockingOnload(false),
    mTrackerObserver(new imgStatusTrackerObserver(this))
{}

imgStatusTracker::imgStatusTracker(const imgStatusTracker& aOther)
  : mImage(aOther.mImage),
    mRequest(aOther.mRequest),
    mState(aOther.mState),
    mImageStatus(aOther.mImageStatus),
    mHadLastPart(aOther.mHadLastPart),
    mBlockingOnload(aOther.mBlockingOnload)
    
    
    
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

uint32_t
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

      for (uint32_t i = 0; i < mProxies.Length(); ++i) {
        mProxies[i]->SetNotificationsDeferred(false);
        statusTracker.SyncNotify(mProxies[i]);
      }

      statusTracker.mRequestRunnable = nullptr;
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
  nsAutoCString spec;
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
  nsAutoCString spec;
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
  nsAutoCString spec;
  uri->GetSpec(spec);
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgStatusTracker::SyncNotify", "uri", spec.get());
#endif

  nsCOMPtr<imgIRequest> kungFuDeathGrip(proxy);

  
  if (mState & stateRequestStarted)
    proxy->OnStartRequest();

  
  if (mState & stateHasSize)
    proxy->OnStartContainer();

  
  if (mState & stateDecodeStarted)
    proxy->OnStartDecode();

  
  if (mState & stateBlockingOnload)
    proxy->BlockOnload();

  if (mImage) {
    int16_t imageType = mImage->GetType();
    
    if (imageType == imgIContainer::TYPE_VECTOR ||
        static_cast<RasterImage*>(mImage)->GetNumFrames() > 0) {

      proxy->OnStartFrame();

      
      
      
      nsIntRect r;
      mImage->GetCurrentFrameRect(r);
      proxy->OnDataAvailable(&r);

      if (mState & stateFrameStopped)
        proxy->OnStopFrame();
    }

    
    bool isAnimated = false;

    nsresult rv = mImage->GetAnimated(&isAnimated);
    if (NS_SUCCEEDED(rv) && isAnimated) {
      proxy->OnImageIsAnimated();
    }
  }

  if (mState & stateDecodeStopped) {
    NS_ABORT_IF_FALSE(mImage, "stopped decoding without ever having an image?");
    proxy->OnStopDecode();
  }

  if (mState & stateRequestStopped) {
    proxy->OnStopRequest(mHadLastPart);
  }
}

void
imgStatusTracker::EmulateRequestFinished(imgRequestProxy* aProxy,
                                         nsresult aStatus)
{
  nsCOMPtr<imgIRequest> kungFuDeathGrip(aProxy);

  
  
  if (!(mState & stateRequestStarted)) {
    aProxy->OnStartRequest();
  }

  if (mState & stateBlockingOnload) {
    aProxy->UnblockOnload();
  }

  if (!(mState & stateRequestStopped)) {
    aProxy->OnStopRequest(true);
  }
}

void
imgStatusTracker::AddConsumer(imgRequestProxy* aConsumer)
{
  mConsumers.AppendElementUnlessExists(aConsumer);
}


bool
imgStatusTracker::RemoveConsumer(imgRequestProxy* aConsumer,
                                 nsresult aStatus,
                                 bool aOnlySendStopRequest)
{
  
  bool removed = mConsumers.RemoveElement(aConsumer);

  
  
  if (removed)
    EmulateRequestFinished(aConsumer, aStatus, aOnlySendStopRequest);
  return removed;
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
imgStatusTracker::SendStartContainer(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartContainer();
}

void
imgStatusTracker::RecordStartFrame()
{
  NS_ABORT_IF_FALSE(mImage, "RecordStartFrame called before we have an Image");
  
  
}

void
imgStatusTracker::SendStartFrame(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartFrame();
}

void
imgStatusTracker::RecordDataAvailable()
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordDataAvailable called before we have an Image");
  
  
}

void
imgStatusTracker::SendDataAvailable(imgRequestProxy* aProxy,
                                    const nsIntRect* aRect)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnDataAvailable(aRect);
}


void
imgStatusTracker::RecordStopFrame()
{
  NS_ABORT_IF_FALSE(mImage, "RecordStopFrame called before we have an Image");
  mState |= stateFrameStopped;
  mImageStatus |= imgIRequest::STATUS_FRAME_COMPLETE;
}

void
imgStatusTracker::SendStopFrame(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStopFrame();
}

void
imgStatusTracker::RecordStopDecode(nsresult aStatus)
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordStopDecode called before we have an Image");
  mState |= stateDecodeStopped;

  if (NS_SUCCEEDED(aStatus) && mImageStatus != imgIRequest::STATUS_ERROR)
    mImageStatus |= imgIRequest::STATUS_DECODE_COMPLETE;
  
  else
    mImageStatus = imgIRequest::STATUS_ERROR;
}

void
imgStatusTracker::SendStopDecode(imgRequestProxy* aProxy,
                                 nsresult aStatus)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStopDecode();
}

void
imgStatusTracker::RecordDiscard()
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordDiscard called before we have an Image");
  
  uint32_t stateBitsToClear = stateDecodeStarted | stateDecodeStopped;
  mState &= ~stateBitsToClear;

  
  uint32_t statusBitsToClear = imgIRequest::STATUS_FRAME_COMPLETE
                               | imgIRequest::STATUS_DECODE_COMPLETE;
  mImageStatus &= ~statusBitsToClear;
}

void
imgStatusTracker::SendImageIsAnimated(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnImageIsAnimated();
}

void
imgStatusTracker::RecordImageIsAnimated()
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordImageIsAnimated called before we have an Image");
  
  
  
  
}

void
imgStatusTracker::SendDiscard(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnDiscard();
}


void
imgStatusTracker::RecordFrameChanged(const nsIntRect* aDirtyRect)
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordFrameChanged called before we have an Image");
  
  
}

void
imgStatusTracker::SendFrameChanged(imgRequestProxy* aProxy,
                                   const nsIntRect* aDirtyRect)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->FrameChanged(aDirtyRect);
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
  mState &= ~stateBlockingOnload;

  mState |= stateRequestStarted;
}

void
imgStatusTracker::SendStartRequest(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartRequest();
}

void
imgStatusTracker::OnStartRequest()
{
  RecordStartRequest();
  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    SendStartRequest(iter.GetNext());
  }
}

void
imgStatusTracker::RecordStopRequest(bool aLastPart,
                                    nsresult aStatus)
{
  mHadLastPart = aLastPart;
  mState |= stateRequestStopped;

  
  if (NS_SUCCEEDED(aStatus) && mImageStatus != imgIRequest::STATUS_ERROR)
    mImageStatus |= imgIRequest::STATUS_LOAD_COMPLETE;
  else
    mImageStatus = imgIRequest::STATUS_ERROR;
}

void
imgStatusTracker::SendStopRequest(imgRequestProxy* aProxy,
                                  bool aLastPart,
                                  nsresult aStatus)
{
  if (!aProxy->NotificationsDeferred()) {
    aProxy->OnStopRequest(aLastPart);
  }
}

void
imgStatusTracker::OnStopRequest(bool aLastPart,
                                nsresult aStatus)
{
  bool preexistingError = mImageStatus == imgIRequest::STATUS_ERROR;

  RecordStopRequest(aLastPart, aStatus);
  
  nsTObserverArray<imgRequestProxy*>::ForwardIterator srIter(mConsumers);
  while (srIter.HasMore()) {
    SendStopRequest(srIter.GetNext(), aLastPart, aStatus);
  }

  if (NS_FAILED(aStatus) && !preexistingError) {
    FireFailureNotification(GetRequest());
  }
}

void
imgStatusTracker::OnDataAvailable()
{
  
  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    iter.GetNext()->SetHasImage();
  }
}

void
imgStatusTracker::RecordBlockOnload()
{
  MOZ_ASSERT(!(mState & stateBlockingOnload));
  mState |= stateBlockingOnload;
}

void
imgStatusTracker::SendBlockOnload(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred()) {
    aProxy->BlockOnload();
  }
}

void
imgStatusTracker::RecordUnblockOnload()
{
  MOZ_ASSERT(mState & stateBlockingOnload);
  mState &= ~stateBlockingOnload;
}

void
imgStatusTracker::SendUnblockOnload(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred()) {
    aProxy->UnblockOnload();
  }
}

void
imgStatusTracker::MaybeUnblockOnload()
{
  if (!mBlockingOnload) {
    return;
  }

  mBlockingOnload = false;

  RecordUnblockOnload();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    SendUnblockOnload(iter.GetNext());
  }
}
