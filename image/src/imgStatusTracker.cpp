





#include "ImageLogging.h"
#include "imgStatusTracker.h"

#include "imgIContainer.h"
#include "imgRequestProxy.h"
#include "Image.h"
#include "nsNetUtil.h"
#include "nsIObserverService.h"

#include "mozilla/Assertions.h"
#include "mozilla/Services.h"

using namespace mozilla::image;
using mozilla::WeakPtr;

imgStatusTrackerInit::imgStatusTrackerInit(mozilla::image::Image* aImage,
                                           imgStatusTracker* aTracker)
{
  MOZ_ASSERT(aImage);

  if (aTracker) {
    mTracker = aTracker;
    mTracker->SetImage(aImage);
  } else {
    mTracker = new imgStatusTracker(aImage);
  }
  aImage->SetStatusTracker(mTracker);
  MOZ_ASSERT(mTracker);
}

imgStatusTrackerInit::~imgStatusTrackerInit()
{
  mTracker->ResetImage();
}

void
imgStatusTracker::SetImage(Image* aImage)
{
  NS_ABORT_IF_FALSE(aImage, "Setting null image");
  NS_ABORT_IF_FALSE(!mImage, "Setting image when we already have one");
  mImage = aImage;
}

void
imgStatusTracker::ResetImage()
{
  NS_ABORT_IF_FALSE(mImage, "Resetting image when it's already null!");
  mImage = nullptr;
}

void imgStatusTracker::SetIsMultipart()
{
  mState |= FLAG_IS_MULTIPART;

  
  if (!(mState & FLAG_ONLOAD_BLOCKED)) {
    mState |= FLAG_ONLOAD_BLOCKED | FLAG_ONLOAD_UNBLOCKED;
  }
}

bool
imgStatusTracker::IsLoading() const
{
  
  
  
  return !(mState & FLAG_REQUEST_STOPPED);
}

uint32_t
imgStatusTracker::GetImageStatus() const
{
  uint32_t status = imgIRequest::STATUS_NONE;

  
  if (mState & FLAG_HAS_SIZE) {
    status |= imgIRequest::STATUS_SIZE_AVAILABLE;
  }
  if (mState & FLAG_DECODE_STARTED) {
    status |= imgIRequest::STATUS_DECODE_STARTED;
  }
  if (mState & FLAG_DECODE_STOPPED) {
    status |= imgIRequest::STATUS_DECODE_COMPLETE;
  }
  if (mState & FLAG_FRAME_STOPPED) {
    status |= imgIRequest::STATUS_FRAME_COMPLETE;
  }
  if (mState & FLAG_REQUEST_STOPPED) {
    status |= imgIRequest::STATUS_LOAD_COMPLETE;
  }
  if (mState & FLAG_HAS_ERROR) {
    status |= imgIRequest::STATUS_ERROR;
  }

  return status;
}


class imgRequestNotifyRunnable : public nsRunnable
{
  public:
    imgRequestNotifyRunnable(imgStatusTracker* aTracker,
                             imgRequestProxy* aRequestProxy)
      : mTracker(aTracker)
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be created on the main thread");
      MOZ_ASSERT(aRequestProxy, "aRequestProxy should not be null");
      MOZ_ASSERT(aTracker, "aTracker should not be null");
      mProxies.AppendElement(aRequestProxy);
    }

    NS_IMETHOD Run()
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be running on the main thread");
      MOZ_ASSERT(mTracker, "mTracker should not be null");
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
  MOZ_ASSERT(NS_IsMainThread(), "imgRequestProxy is not threadsafe");
#ifdef PR_LOGGING
  if (mImage && mImage->GetURI()) {
    nsRefPtr<ImageURL> uri(mImage->GetURI());
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
    imgStatusNotifyRunnable(imgStatusTracker* statusTracker,
                            imgRequestProxy* requestproxy)
      : mStatusTracker(statusTracker), mProxy(requestproxy)
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be created on the main thread");
      MOZ_ASSERT(requestproxy, "requestproxy cannot be null");
      MOZ_ASSERT(statusTracker, "status should not be null");
      mImage = statusTracker->GetImage();
    }

    NS_IMETHOD Run()
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be running on the main thread");
      mProxy->SetNotificationsDeferred(false);

      mStatusTracker->SyncNotify(mProxy);
      return NS_OK;
    }

  private:
    nsRefPtr<imgStatusTracker> mStatusTracker;
    
    
    nsRefPtr<Image> mImage;
    nsRefPtr<imgRequestProxy> mProxy;
};

void
imgStatusTracker::NotifyCurrentState(imgRequestProxy* proxy)
{
  MOZ_ASSERT(NS_IsMainThread(), "imgRequestProxy is not threadsafe");
#ifdef PR_LOGGING
  nsRefPtr<ImageURL> uri;
  proxy->GetURI(getter_AddRefs(uri));
  nsAutoCString spec;
  uri->GetSpec(spec);
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgStatusTracker::NotifyCurrentState", "uri", spec.get());
#endif

  proxy->SetNotificationsDeferred(true);

  
  nsCOMPtr<nsIRunnable> ev = new imgStatusNotifyRunnable(this, proxy);
  NS_DispatchToCurrentThread(ev);
}

