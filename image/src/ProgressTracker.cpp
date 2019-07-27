





#include "ImageLogging.h"
#include "ProgressTracker.h"

#include "imgIContainer.h"
#include "imgINotificationObserver.h"
#include "imgIRequest.h"
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
  } else {
    mTracker = new ProgressTracker();
  }
  mTracker->SetImage(aImage);
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
  

  if (aProgress & FLAG_SIZE_AVAILABLE) {
    
  }
  if (aProgress & FLAG_DECODE_STARTED) {
    
  }
  if (aProgress & FLAG_DECODE_COMPLETE) {
    MOZ_ASSERT(aProgress & FLAG_DECODE_STARTED);
  }
  if (aProgress & FLAG_FRAME_COMPLETE) {
    MOZ_ASSERT(aProgress & FLAG_DECODE_STARTED);
  }
  if (aProgress & FLAG_LOAD_COMPLETE) {
    
  }
  if (aProgress & FLAG_ONLOAD_BLOCKED) {
    MOZ_ASSERT(aProgress & FLAG_DECODE_STARTED);
  }
  if (aProgress & FLAG_ONLOAD_UNBLOCKED) {
    MOZ_ASSERT(aProgress & FLAG_ONLOAD_BLOCKED);
    MOZ_ASSERT(aProgress & (FLAG_FRAME_COMPLETE | FLAG_HAS_ERROR));
  }
  if (aProgress & FLAG_IS_ANIMATED) {
    MOZ_ASSERT(aProgress & FLAG_DECODE_STARTED);
    MOZ_ASSERT(aProgress & FLAG_SIZE_AVAILABLE);
  }
  if (aProgress & FLAG_HAS_TRANSPARENCY) {
    MOZ_ASSERT(aProgress & FLAG_SIZE_AVAILABLE);
  }
  if (aProgress & FLAG_LAST_PART_COMPLETE) {
    MOZ_ASSERT(aProgress & FLAG_LOAD_COMPLETE);
  }
  if (aProgress & FLAG_HAS_ERROR) {
    
  }
}

void
ProgressTracker::SetImage(Image* aImage)
{
  MutexAutoLock lock(mImageMutex);
  MOZ_ASSERT(aImage, "Setting null image");
  MOZ_ASSERT(!mImage, "Setting image when we already have one");
  mImage = aImage;
}

void
ProgressTracker::ResetImage()
{
  MutexAutoLock lock(mImageMutex);
  MOZ_ASSERT(mImage, "Resetting image when it's already null!");
  mImage = nullptr;
}

uint32_t
ProgressTracker::GetImageStatus() const
{
  uint32_t status = imgIRequest::STATUS_NONE;

  
  if (mProgress & FLAG_SIZE_AVAILABLE) {
    status |= imgIRequest::STATUS_SIZE_AVAILABLE;
  }
  if (mProgress & FLAG_DECODE_STARTED) {
    status |= imgIRequest::STATUS_DECODE_STARTED;
  }
  if (mProgress & FLAG_DECODE_COMPLETE) {
    status |= imgIRequest::STATUS_DECODE_COMPLETE;
  }
  if (mProgress & FLAG_FRAME_COMPLETE) {
    status |= imgIRequest::STATUS_FRAME_COMPLETE;
  }
  if (mProgress & FLAG_LOAD_COMPLETE) {
    status |= imgIRequest::STATUS_LOAD_COMPLETE;
  }
  if (mProgress & FLAG_IS_ANIMATED) {
    status |= imgIRequest::STATUS_IS_ANIMATED;
  }
  if (mProgress & FLAG_HAS_TRANSPARENCY) {
    status |= imgIRequest::STATUS_HAS_TRANSPARENCY;
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
                        IProgressObserver* aObserver)
      : mTracker(aTracker)
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be created on the main thread");
      MOZ_ASSERT(aTracker, "aTracker should not be null");
      MOZ_ASSERT(aObserver, "aObserver should not be null");
      mObservers.AppendElement(aObserver);
    }

    NS_IMETHOD Run()
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be running on the main thread");
      MOZ_ASSERT(mTracker, "mTracker should not be null");
      for (uint32_t i = 0; i < mObservers.Length(); ++i) {
        mObservers[i]->SetNotificationsDeferred(false);
        mTracker->SyncNotify(mObservers[i]);
      }

      mTracker->mRunnable = nullptr;
      return NS_OK;
    }

    void AddObserver(IProgressObserver* aObserver)
    {
      mObservers.AppendElement(aObserver);
    }

    void RemoveObserver(IProgressObserver* aObserver)
    {
      mObservers.RemoveElement(aObserver);
    }

  private:
    friend class ProgressTracker;

    nsRefPtr<ProgressTracker> mTracker;
    nsTArray<nsRefPtr<IProgressObserver>> mObservers;
};

