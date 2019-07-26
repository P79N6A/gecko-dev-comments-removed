





#include "imgStatusTracker.h"

#include "imgIContainer.h"
#include "imgRequestProxy.h"
#include "imgDecoderObserver.h"
#include "Image.h"
#include "ImageLogging.h"
#include "RasterImage.h"
#include "nsIObserverService.h"
#include "RasterImage.h"

#include "mozilla/Util.h"
#include "mozilla/Assertions.h"
#include "mozilla/Services.h"

using namespace mozilla::image;

class imgStatusTrackerNotifyingObserver : public imgDecoderObserver
{
public:
  imgStatusTrackerNotifyingObserver(imgStatusTracker* aTracker)
  : mTracker(aTracker) {}

  virtual ~imgStatusTrackerNotifyingObserver() {}

  void SetTracker(imgStatusTracker* aTracker) {
    mTracker = aTracker;
  }

  

  virtual void OnStartDecode()
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerNotifyingObserver::OnStartDecode");
    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "OnStartDecode callback before we've created our image");

    mTracker->RecordStartDecode();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
    while (iter.HasMore()) {
      mTracker->SendStartDecode(iter.GetNext());
    }

    if (!mTracker->IsMultipart()) {
      mTracker->RecordBlockOnload();

      nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
      while (iter.HasMore()) {
        mTracker->SendBlockOnload(iter.GetNext());
      }
    }
  }

  virtual void OnStartRequest()
  {
    NS_NOTREACHED("imgStatusTrackerNotifyingObserver(imgDecoderObserver)::OnStartRequest");
  }

  virtual void OnStartContainer()
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerNotifyingObserver::OnStartContainer");

    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "OnStartContainer callback before we've created our image");
    mTracker->RecordStartContainer(mTracker->GetImage());

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
    while (iter.HasMore()) {
      mTracker->SendStartContainer(iter.GetNext());
    }
  }

  virtual void OnStartFrame()
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerNotifyingObserver::OnStartFrame");
    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "OnStartFrame callback before we've created our image");

    mTracker->RecordStartFrame();

    
    
  }

  virtual void FrameChanged(const nsIntRect* dirtyRect)
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerNotifyingObserver::FrameChanged");
    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "FrameChanged callback before we've created our image");

    mTracker->RecordFrameChanged(dirtyRect);

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
    while (iter.HasMore()) {
      mTracker->SendFrameChanged(iter.GetNext(), dirtyRect);
    }
  }

  virtual void OnStopFrame()
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerNotifyingObserver::OnStopFrame");
    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "OnStopFrame callback before we've created our image");

    mTracker->RecordStopFrame();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
    while (iter.HasMore()) {
      mTracker->SendStopFrame(iter.GetNext());
    }

    mTracker->MaybeUnblockOnload();
  }

  virtual void OnStopDecode(nsresult aStatus)
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerNotifyingObserver::OnStopDecode");
    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "OnStopDecode callback before we've created our image");

    bool preexistingError = mTracker->GetImageStatus() == imgIRequest::STATUS_ERROR;

    mTracker->RecordStopDecode(aStatus);

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
    while (iter.HasMore()) {
      mTracker->SendStopDecode(iter.GetNext(), aStatus);
    }

    
    
    mTracker->MaybeUnblockOnload();

    if (NS_FAILED(aStatus) && !preexistingError) {
      mTracker->FireFailureNotification();
    }
  }

  virtual void OnStopRequest(bool aLastPart, nsresult aStatus)
  {
    NS_NOTREACHED("imgStatusTrackerNotifyingObserver(imgDecoderObserver)::OnStopRequest");
  }

  virtual void OnDiscard()
  {
    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "OnDiscard callback before we've created our image");

    mTracker->RecordDiscard();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
    while (iter.HasMore()) {
      mTracker->SendDiscard(iter.GetNext());
    }
  }

  virtual void OnUnlockedDraw()
  {
    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "OnUnlockedDraw callback before we've created our image");
    mTracker->RecordUnlockedDraw();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
    while (iter.HasMore()) {
      mTracker->SendUnlockedDraw(iter.GetNext());
    }
  }

  virtual void OnImageIsAnimated()
  {
    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "OnImageIsAnimated callback before we've created our image");
    mTracker->RecordImageIsAnimated();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mTracker->mConsumers);
    while (iter.HasMore()) {
      mTracker->SendImageIsAnimated(iter.GetNext());
    }
  }

  virtual void OnError()
  {
    mTracker->RecordError();
  }