#define NOTIFY_IMAGE_OBSERVERS(func) \
  do { \
    ProxyArray::ForwardIterator iter(aProxies); \
    while (iter.HasMore()) { \
      nsRefPtr<imgRequestProxy> proxy = iter.GetNext().get(); \
      if (proxy && !proxy->NotificationsDeferred()) { \
        proxy->func; \
      } \
    } \
  } while (false);

 void
imgStatusTracker::SyncNotifyState(ProxyArray& aProxies,
                                  bool aHasImage, uint32_t aState,
                                  const nsIntRect& aDirtyRect)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  if (aState & FLAG_REQUEST_STARTED)
    NOTIFY_IMAGE_OBSERVERS(OnStartRequest());

  
  if (aState & FLAG_HAS_SIZE)
    NOTIFY_IMAGE_OBSERVERS(OnStartContainer());

  
  if (aState & FLAG_DECODE_STARTED)
    NOTIFY_IMAGE_OBSERVERS(OnStartDecode());

  
  if (aState & FLAG_ONLOAD_BLOCKED)
    NOTIFY_IMAGE_OBSERVERS(BlockOnload());

  if (aHasImage) {
    
    
    
    
    if (!aDirtyRect.IsEmpty())
      NOTIFY_IMAGE_OBSERVERS(OnFrameUpdate(&aDirtyRect));

    if (aState & FLAG_FRAME_STOPPED)
      NOTIFY_IMAGE_OBSERVERS(OnStopFrame());

    
    if (aState & FLAG_IS_ANIMATED)
      NOTIFY_IMAGE_OBSERVERS(OnImageIsAnimated());
  }

  
  
  
  if (aState & FLAG_ONLOAD_UNBLOCKED) {
    NOTIFY_IMAGE_OBSERVERS(UnblockOnload());
  }

  if (aState & FLAG_DECODE_STOPPED) {
    MOZ_ASSERT(aHasImage, "Stopped decoding without ever having an image?");
    NOTIFY_IMAGE_OBSERVERS(OnStopDecode());
  }

  if (aState & FLAG_REQUEST_STOPPED) {
    NOTIFY_IMAGE_OBSERVERS(OnStopRequest(aState & FLAG_MULTIPART_STOPPED));
  }
}

ImageStatusDiff
imgStatusTracker::Difference(const ImageStatusDiff& aOther) const
{
  ImageStatusDiff diff;
  diff.diffState = ~mState & aOther.diffState;

  
  if (!((mState | diff.diffState) & FLAG_ONLOAD_BLOCKED)) {
    diff.diffState &= ~FLAG_ONLOAD_UNBLOCKED;
  }

  return diff;
}

void
imgStatusTracker::ApplyDifference(const ImageStatusDiff& aDiff)
{
  LOG_SCOPE(GetImgLog(), "imgStatusTracker::ApplyDifference");
  mState |= aDiff.diffState;
}

void
imgStatusTracker::SyncNotifyDifference(const ImageStatusDiff& aDiff,
                                       const nsIntRect& aInvalidRect )
{
  MOZ_ASSERT(NS_IsMainThread(), "Use mConsumers on main thread only");
  LOG_SCOPE(GetImgLog(), "imgStatusTracker::SyncNotifyDifference");

  SyncNotifyState(mConsumers, !!mImage, aDiff.diffState, aInvalidRect);

  if (aDiff.diffState & FLAG_HAS_ERROR) {
    FireFailureNotification();
  }
}

void
imgStatusTracker::SyncNotify(imgRequestProxy* proxy)
{
  MOZ_ASSERT(NS_IsMainThread(), "imgRequestProxy is not threadsafe");
#ifdef PR_LOGGING
  nsRefPtr<ImageURL> uri;
  proxy->GetURI(getter_AddRefs(uri));
  nsAutoCString spec;
  uri->GetSpec(spec);
  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgStatusTracker::SyncNotify", "uri", spec.get());
#endif

  nsIntRect r;
  if (mImage) {
    
    
    r = mImage->FrameRect(imgIContainer::FRAME_CURRENT);
  }

  ProxyArray array;
  array.AppendElement(proxy);
  SyncNotifyState(array, !!mImage, mState, r);
}

void
imgStatusTracker::EmulateRequestFinished(imgRequestProxy* aProxy,
                                         nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread(),
             "SyncNotifyState and mConsumers are not threadsafe");
  nsCOMPtr<imgIRequest> kungFuDeathGrip(aProxy);

  
  
  if (!(mState & FLAG_REQUEST_STARTED)) {
    aProxy->OnStartRequest();
  }

  if (mState & FLAG_ONLOAD_BLOCKED && !(mState & FLAG_ONLOAD_UNBLOCKED)) {
    aProxy->UnblockOnload();
  }

  if (!(mState & FLAG_REQUEST_STOPPED)) {
    aProxy->OnStopRequest(true);
  }
}

void
imgStatusTracker::AddConsumer(imgRequestProxy* aConsumer)
{
  MOZ_ASSERT(NS_IsMainThread());
  mConsumers.AppendElementUnlessExists(aConsumer);
}


