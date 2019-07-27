




#ifndef MOZILLA_IMAGELIB_MULTIPARTIMAGE_H_
#define MOZILLA_IMAGELIB_MULTIPARTIMAGE_H_

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

  
  virtual already_AddRefed<imgIContainer> Unwrap() MOZ_OVERRIDE;
  virtual already_AddRefed<ProgressTracker> GetProgressTracker() MOZ_OVERRIDE;
  virtual void SetProgressTracker(ProgressTracker* aTracker) MOZ_OVERRIDE;
  virtual nsresult OnImageDataAvailable(nsIRequest* aRequest,
                                        nsISupports* aContext,
                                        nsIInputStream* aInStr,
                                        uint64_t aSourceOffset,
                                        uint32_t aCount) MOZ_OVERRIDE;
  virtual nsresult OnImageDataComplete(nsIRequest* aRequest,
                                       nsISupports* aContext,
                                       nsresult aStatus,
                                       bool aLastPart) MOZ_OVERRIDE;

  
  
  NS_IMETHOD LockImage() MOZ_OVERRIDE { return NS_OK; }
  NS_IMETHOD UnlockImage() MOZ_OVERRIDE { return NS_OK; }
  virtual void IncrementAnimationConsumers() MOZ_OVERRIDE { }
  virtual void DecrementAnimationConsumers() MOZ_OVERRIDE { }
#ifdef DEBUG
  virtual uint32_t GetAnimationConsumers() MOZ_OVERRIDE { return 1; }
#endif

  
  virtual void Notify(int32_t aType,
                      const nsIntRect* aRect = nullptr) MOZ_OVERRIDE;
  virtual void OnLoadComplete(bool aLastPart) MOZ_OVERRIDE;
  virtual void SetHasImage() MOZ_OVERRIDE;
  virtual void OnStartDecode() MOZ_OVERRIDE;
  virtual bool NotificationsDeferred() const MOZ_OVERRIDE;
  virtual void SetNotificationsDeferred(bool aDeferNotifications) MOZ_OVERRIDE;

  
  
  virtual void BlockOnload() MOZ_OVERRIDE { }
  virtual void UnblockOnload() MOZ_OVERRIDE { }

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
