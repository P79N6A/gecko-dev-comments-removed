




#include "MultipartImage.h"

#include "imgINotificationObserver.h"

namespace mozilla {
namespace image {





class NextPartObserver : public IProgressObserver
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(NextPartObserver)
  NS_INLINE_DECL_REFCOUNTING(NextPartObserver, override)

  explicit NextPartObserver(MultipartImage* aOwner)
    : mOwner(aOwner)
  {
    MOZ_ASSERT(mOwner);
  }

  void BeginObserving(Image* aImage)
  {
    MOZ_ASSERT(aImage);
    mImage = aImage;

    nsRefPtr<ProgressTracker> tracker = mImage->GetProgressTracker();
    tracker->AddObserver(this);
  }

  void BlockUntilDecodedAndFinishObserving()
  {
    
    mImage->GetFrame(imgIContainer::FRAME_CURRENT,
                     imgIContainer::FLAG_SYNC_DECODE);

    FinishObserving();
  }

  virtual void Notify(int32_t aType,
                      const nsIntRect* aRect = nullptr) override
  {
    if (!mImage) {
      
      return;
    }

    if (aType == imgINotificationObserver::FRAME_COMPLETE) {
      FinishObserving();
    }
  }

  virtual void OnLoadComplete(bool aLastPart) override
  {
    if (!mImage) {
      
      return;
    }

    
    
    nsRefPtr<ProgressTracker> tracker = mImage->GetProgressTracker();
    if (tracker->GetProgress() & FLAG_HAS_ERROR) {
      FinishObserving();
    }
  }

  
  virtual void BlockOnload() override { }
  virtual void UnblockOnload() override { }
  virtual void SetHasImage() override { }
  virtual void OnStartDecode() override { }
  virtual bool NotificationsDeferred() const override { return false; }
  virtual void SetNotificationsDeferred(bool) override { }

private:
  virtual ~NextPartObserver() { }

  void FinishObserving()
  {
    MOZ_ASSERT(mImage);

    nsRefPtr<ProgressTracker> tracker = mImage->GetProgressTracker();
    tracker->RemoveObserver(this);
    mImage = nullptr;

    mOwner->FinishTransition();
  }

  MultipartImage* mOwner;
  nsRefPtr<Image> mImage;
};






MultipartImage::MultipartImage(Image* aImage, ProgressTracker* aTracker)
  : ImageWrapper(aImage)
  , mDeferNotifications(false)
{
  MOZ_ASSERT(aTracker);
  mProgressTrackerInit = new ProgressTrackerInit(this, aTracker);
  mNextPartObserver = new NextPartObserver(this);

  
  nsRefPtr<ProgressTracker> firstPartTracker =
    InnerImage()->GetProgressTracker();
  firstPartTracker->AddObserver(this);
  InnerImage()->RequestDecode();
  InnerImage()->IncrementAnimationConsumers();
}

MultipartImage::~MultipartImage() { }

NS_IMPL_QUERY_INTERFACE_INHERITED0(MultipartImage, ImageWrapper)
NS_IMPL_ADDREF(MultipartImage)
NS_IMPL_RELEASE(MultipartImage)

void
MultipartImage::BeginTransitionToPart(Image* aNextPart)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aNextPart);

  if (mNextPart) {
    
    mNextPartObserver->BlockUntilDecodedAndFinishObserving();
    MOZ_ASSERT(!mNextPart);
  }

  mNextPart = aNextPart;

  
  
  mNextPartObserver->BeginObserving(mNextPart);
  mNextPart->RequestDecode();
  mNextPart->IncrementAnimationConsumers();
}

void MultipartImage::FinishTransition()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mNextPart, "Should have a next part here");

  nsRefPtr<ProgressTracker> newCurrentPartTracker =
    mNextPart->GetProgressTracker();
  if (newCurrentPartTracker->GetProgress() & FLAG_HAS_ERROR) {
    
    mNextPart = nullptr;

    
    mTracker->ResetForNewRequest();
    nsRefPtr<ProgressTracker> currentPartTracker =
      InnerImage()->GetProgressTracker();
    mTracker->SyncNotifyProgress(currentPartTracker->GetProgress());

    return;
  }

  
  {
    nsRefPtr<ProgressTracker> currentPartTracker =
      InnerImage()->GetProgressTracker();
    currentPartTracker->RemoveObserver(this);
  }

  
  mTracker->ResetForNewRequest();
  SetInnerImage(mNextPart);
  mNextPart = nullptr;
  newCurrentPartTracker->AddObserver(this);

  
  
  mTracker->SyncNotifyProgress(newCurrentPartTracker->GetProgress(),
                               nsIntRect::GetMaxSizedIntRect());
}

