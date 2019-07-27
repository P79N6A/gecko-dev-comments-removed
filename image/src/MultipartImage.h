




#ifndef mozilla_image_src_MultipartImage_h
#define mozilla_image_src_MultipartImage_h

#include "ImageWrapper.h"
#include "IProgressObserver.h"
#include "ProgressTracker.h"

namespace mozilla {
namespace image {

class NextPartObserver;





class MultipartImage
  : public ImageWrapper
  , public IProgressObserver
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(MultipartImage)
  NS_DECL_ISUPPORTS

  MultipartImage(Image* aImage, ProgressTracker* aTracker);

  void BeginTransitionToPart(Image* aNextPart);

  
  virtual already_AddRefed<imgIContainer> Unwrap() override;
  virtual already_AddRefed<ProgressTracker> GetProgressTracker() override;
  virtual void SetProgressTracker(ProgressTracker* aTracker) override;
  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) override;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aStatus,
                                       bool aLastPart) override;

  
  
  NS_IMETHOD LockImage() override { return NS_OK; }
  NS_IMETHOD UnlockImage() override { return NS_OK; }
  virtual void IncrementAnimationConsumers() override { }
  virtual void DecrementAnimationConsumers() override { }
#ifdef DEBUG
  virtual uint32_t GetAnimationConsumers() override { return 1; }
#endif

  
  virtual void Notify(int32_t aType,
                      const nsIntRect* aRect = nullptr) override;
  virtual void OnLoadComplete(bool aLastPart) override;
  virtual void SetHasImage() override;
  virtual void OnStartDecode() override;
  virtual bool NotificationsDeferred() const override;
  virtual void SetNotificationsDeferred(bool aDeferNotifications) override;

  
  
  virtual void BlockOnload() override { }
  virtual void UnblockOnload() override { }

protected:
  virtual ~MultipartImage();

private:
  friend class NextPartObserver;

  void FinishTransition();

  nsRefPtr<ProgressTracker> mTracker;
  nsAutoPtr<ProgressTrackerInit> mProgressTrackerInit;
  nsRefPtr<NextPartObserver> mNextPartObserver;
  nsRefPtr<Image> mNextPart;
  bool mDeferNotifications : 1;
};

} 
} 

#endif 