private:
  imgStatusTracker* mTracker;
};

class imgStatusTrackerObserver : public imgDecoderObserver
{
public:
  imgStatusTrackerObserver(imgStatusTracker* aTracker)
  : mTracker(aTracker) {}

  virtual ~imgStatusTrackerObserver() {}

  void SetTracker(imgStatusTracker* aTracker) {
    mTracker = aTracker;
  }

  

  virtual void OnStartDecode() MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnStartDecode");
    mTracker->RecordStartDecode();
    if (!mTracker->IsMultipart()) {
      mTracker->RecordBlockOnload();
    }
  }

  virtual void OnStartRequest() MOZ_OVERRIDE
  {
    NS_NOTREACHED("imgStatusTrackerObserver(imgDecoderObserver)::OnStartRequest");
  }

  virtual void OnStartContainer() MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnStartContainer");
    mTracker->RecordStartContainer(mTracker->GetImage());
  }

  virtual void OnStartFrame() MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnStartFrame");
    mTracker->RecordStartFrame();
  }

  virtual void FrameChanged(const nsIntRect* dirtyRect) MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::FrameChanged");
    mTracker->RecordFrameChanged(dirtyRect);
  }

  virtual void OnStopFrame() MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnStopFrame");
    mTracker->RecordStopFrame();
    mTracker->RecordUnblockOnload();
  }

  virtual void OnStopDecode(nsresult aStatus) MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnStopDecode");
    mTracker->RecordStopDecode(aStatus);

    
    
    mTracker->RecordUnblockOnload();
  }

  virtual void OnStopRequest(bool aLastPart, nsresult aStatus) MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnStopRequest");
    mTracker->RecordStopRequest(aLastPart, aStatus);
  }

  virtual void OnDiscard() MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnDiscard");
    mTracker->RecordDiscard();
  }

  virtual void OnUnlockedDraw() MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnUnlockedDraw");
    NS_ABORT_IF_FALSE(mTracker->GetImage(),
                      "OnUnlockedDraw callback before we've created our image");
    mTracker->RecordUnlockedDraw();
  }

  virtual void OnImageIsAnimated() MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnImageIsAnimated");
    mTracker->RecordImageIsAnimated();
  }

  virtual void OnError() MOZ_OVERRIDE
  {
    LOG_SCOPE(GetImgLog(), "imgStatusTrackerObserver::OnError");
    mTracker->RecordError();
  }

private:
  imgStatusTracker* mTracker;
};



imgStatusTracker::imgStatusTracker(Image* aImage)
  : mImage(aImage),
    mState(0),
    mImageStatus(imgIRequest::STATUS_NONE),
    mIsMultipart(false),
    mHadLastPart(false)
{
  mTrackerObserver = new imgStatusTrackerObserver(this);
}


imgStatusTracker::imgStatusTracker(const imgStatusTracker& aOther)
  : mImage(aOther.mImage),
    mState(aOther.mState),
    mImageStatus(aOther.mImageStatus),
    mIsMultipart(aOther.mIsMultipart),
    mHadLastPart(aOther.mHadLastPart)
    
    
    
    
    
    
    
{
  mTrackerObserver = new imgStatusTrackerObserver(this);
}

