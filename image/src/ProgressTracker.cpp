





#include "ImageLogging.h"
#include "ProgressTracker.h"

#include "imgIContainer.h"
#include "imgRequestProxy.h"
#include "Image.h"
#include "nsNetUtil.h"
#include "nsIObserverService.h"

#include "mozilla/Assertions.h"
#include "mozilla/Services.h"

using mozilla::WeakPtr;

namespace mozilla {
namespace image {

ProgressTrackerInit::ProgressTrackerInit(Image* aImage,
                                         ProgressTracker* aTracker)
{
  MOZ_ASSERT(aImage);

  if (aTracker) {
    mTracker = aTracker;
    mTracker->SetImage(aImage);
  } else {
    mTracker = new ProgressTracker(aImage);
  }
  aImage->SetProgressTracker(mTracker);
  MOZ_ASSERT(mTracker);
}

ProgressTrackerInit::~ProgressTrackerInit()
{
  mTracker->ResetImage();
}

static void
CheckProgressConsistency(Progress aProgress)
{
  

  if (aProgress & FLAG_HAS_SIZE) {
    
  }
  if (aProgress & FLAG_DECODE_STARTED) {
    
  }
  if (aProgress & FLAG_DECODE_STOPPED) {
    MOZ_ASSERT(aProgress & FLAG_DECODE_STARTED);
  }
  if (aProgress & FLAG_FRAME_STOPPED) {
    MOZ_ASSERT(aProgress & FLAG_DECODE_STARTED);
  }
  if (aProgress & FLAG_REQUEST_STOPPED) {
    
  }
  if (aProgress & FLAG_ONLOAD_BLOCKED) {
    if (aProgress & FLAG_IS_MULTIPART) {
      MOZ_ASSERT(aProgress & FLAG_ONLOAD_UNBLOCKED);
    } else {
      MOZ_ASSERT(aProgress & FLAG_DECODE_STARTED);
    }
  }
  if (aProgress & FLAG_ONLOAD_UNBLOCKED) {
    MOZ_ASSERT(aProgress & FLAG_ONLOAD_BLOCKED);
    MOZ_ASSERT(aProgress & (FLAG_FRAME_STOPPED |
                            FLAG_IS_MULTIPART |
                            FLAG_HAS_ERROR));
  }
  if (aProgress & FLAG_IS_ANIMATED) {
    MOZ_ASSERT(aProgress & FLAG_DECODE_STARTED);
    MOZ_ASSERT(aProgress & FLAG_HAS_SIZE);
  }
  if (aProgress & FLAG_HAS_TRANSPARENCY) {
    MOZ_ASSERT(aProgress & FLAG_DECODE_STARTED);
    MOZ_ASSERT(aProgress & FLAG_HAS_SIZE);
  }
  if (aProgress & FLAG_IS_MULTIPART) {
    
  }
  if (aProgress & FLAG_MULTIPART_STOPPED) {
    MOZ_ASSERT(aProgress & FLAG_REQUEST_STOPPED);
  }
  if (aProgress & FLAG_HAS_ERROR) {
    
  }
}

void
ProgressTracker::SetImage(Image* aImage)
{
  NS_ABORT_IF_FALSE(aImage, "Setting null image");
  NS_ABORT_IF_FALSE(!mImage, "Setting image when we already have one");
  mImage = aImage;
}

void
ProgressTracker::ResetImage()
{
  NS_ABORT_IF_FALSE(mImage, "Resetting image when it's already null!");
  mImage = nullptr;
}

void ProgressTracker::SetIsMultipart()
{
  if (mProgress & FLAG_IS_MULTIPART) {
    return;
  }

  MOZ_ASSERT(!(mProgress & FLAG_ONLOAD_BLOCKED),
             "Blocked onload before we knew we were multipart?");

  
  mProgress |= FLAG_IS_MULTIPART | FLAG_ONLOAD_BLOCKED | FLAG_ONLOAD_UNBLOCKED;

  CheckProgressConsistency(mProgress);
}

bool
ProgressTracker::IsLoading() const
{
  
  
  
  return !(mProgress & FLAG_REQUEST_STOPPED);
}

uint32_t
ProgressTracker::GetImageStatus() const
{
  uint32_t status = imgIRequest::STATUS_NONE;

  
  if (mProgress & FLAG_HAS_SIZE) {
    status |= imgIRequest::STATUS_SIZE_AVAILABLE;
  }
  if (mProgress & FLAG_DECODE_STARTED) {
    status |= imgIRequest::STATUS_DECODE_STARTED;
  }
  if (mProgress & FLAG_DECODE_STOPPED) {
    status |= imgIRequest::STATUS_DECODE_COMPLETE;
  }
  if (mProgress & FLAG_FRAME_STOPPED) {
    status |= imgIRequest::STATUS_FRAME_COMPLETE;
  }
  if (mProgress & FLAG_REQUEST_STOPPED) {
    status |= imgIRequest::STATUS_LOAD_COMPLETE;
  }
  if (mProgress & FLAG_HAS_ERROR) {
    status |= imgIRequest::STATUS_ERROR;
  }

  return status;
}


class AsyncNotifyRunnable : public nsRunnable
{
  public:
    AsyncNotifyRunnable(ProgressTracker* aTracker,
                        imgRequestProxy* aRequestProxy)
      : mTracker(aTracker)
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be created on the main thread");
      MOZ_ASSERT(aTracker, "aTracker should not be null");
      MOZ_ASSERT(aRequestProxy, "aRequestProxy should not be null");
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