void
ProgressTracker::Notify(IProgressObserver* aObserver)
{
  MOZ_ASSERT(NS_IsMainThread());

#ifdef PR_LOGGING
  nsRefPtr<Image> image = GetImage();
  if (image && image->GetURI()) {
    nsRefPtr<ImageURL> uri(image->GetURI());
    nsAutoCString spec;
    uri->GetSpec(spec);
    LOG_FUNC_WITH_PARAM(GetImgLog(),
                        "ProgressTracker::Notify async", "uri", spec.get());
  } else {
    LOG_FUNC_WITH_PARAM(GetImgLog(),
                        "ProgressTracker::Notify async", "uri", "<unknown>");
  }
#endif

  aObserver->SetNotificationsDeferred(true);

  
  
  
  AsyncNotifyRunnable* runnable =
    static_cast<AsyncNotifyRunnable*>(mRunnable.get());

  if (runnable) {
    runnable->AddObserver(aObserver);
  } else {
    mRunnable = new AsyncNotifyRunnable(this, aObserver);
    NS_DispatchToCurrentThread(mRunnable);
  }
}



class AsyncNotifyCurrentStateRunnable : public nsRunnable
{
  public:
    AsyncNotifyCurrentStateRunnable(ProgressTracker* aProgressTracker,
                                    IProgressObserver* aObserver)
      : mProgressTracker(aProgressTracker)
      , mObserver(aObserver)
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be created on the main thread");
      MOZ_ASSERT(mProgressTracker, "mProgressTracker should not be null");
      MOZ_ASSERT(mObserver, "mObserver should not be null");
      mImage = mProgressTracker->GetImage();
    }

    NS_IMETHOD Run()
    {
      MOZ_ASSERT(NS_IsMainThread(), "Should be running on the main thread");
      mObserver->SetNotificationsDeferred(false);

      mProgressTracker->SyncNotify(mObserver);
      return NS_OK;
    }

  private:
    nsRefPtr<ProgressTracker> mProgressTracker;
    nsRefPtr<IProgressObserver> mObserver;

    
    
    nsRefPtr<Image> mImage;
};

void
ProgressTracker::NotifyCurrentState(IProgressObserver* aObserver)
{
  MOZ_ASSERT(NS_IsMainThread());

#ifdef PR_LOGGING
  nsRefPtr<Image> image = GetImage();
  nsAutoCString spec;
  if (image && image->GetURI()) {
    image->GetURI()->GetSpec(spec);
  }
  LOG_FUNC_WITH_PARAM(GetImgLog(),
                      "ProgressTracker::NotifyCurrentState", "uri", spec.get());
#endif

  aObserver->SetNotificationsDeferred(true);

  nsCOMPtr<nsIRunnable> ev = new AsyncNotifyCurrentStateRunnable(this,
                                                                 aObserver);
  NS_DispatchToCurrentThread(ev);
}

#define NOTIFY_IMAGE_OBSERVERS(OBSERVERS, FUNC) \
  do { \
    ObserverArray::ForwardIterator iter(OBSERVERS); \
    while (iter.HasMore()) { \
      nsRefPtr<IProgressObserver> observer = iter.GetNext().get(); \
      if (observer && !observer->NotificationsDeferred()) { \
        observer->FUNC; \
      } \
    } \
  } while (false);

 void
ProgressTracker::SyncNotifyInternal(ObserverArray& aObservers,
                                    bool aHasImage,
                                    Progress aProgress,
                                    const nsIntRect& aDirtyRect)
{
  MOZ_ASSERT(NS_IsMainThread());

  typedef imgINotificationObserver I;

  if (aProgress & FLAG_SIZE_AVAILABLE) {
    NOTIFY_IMAGE_OBSERVERS(aObservers, Notify(I::SIZE_AVAILABLE));
  }

  if (aProgress & FLAG_DECODE_STARTED) {
    NOTIFY_IMAGE_OBSERVERS(aObservers, OnStartDecode());
  }

  if (aProgress & FLAG_ONLOAD_BLOCKED) {
    NOTIFY_IMAGE_OBSERVERS(aObservers, BlockOnload());
  }

  if (aHasImage) {
    
    
    
    
    if (!aDirtyRect.IsEmpty()) {
      NOTIFY_IMAGE_OBSERVERS(aObservers, Notify(I::FRAME_UPDATE, &aDirtyRect));
    }

    if (aProgress & FLAG_FRAME_COMPLETE) {
      NOTIFY_IMAGE_OBSERVERS(aObservers, Notify(I::FRAME_COMPLETE));
    }

    if (aProgress & FLAG_HAS_TRANSPARENCY) {
      NOTIFY_IMAGE_OBSERVERS(aObservers, Notify(I::HAS_TRANSPARENCY));
    }

    if (aProgress & FLAG_IS_ANIMATED) {
      NOTIFY_IMAGE_OBSERVERS(aObservers, Notify(I::IS_ANIMATED));
    }
  }

  
  
  
  if (aProgress & FLAG_ONLOAD_UNBLOCKED) {
    NOTIFY_IMAGE_OBSERVERS(aObservers, UnblockOnload());
  }

  if (aProgress & FLAG_DECODE_COMPLETE) {
    MOZ_ASSERT(aHasImage, "Stopped decoding without ever having an image?");
    NOTIFY_IMAGE_OBSERVERS(aObservers, Notify(I::DECODE_COMPLETE));
  }

  if (aProgress & FLAG_LOAD_COMPLETE) {
    NOTIFY_IMAGE_OBSERVERS(aObservers,
                           OnLoadComplete(aProgress & FLAG_LAST_PART_COMPLETE));
  }
}