imgStatusTracker::~imgStatusTracker()
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
    imgRequestNotifyRunnable(imgStatusTracker* aTracker, imgRequestProxy* aRequestProxy)
      : mTracker(aTracker)
    {
      mProxies.AppendElement(aRequestProxy);
    }

    NS_IMETHOD Run()
    {
      for (uint32_t i = 0; i < mProxies.Length(); ++i) {
        mProxies[i]->SetNotificationsDeferred(false);
        mTracker->SyncNotify(mProxies[i]);
      }

      mTracker->mRequestRunnable = nullptr;
      return NS_OK;
    }

    void AddProxy(imgRequestProxy* aRequestProxy)
    {
      mProxies.AppendElement(aRequestProxy);
    }

    void RemoveProxy(imgRequestProxy* aRequestProxy)
    {
      mProxies.RemoveElement(aRequestProxy);
    }

  private:
    friend class imgStatusTracker;

    nsRefPtr<imgStatusTracker> mTracker;
    nsTArray< nsRefPtr<imgRequestProxy> > mProxies;
};

void
imgStatusTracker::Notify(imgRequestProxy* proxy)
{
#ifdef PR_LOGGING
  if (GetImage() && GetImage()->GetURI()) {
    nsCOMPtr<nsIURI> uri(GetImage()->GetURI());
    nsAutoCString spec;
    uri->GetSpec(spec);
    LOG_FUNC_WITH_PARAM(GetImgLog(), "imgStatusTracker::Notify async", "uri", spec.get());
  } else {
    LOG_FUNC_WITH_PARAM(GetImgLog(), "imgStatusTracker::Notify async", "uri", "<unknown>");
  }
#endif

  proxy->SetNotificationsDeferred(true);

  
  
  
  imgRequestNotifyRunnable* runnable = static_cast<imgRequestNotifyRunnable*>(mRequestRunnable.get());
  if (runnable) {
    runnable->AddProxy(proxy);
  } else {
    mRequestRunnable = new imgRequestNotifyRunnable(this, proxy);
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
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgStatusTracker::NotifyCurrentState", "uri", spec.get());
#endif

  proxy->SetNotificationsDeferred(true);

  
  nsCOMPtr<nsIRunnable> ev = new imgStatusNotifyRunnable(*this, proxy);
  NS_DispatchToCurrentThread(ev);
}

#define NOTIFY_IMAGE_OBSERVERS(func) \
  do { \
    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(proxies); \
    while (iter.HasMore()) { \
      nsRefPtr<imgRequestProxy> proxy = iter.GetNext(); \
      if (!proxy->NotificationsDeferred()) { \
        proxy->func; \
      } \
    } \
  } while (false);

 void
imgStatusTracker::SyncNotifyState(nsTObserverArray<imgRequestProxy*>& proxies,
                                  bool hasImage, uint32_t state,
                                  nsIntRect& dirtyRect, bool hadLastPart)
{
  
  if (state & stateRequestStarted)
    NOTIFY_IMAGE_OBSERVERS(OnStartRequest());

  
  if (state & stateHasSize)
    NOTIFY_IMAGE_OBSERVERS(OnStartContainer());

  
  if (state & stateDecodeStarted)
    NOTIFY_IMAGE_OBSERVERS(OnStartDecode());

  
  if (state & stateBlockingOnload)
    NOTIFY_IMAGE_OBSERVERS(BlockOnload());

  if (hasImage) {
    
    
    
    
    if (!dirtyRect.IsEmpty())
      NOTIFY_IMAGE_OBSERVERS(OnFrameUpdate(&dirtyRect));

    if (state & stateFrameStopped)
      NOTIFY_IMAGE_OBSERVERS(OnStopFrame());

    
    if (state & stateImageIsAnimated)
      NOTIFY_IMAGE_OBSERVERS(OnImageIsAnimated());
  }

  if (state & stateDecodeStopped) {
    NS_ABORT_IF_FALSE(hasImage, "stopped decoding without ever having an image?");
    NOTIFY_IMAGE_OBSERVERS(OnStopDecode());
  }

  if (state & stateRequestStopped) {
    NOTIFY_IMAGE_OBSERVERS(OnStopRequest(hadLastPart));
  }
}

void
imgStatusTracker::SyncAndSyncNotifyDifference(imgStatusTracker* other)
{
  LOG_SCOPE(GetImgLog(), "imgStatusTracker::SyncAndSyncNotifyDifference");

  
  uint32_t loadState = mState & stateRequestStarted;
  uint32_t diffState = ~mState & other->mState & ~stateRequestStarted;
  bool unblockedOnload = mState & stateBlockingOnload && !(other->mState & stateBlockingOnload);
  bool foundError = (mImageStatus != imgIRequest::STATUS_ERROR) && (other->mImageStatus == imgIRequest::STATUS_ERROR);

  
  

  
  mInvalidRect = mInvalidRect.Union(other->mInvalidRect);
  mState |= diffState | loadState;
  if (unblockedOnload) {
    mState &= ~stateBlockingOnload;
  }
  mImageStatus = other->mImageStatus;
  mIsMultipart = other->mIsMultipart;
  mHadLastPart = other->mHadLastPart;
  mImageStatus |= other->mImageStatus;

  
  if (mImageStatus & imgIRequest::STATUS_ERROR) {
    mImageStatus = imgIRequest::STATUS_ERROR;
  } else {
    
    if (!(other->mImageStatus & imgIRequest::STATUS_DECODE_STARTED)) {
      mImageStatus &= ~imgIRequest::STATUS_DECODE_STARTED;
    }
  }

  SyncNotifyState(mConsumers, !!mImage, diffState, mInvalidRect, mHadLastPart);

  if (unblockedOnload) {
    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mConsumers);
    while (iter.HasMore()) {
      
      
      nsRefPtr<imgRequestProxy> proxy = iter.GetNext();

      if (!proxy->NotificationsDeferred()) {
        SendUnblockOnload(proxy);
      }
    }
  }

  
  other->mInvalidRect.SetEmpty();
  mInvalidRect.SetEmpty();

  if (foundError) {
    FireFailureNotification();
  }
}