      mTracker->mRunnable = nullptr;
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
    friend class ProgressTracker;

    nsRefPtr<ProgressTracker> mTracker;
    nsTArray<nsRefPtr<imgRequestProxy>> mProxies;
};

void
ProgressTracker::Notify(imgRequestProxy* proxy)
{
  MOZ_ASSERT(NS_IsMainThread(), "imgRequestProxy is not threadsafe");
#ifdef PR_LOGGING
  if (mImage && mImage->GetURI()) {
    nsRefPtr<ImageURL> uri(mImage->GetURI());
    nsAutoCString spec;
    uri->GetSpec(spec);
    LOG_FUNC_WITH_PARAM(GetImgLog(), "ProgressTracker::Notify async", "uri", spec.get());
  } else {
    LOG_FUNC_WITH_PARAM(GetImgLog(), "ProgressTracker::Notify async", "uri", "<unknown>");
  }
#endif

  proxy->SetNotificationsDeferred(true);

  
  
  
  AsyncNotifyRunnable* runnable =
    static_cast<AsyncNotifyRunnable*>(mRunnable.get());

  if (runnable) {
    runnable->AddProxy(proxy);
  } else {
    mRunnable = new AsyncNotifyRunnable(this, proxy);
    NS_DispatchToCurrentThread(mRunnable);
  }
}



class AsyncNotifyCurrentStateRunnable : public nsRunnable
{
  public:
    AsyncNotifyCurrentStateRunnable(ProgressTracker* aProgressTracker,
                                    imgRequestProxy* aProxy)
      : mProgressTracker(aProgressTracker)
      , mProxy(aProxy)
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be created on the main thread");
      MOZ_ASSERT(mProgressTracker, "mProgressTracker should not be null");
      MOZ_ASSERT(mProxy, "mProxy should not be null");
      mImage = mProgressTracker->GetImage();
    }

    NS_IMETHOD Run()
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be running on the main thread");
      mProxy->SetNotificationsDeferred(false);

      mProgressTracker->SyncNotify(mProxy);
      return NS_OK;
    }

  private:
    nsRefPtr<ProgressTracker> mProgressTracker;
    nsRefPtr<imgRequestProxy> mProxy;

    
    
    nsRefPtr<Image> mImage;
};

void
ProgressTracker::NotifyCurrentState(imgRequestProxy* proxy)
{
  MOZ_ASSERT(NS_IsMainThread(), "imgRequestProxy is not threadsafe");
#ifdef PR_LOGGING
  nsRefPtr<ImageURL> uri;
  proxy->GetURI(getter_AddRefs(uri));
  nsAutoCString spec;
  uri->GetSpec(spec);
  LOG_FUNC_WITH_PARAM(GetImgLog(), "ProgressTracker::NotifyCurrentState", "uri", spec.get());
#endif

  proxy->SetNotificationsDeferred(true);

  nsCOMPtr<nsIRunnable> ev = new AsyncNotifyCurrentStateRunnable(this, proxy);
  NS_DispatchToCurrentThread(ev);
}

#define NOTIFY_IMAGE_OBSERVERS(PROXIES, FUNC) \
  do { \
    ProxyArray::ForwardIterator iter(PROXIES); \
    while (iter.HasMore()) { \
      nsRefPtr<imgRequestProxy> proxy = iter.GetNext().get(); \
      if (proxy && !proxy->NotificationsDeferred()) { \
        proxy->FUNC; \
      } \
    } \
  } while (false);

 void
ProgressTracker::SyncNotifyInternal(ProxyArray& aProxies,
                                    bool aHasImage,
                                    Progress aProgress,
                                    const nsIntRect& aDirtyRect)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (aProgress & FLAG_HAS_SIZE)
    NOTIFY_IMAGE_OBSERVERS(aProxies, OnSizeAvailable());

  if (aProgress & FLAG_DECODE_STARTED)
    NOTIFY_IMAGE_OBSERVERS(aProxies, OnStartDecode());

  if (aProgress & FLAG_ONLOAD_BLOCKED)
    NOTIFY_IMAGE_OBSERVERS(aProxies, BlockOnload());

  if (aHasImage) {
    
    
    
    
    if (!aDirtyRect.IsEmpty())
      NOTIFY_IMAGE_OBSERVERS(aProxies, OnFrameUpdate(&aDirtyRect));

    if (aProgress & FLAG_FRAME_STOPPED)
      NOTIFY_IMAGE_OBSERVERS(aProxies, OnFrameComplete());

    if (aProgress & FLAG_HAS_TRANSPARENCY)
      NOTIFY_IMAGE_OBSERVERS(aProxies, OnImageHasTransparency());

    if (aProgress & FLAG_IS_ANIMATED)
      NOTIFY_IMAGE_OBSERVERS(aProxies, OnImageIsAnimated());
  }

  
  
  
  if (aProgress & FLAG_ONLOAD_UNBLOCKED) {
    NOTIFY_IMAGE_OBSERVERS(aProxies, UnblockOnload());
  }

  if (aProgress & FLAG_DECODE_STOPPED) {
    MOZ_ASSERT(aHasImage, "Stopped decoding without ever having an image?");
    NOTIFY_IMAGE_OBSERVERS(aProxies, OnDecodeComplete());
  }

  if (aProgress & FLAG_REQUEST_STOPPED) {
    NOTIFY_IMAGE_OBSERVERS(aProxies,
                           OnLoadComplete(aProgress & FLAG_MULTIPART_STOPPED));
  }
}