void
ProgressTracker::SyncNotifyProgress(Progress aProgress,
                                    const nsIntRect& aInvalidRect
                                                  )
{
  MOZ_ASSERT(NS_IsMainThread(), "Use mObservers on main thread only");

  
  Progress progress = Difference(aProgress);
  if (!((mProgress | progress) & FLAG_ONLOAD_BLOCKED)) {
    progress &= ~FLAG_ONLOAD_UNBLOCKED;
  }

  
  
  
  if ((aProgress & FLAG_DECODE_COMPLETE) &&
      (mProgress & FLAG_ONLOAD_BLOCKED) &&
      (mProgress & FLAG_ONLOAD_UNBLOCKED)) {
    progress |= FLAG_ONLOAD_BLOCKED | FLAG_ONLOAD_UNBLOCKED;
  }

  
  mProgress |= progress;

  CheckProgressConsistency(mProgress);

  
  SyncNotifyInternal(mObservers, HasImage(), progress, aInvalidRect);

  if (progress & FLAG_HAS_ERROR) {
    FireFailureNotification();
  }
}

void
ProgressTracker::SyncNotify(IProgressObserver* aObserver)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<Image> image = GetImage();

#ifdef PR_LOGGING
  nsAutoCString spec;
  if (image && image->GetURI()) {
    image->GetURI()->GetSpec(spec);
  }
  LOG_SCOPE_WITH_PARAM(GetImgLog(),
                       "ProgressTracker::SyncNotify", "uri", spec.get());
#endif

  nsIntRect rect;
  if (image) {
    if (NS_FAILED(image->GetWidth(&rect.width)) ||
        NS_FAILED(image->GetHeight(&rect.height))) {
      
      rect = GetMaxSizedIntRect();
    }
  }

  ObserverArray array;
  array.AppendElement(aObserver);
  SyncNotifyInternal(array, !!image, mProgress, rect);
}

void
ProgressTracker::EmulateRequestFinished(IProgressObserver* aObserver)
{
  MOZ_ASSERT(NS_IsMainThread(),
             "SyncNotifyState and mObservers are not threadsafe");
  nsRefPtr<IProgressObserver> kungFuDeathGrip(aObserver);

  if (mProgress & FLAG_ONLOAD_BLOCKED && !(mProgress & FLAG_ONLOAD_UNBLOCKED)) {
    aObserver->UnblockOnload();
  }

  if (!(mProgress & FLAG_LOAD_COMPLETE)) {
    aObserver->OnLoadComplete(true);
  }
}

void
ProgressTracker::AddObserver(IProgressObserver* aObserver)
{
  MOZ_ASSERT(NS_IsMainThread());
  mObservers.AppendElementUnlessExists(aObserver);
}

bool
ProgressTracker::RemoveObserver(IProgressObserver* aObserver)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  bool removed = mObservers.RemoveElement(aObserver);

  
  
  if (removed && !aObserver->NotificationsDeferred()) {
    EmulateRequestFinished(aObserver);
  }

  
  
  AsyncNotifyRunnable* runnable =
    static_cast<AsyncNotifyRunnable*>(mRunnable.get());

  if (aObserver->NotificationsDeferred() && runnable) {
    runnable->RemoveObserver(aObserver);
    aObserver->SetNotificationsDeferred(false);
  }

  return removed;
}

bool
ProgressTracker::FirstObserverIs(IProgressObserver* aObserver)
{
  MOZ_ASSERT(NS_IsMainThread(), "Use mObservers on main thread only");
  ObserverArray::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    nsRefPtr<IProgressObserver> observer = iter.GetNext().get();
    if (observer) {
      return observer.get() == aObserver;
    }
  }
  return false;
}

void
ProgressTracker::OnUnlockedDraw()
{
  MOZ_ASSERT(NS_IsMainThread());
  NOTIFY_IMAGE_OBSERVERS(mObservers,
                         Notify(imgINotificationObserver::UNLOCKED_DRAW));
}

void
ProgressTracker::ResetForNewRequest()
{
  MOZ_ASSERT(NS_IsMainThread());
  mProgress = NoProgress;
  CheckProgressConsistency(mProgress);
}

void
ProgressTracker::OnDiscard()
{
  MOZ_ASSERT(NS_IsMainThread());
  NOTIFY_IMAGE_OBSERVERS(mObservers,
                         Notify(imgINotificationObserver::DISCARD));
}

void
ProgressTracker::OnImageAvailable()
{
  MOZ_ASSERT(NS_IsMainThread());
  
  ObserverArray::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    nsRefPtr<IProgressObserver> observer = iter.GetNext().get();
    if (observer) {
      observer->SetHasImage();
    }
  }
}

void
ProgressTracker::FireFailureNotification()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  nsRefPtr<Image> image = GetImage();
  if (image) {
    
    nsCOMPtr<nsIURI> uri;
    {
      nsRefPtr<ImageURL> threadsafeUriData = image->GetURI();
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