imgStatusTracker*
imgStatusTracker::CloneForRecording()
{
  imgStatusTracker* clone = new imgStatusTracker(*this);
  return clone;
}

void
imgStatusTracker::SyncNotify(imgRequestProxy* proxy)
{
#ifdef PR_LOGGING
  nsCOMPtr<nsIURI> uri;
  proxy->GetURI(getter_AddRefs(uri));
  nsAutoCString spec;
  uri->GetSpec(spec);
  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgStatusTracker::SyncNotify", "uri", spec.get());
#endif

  nsIntRect r;
  if (mImage) {
    
    
    r = mImage->FrameRect(imgIContainer::FRAME_CURRENT);
  }

  nsTObserverArray<imgRequestProxy*> array;
  array.AppendElement(proxy);
  SyncNotifyState(array, !!mImage, mState, r, mHadLastPart);
}

void
imgStatusTracker::SyncNotifyDecodeState()
{
  LOG_SCOPE(GetImgLog(), "imgStatusTracker::SyncNotifyDecodeState");

  SyncNotifyState(mConsumers, !!mImage, mState & ~stateRequestStarted, mInvalidRect, mHadLastPart);

  mInvalidRect.SetEmpty();
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
imgStatusTracker::RemoveConsumer(imgRequestProxy* aConsumer, nsresult aStatus)
{
  
  bool removed = mConsumers.RemoveElement(aConsumer);

  
  
  if (removed && !aConsumer->NotificationsDeferred()) {
    EmulateRequestFinished(aConsumer, aStatus);
  }

  
  
  imgRequestNotifyRunnable* runnable = static_cast<imgRequestNotifyRunnable*>(mRequestRunnable.get());
  if (aConsumer->NotificationsDeferred() && runnable) {
    runnable->RemoveProxy(aConsumer);
    aConsumer->SetNotificationsDeferred(false);
  }

  return removed;
}

void
imgStatusTracker::RecordCancel()
{
  if (!(mImageStatus & imgIRequest::STATUS_LOAD_PARTIAL))
    mImageStatus = imgIRequest::STATUS_ERROR;
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
  mImageStatus &= ~imgIRequest::STATUS_DECODE_STARTED;
}

void
imgStatusTracker::RecordStartDecode()
{
  NS_ABORT_IF_FALSE(mImage, "RecordStartDecode without an Image");
  mState |= stateDecodeStarted;
  mImageStatus |= imgIRequest::STATUS_DECODE_STARTED;
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
  mInvalidRect.SetEmpty();
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

  if (NS_SUCCEEDED(aStatus) && mImageStatus != imgIRequest::STATUS_ERROR) {
    mImageStatus |= imgIRequest::STATUS_DECODE_COMPLETE;
    mImageStatus &= ~imgIRequest::STATUS_DECODE_STARTED;
  
  } else {
    mImageStatus = imgIRequest::STATUS_ERROR;
  }
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
  
  uint32_t stateBitsToClear = stateDecodeStopped;
  mState &= ~stateBitsToClear;

  
  uint32_t statusBitsToClear = imgIRequest::STATUS_DECODE_STARTED |
                               imgIRequest::STATUS_FRAME_COMPLETE |
                               imgIRequest::STATUS_DECODE_COMPLETE;
  mImageStatus &= ~statusBitsToClear;
}

void
imgStatusTracker::SendDiscard(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnDiscard();
}


void
imgStatusTracker::RecordUnlockedDraw()
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordUnlockedDraw called before we have an Image");
}