void
ProgressTracker::SyncNotifyProgress(Progress aProgress,
                                    const nsIntRect& aInvalidRect )
{
  MOZ_ASSERT(NS_IsMainThread(), "Use mConsumers on main thread only");

  
  Progress progress = Difference(aProgress);
  if (!((mProgress | progress) & FLAG_ONLOAD_BLOCKED)) {
    progress &= ~FLAG_ONLOAD_UNBLOCKED;
  }

  
  mProgress |= progress;

  CheckProgressConsistency(mProgress);

  
  SyncNotifyInternal(mConsumers, !!mImage, progress, aInvalidRect);

  if (progress & FLAG_HAS_ERROR) {
    FireFailureNotification();
  }
}

void
ProgressTracker::SyncNotify(imgRequestProxy* proxy)
{
  MOZ_ASSERT(NS_IsMainThread(), "imgRequestProxy is not threadsafe");
#ifdef PR_LOGGING
  nsRefPtr<ImageURL> uri;
  proxy->GetURI(getter_AddRefs(uri));
  nsAutoCString spec;
  uri->GetSpec(spec);
  LOG_SCOPE_WITH_PARAM(GetImgLog(), "ProgressTracker::SyncNotify", "uri", spec.get());
#endif

  nsIntRect r;
  if (mImage) {
    
    
    r = mImage->FrameRect(imgIContainer::FRAME_CURRENT);
  }

  ProxyArray array;
  array.AppendElement(proxy);
  SyncNotifyInternal(array, !!mImage, mProgress, r);
}

void
ProgressTracker::EmulateRequestFinished(imgRequestProxy* aProxy,
                                        nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread(),
             "SyncNotifyState and mConsumers are not threadsafe");
  nsCOMPtr<imgIRequest> kungFuDeathGrip(aProxy);

  if (mProgress & FLAG_ONLOAD_BLOCKED && !(mProgress & FLAG_ONLOAD_UNBLOCKED)) {
    aProxy->UnblockOnload();
  }

  if (!(mProgress & FLAG_REQUEST_STOPPED)) {
    aProxy->OnLoadComplete(true);
  }
}

void
ProgressTracker::AddConsumer(imgRequestProxy* aConsumer)
{
  MOZ_ASSERT(NS_IsMainThread());
  mConsumers.AppendElementUnlessExists(aConsumer);
}


bool
ProgressTracker::RemoveConsumer(imgRequestProxy* aConsumer, nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  bool removed = mConsumers.RemoveElement(aConsumer);

  
  
  if (removed && !aConsumer->NotificationsDeferred()) {
    EmulateRequestFinished(aConsumer, aStatus);
  }

  
  
  AsyncNotifyRunnable* runnable =
    static_cast<AsyncNotifyRunnable*>(mRunnable.get());

  if (aConsumer->NotificationsDeferred() && runnable) {
    runnable->RemoveProxy(aConsumer);
    aConsumer->SetNotificationsDeferred(false);
  }

  return removed;
}

bool
ProgressTracker::FirstConsumerIs(imgRequestProxy* aConsumer)
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
ProgressTracker::OnUnlockedDraw()
{
  MOZ_ASSERT(NS_IsMainThread());
  NOTIFY_IMAGE_OBSERVERS(mConsumers, OnUnlockedDraw());
}

void
ProgressTracker::ResetForNewRequest()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  mProgress &= FLAG_IS_MULTIPART | FLAG_HAS_ERROR |
               FLAG_ONLOAD_BLOCKED | FLAG_ONLOAD_UNBLOCKED;

  CheckProgressConsistency(mProgress);
}

void
ProgressTracker::OnDiscard()
{
  MOZ_ASSERT(NS_IsMainThread());
  NOTIFY_IMAGE_OBSERVERS(mConsumers, OnDiscard());
}

void
ProgressTracker::OnImageAvailable()
{
  if (!NS_IsMainThread()) {
    
    
    
    NS_DispatchToMainThread(
      NS_NewRunnableMethod(this, &ProgressTracker::OnImageAvailable));
    return;
  }

  NOTIFY_IMAGE_OBSERVERS(mConsumers, SetHasImage());
}

void
ProgressTracker::FireFailureNotification()
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

} 
} 