already_AddRefed<imgIContainer>
MultipartImage::Unwrap()
{
  
  
  nsCOMPtr<imgIContainer> image = this;
  return image.forget();
}

already_AddRefed<ProgressTracker>
MultipartImage::GetProgressTracker()
{
  MOZ_ASSERT(mTracker);
  nsRefPtr<ProgressTracker> tracker = mTracker;
  return tracker.forget();
}

void
MultipartImage::SetProgressTracker(ProgressTracker* aTracker)
{
  MOZ_ASSERT(aTracker);
  MOZ_ASSERT(!mTracker);
  mTracker = aTracker;
}

nsresult
MultipartImage::OnImageDataAvailable(nsIRequest* aRequest,
                                     nsISupports* aContext,
                                     nsIInputStream* aInStr,
                                     uint64_t aSourceOffset,
                                     uint32_t aCount)
{
  
  

  
  nsRefPtr<Image> nextPart = mNextPart;
  if (nextPart) {
    nextPart->OnImageDataAvailable(aRequest, aContext, aInStr,
                                   aSourceOffset, aCount);
  } else {
    InnerImage()->OnImageDataAvailable(aRequest, aContext, aInStr,
                                       aSourceOffset, aCount);
  }

  return NS_OK;
}

nsresult
MultipartImage::OnImageDataComplete(nsIRequest* aRequest,
                                    nsISupports* aContext,
                                    nsresult aStatus,
                                    bool aLastPart)
{
  
  

  
  nsRefPtr<Image> nextPart = mNextPart;
  if (nextPart) {
    nextPart->OnImageDataComplete(aRequest, aContext, aStatus, aLastPart);
  } else {
    InnerImage()->OnImageDataComplete(aRequest, aContext, aStatus, aLastPart);
  }

  return NS_OK;
}

void
MultipartImage::Notify(int32_t aType, const nsIntRect* aRect )
{
  if (aType == imgINotificationObserver::SIZE_AVAILABLE) {
    mTracker->SyncNotifyProgress(FLAG_SIZE_AVAILABLE);
  } else if (aType == imgINotificationObserver::FRAME_UPDATE) {
    mTracker->SyncNotifyProgress(NoProgress, *aRect);
  } else if (aType == imgINotificationObserver::FRAME_COMPLETE) {
    mTracker->SyncNotifyProgress(FLAG_FRAME_COMPLETE);
  } else if (aType == imgINotificationObserver::LOAD_COMPLETE) {
    mTracker->SyncNotifyProgress(FLAG_LOAD_COMPLETE);
  } else if (aType == imgINotificationObserver::DECODE_COMPLETE) {
    mTracker->SyncNotifyProgress(FLAG_DECODE_COMPLETE);
  } else if (aType == imgINotificationObserver::DISCARD) {
    mTracker->OnDiscard();
  } else if (aType == imgINotificationObserver::UNLOCKED_DRAW) {
    mTracker->OnUnlockedDraw();
  } else if (aType == imgINotificationObserver::IS_ANIMATED) {
    mTracker->SyncNotifyProgress(FLAG_IS_ANIMATED);
  } else if (aType == imgINotificationObserver::HAS_TRANSPARENCY) {
    mTracker->SyncNotifyProgress(FLAG_HAS_TRANSPARENCY);
  } else {
    NS_NOTREACHED("Notification list should be exhaustive");
  }
}

void
MultipartImage::OnLoadComplete(bool aLastPart)
{
  Progress progress = FLAG_LOAD_COMPLETE;
  if (aLastPart) {
    progress |= FLAG_LAST_PART_COMPLETE;
  }
  mTracker->SyncNotifyProgress(progress);
}

void
MultipartImage::SetHasImage()
{
  mTracker->OnImageAvailable();
}

void
MultipartImage::OnStartDecode()
{
  mTracker->SyncNotifyProgress(FLAG_DECODE_STARTED);
}

bool
MultipartImage::NotificationsDeferred() const
{
  return mDeferNotifications;
}

void
MultipartImage::SetNotificationsDeferred(bool aDeferNotifications)
{
  mDeferNotifications = aDeferNotifications;
}

} 
} 