void
imgStatusTracker::RecordImageIsAnimated()
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordImageIsAnimated called before we have an Image");
  mState |= stateImageIsAnimated;
}

void
imgStatusTracker::SendImageIsAnimated(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnImageIsAnimated();
}

void
imgStatusTracker::SendUnlockedDraw(imgRequestProxy* aProxy)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnUnlockedDraw();
}

void
imgStatusTracker::OnUnlockedDraw()
{
  RecordUnlockedDraw();
  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    SendUnlockedDraw(iter.GetNext());
  }
}

void
imgStatusTracker::RecordFrameChanged(const nsIntRect* aDirtyRect)
{
  NS_ABORT_IF_FALSE(mImage,
                    "RecordFrameChanged called before we have an Image");
  mInvalidRect = mInvalidRect.Union(*aDirtyRect);
}

void
imgStatusTracker::SendFrameChanged(imgRequestProxy* aProxy,
                                   const nsIntRect* aDirtyRect)
{
  if (!aProxy->NotificationsDeferred())
    aProxy->OnFrameUpdate(aDirtyRect);
}


void
imgStatusTracker::RecordStartRequest()
{
  
  
  mImageStatus &= ~imgIRequest::STATUS_LOAD_PARTIAL;
  mImageStatus &= ~imgIRequest::STATUS_LOAD_COMPLETE;
  mImageStatus &= ~imgIRequest::STATUS_FRAME_COMPLETE;
  mImageStatus &= ~imgIRequest::STATUS_DECODE_STARTED;
  mImageStatus &= ~imgIRequest::STATUS_DECODE_COMPLETE;
  mState &= ~stateRequestStarted;
  mState &= ~stateDecodeStarted;
  mState &= ~stateDecodeStopped;
  mState &= ~stateRequestStopped;
  mState &= ~stateBlockingOnload;
  mState &= ~stateImageIsAnimated;

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
    FireFailureNotification();
  }
}

void
imgStatusTracker::OnDiscard()
{
  RecordDiscard();

  
  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    SendDiscard(iter.GetNext());
  }
}

void
imgStatusTracker::FrameChanged(const nsIntRect* aDirtyRect)
{
  RecordFrameChanged(aDirtyRect);

  
  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    SendFrameChanged(iter.GetNext(), aDirtyRect);
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
  if (!(mState & stateBlockingOnload)) {
    return;
  }

  RecordUnblockOnload();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    SendUnblockOnload(iter.GetNext());
  }
}

void
imgStatusTracker::RecordError()
{
  mImageStatus = imgIRequest::STATUS_ERROR;
}

void
imgStatusTracker::FireFailureNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  if (GetImage()) {
    nsCOMPtr<nsIURI> uri = GetImage()->GetURI();
    if (uri) {
      nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
      if (os) {
        os->NotifyObservers(uri, "net:failed-to-process-uri-content", nullptr);
      }
    }
  }
}