bool
imgStatusTracker::RemoveConsumer(imgRequestProxy* aConsumer, nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());
  
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

bool
imgStatusTracker::FirstConsumerIs(imgRequestProxy* aConsumer)
{
  MOZ_ASSERT(NS_IsMainThread(), "Use mConsumers on main thread only");
  ProxyArray::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    nsRefPtr<imgRequestProxy> proxy = iter.GetNext().get();
    if (proxy) {
      return proxy.get() == aConsumer;
    }
  }
  return false;
}

void
imgStatusTracker::RecordError()
{
  mState |= FLAG_HAS_ERROR;
}

void
imgStatusTracker::SendStartDecode(imgRequestProxy* aProxy)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartDecode();
}

void
imgStatusTracker::SendStartContainer(imgRequestProxy* aProxy)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartContainer();
}

void
imgStatusTracker::SendStopFrame(imgRequestProxy* aProxy)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStopFrame();
}

void
imgStatusTracker::SendStopDecode(imgRequestProxy* aProxy,
                                 nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStopDecode();
}

void
imgStatusTracker::SendDiscard(imgRequestProxy* aProxy)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred())
    aProxy->OnDiscard();
}


void
imgStatusTracker::SendImageIsAnimated(imgRequestProxy* aProxy)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred())
    aProxy->OnImageIsAnimated();
}

void
imgStatusTracker::SendUnlockedDraw(imgRequestProxy* aProxy)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred())
    aProxy->OnUnlockedDraw();
}

void
imgStatusTracker::OnUnlockedDraw()
{
  MOZ_ASSERT(NS_IsMainThread());
  ProxyArray::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    nsRefPtr<imgRequestProxy> proxy = iter.GetNext().get();
    if (proxy) {
      SendUnlockedDraw(proxy);
    }
  }
}


void
imgStatusTracker::SendStartRequest(imgRequestProxy* aProxy)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred())
    aProxy->OnStartRequest();
}

void
imgStatusTracker::OnStartRequest()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  mState &= ~FLAG_REQUEST_STARTED;
  mState &= ~FLAG_DECODE_STARTED;
  mState &= ~FLAG_DECODE_STOPPED;
  mState &= ~FLAG_REQUEST_STOPPED;
  mState &= ~FLAG_ONLOAD_BLOCKED;
  mState &= ~FLAG_ONLOAD_UNBLOCKED;
  mState &= ~FLAG_IS_ANIMATED;

  mState |= FLAG_REQUEST_STARTED;

  ProxyArray::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    nsRefPtr<imgRequestProxy> proxy = iter.GetNext().get();
    if (proxy) {
      SendStartRequest(proxy);
    }
  }
}

void
imgStatusTracker::SendStopRequest(imgRequestProxy* aProxy,
                                  bool aLastPart,
                                  nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred()) {
    aProxy->OnStopRequest(aLastPart);
  }
}

void
imgStatusTracker::OnDiscard()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  ProxyArray::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    nsRefPtr<imgRequestProxy> proxy = iter.GetNext().get();
    if (proxy) {
      SendDiscard(proxy);
    }
  }
}

void
imgStatusTracker::OnDataAvailable()
{
  if (!NS_IsMainThread()) {
    
    
    
    NS_DispatchToMainThread(
      NS_NewRunnableMethod(this, &imgStatusTracker::OnDataAvailable));
    return;
  }
  
  ProxyArray::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    nsRefPtr<imgRequestProxy> proxy = iter.GetNext().get();
    if (proxy) {
      proxy->SetHasImage();
    }
  }
}

void
imgStatusTracker::SendBlockOnload(imgRequestProxy* aProxy)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred()) {
    aProxy->BlockOnload();
  }
}

void
imgStatusTracker::SendUnblockOnload(imgRequestProxy* aProxy)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!aProxy->NotificationsDeferred()) {
    aProxy->UnblockOnload();
  }
}

void
imgStatusTracker::MaybeUnblockOnload()
{
  if (!NS_IsMainThread()) {
    NS_DispatchToMainThread(
      NS_NewRunnableMethod(this, &imgStatusTracker::MaybeUnblockOnload));
    return;
  }
  if (!(mState & FLAG_ONLOAD_BLOCKED) || (mState & FLAG_ONLOAD_UNBLOCKED)) {
    return;
  }

  mState |= FLAG_ONLOAD_UNBLOCKED;

  ProxyArray::ForwardIterator iter(mConsumers);
  while (iter.HasMore()) {
    nsRefPtr<imgRequestProxy> proxy = iter.GetNext().get();
    if (proxy) {
      SendUnblockOnload(proxy);
    }
  }
}

void
imgStatusTracker::FireFailureNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  if (mImage) {
    
    nsCOMPtr<nsIURI> uri;
    {
      nsRefPtr<ImageURL> threadsafeUriData = mImage->GetURI();
      uri = threadsafeUriData ? threadsafeUriData->ToIURI() : nullptr;
    }
    if (uri) {
      nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
      if (os) {
        os->NotifyObservers(uri, "net:failed-to-process-uri-content", nullptr);
      }
    }
  }
}
