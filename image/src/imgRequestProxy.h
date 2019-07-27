





#ifndef mozilla_image_src_imgRequestProxy_h
#define mozilla_image_src_imgRequestProxy_h

#include "imgIRequest.h"
#include "nsISecurityInfoProvider.h"

#include "nsILoadGroup.h"
#include "nsISupportsPriority.h"
#include "nsITimedChannel.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "mozilla/TimeStamp.h"

#include "imgRequest.h"
#include "IProgressObserver.h"

#define NS_IMGREQUESTPROXY_CID \
{ /* 20557898-1dd2-11b2-8f65-9c462ee2bc95 */         \
     0x20557898,                                     \
     0x1dd2,                                         \
     0x11b2,                                         \
    {0x8f, 0x65, 0x9c, 0x46, 0x2e, 0xe2, 0xbc, 0x95} \
}

class imgINotificationObserver;
class imgStatusNotifyRunnable;
struct nsIntRect;
class ProxyBehaviour;

namespace mozilla {
namespace image {
class Image;
class ImageURL;
class ProgressTracker;
} 
} 

class imgRequestProxy : public imgIRequest,
                        public mozilla::image::IProgressObserver,
                        public nsISupportsPriority,
                        public nsISecurityInfoProvider,
                        public nsITimedChannel
{
protected:
  virtual ~imgRequestProxy();

public:
  typedef mozilla::image::Image Image;
  typedef mozilla::image::ImageURL ImageURL;
  typedef mozilla::image::ProgressTracker ProgressTracker;

  MOZ_DECLARE_REFCOUNTED_TYPENAME(imgRequestProxy)
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIREQUEST
  NS_DECL_NSIREQUEST
  NS_DECL_NSISUPPORTSPRIORITY
  NS_DECL_NSISECURITYINFOPROVIDER
  

  imgRequestProxy();

  
  
  nsresult Init(imgRequest* aOwner,
                nsILoadGroup* aLoadGroup,
                ImageURL* aURI,
                imgINotificationObserver* aObserver);

  nsresult ChangeOwner(imgRequest* aNewOwner); 
                                               
                                               
                                               

  void AddToLoadGroup();
  void RemoveFromLoadGroup(bool releaseLoadGroup);

  inline bool HasObserver() const {
    return mListener != nullptr;
  }

  
  
  
  
  void NotifyListener();

  
  
  
  void SyncNotifyListener();

  
  virtual void Notify(int32_t aType,
                      const nsIntRect* aRect = nullptr) override;
  virtual void OnLoadComplete(bool aLastPart) override;

  
  virtual void BlockOnload() override;
  virtual void UnblockOnload() override;

  
  virtual void SetHasImage() override;
  virtual void OnStartDecode() override;

  
  
  virtual bool NotificationsDeferred() const override
  {
    return mDeferNotifications;
  }
  virtual void SetNotificationsDeferred(bool aDeferNotifications) override
  {
    mDeferNotifications = aDeferNotifications;
  }

  
  
  
  
  void ClearAnimationConsumers();

  virtual nsresult Clone(imgINotificationObserver* aObserver,
                         imgRequestProxy** aClone);
  nsresult GetStaticRequest(imgRequestProxy** aReturn);

  nsresult GetURI(ImageURL** aURI);

protected:
  friend class mozilla::image::ProgressTracker;
  friend class imgStatusNotifyRunnable;

  class imgCancelRunnable;
  friend class imgCancelRunnable;

  class imgCancelRunnable : public nsRunnable
  {
    public:
      imgCancelRunnable(imgRequestProxy* owner, nsresult status)
        : mOwner(owner), mStatus(status)
      { }

      NS_IMETHOD Run() override {
        mOwner->DoCancel(mStatus);
        return NS_OK;
      }

    private:
      nsRefPtr<imgRequestProxy> mOwner;
      nsresult mStatus;
  };

  
  void DoCancel(nsresult status);

  
  void NullOutListener();

  void DoRemoveFromLoadGroup() {
    RemoveFromLoadGroup(true);
  }

  
  
  
  
  already_AddRefed<ProgressTracker> GetProgressTracker() const;

  nsITimedChannel* TimedChannel()
  {
    if (!GetOwner()) {
      return nullptr;
    }
    return GetOwner()->GetTimedChannel();
  }

  already_AddRefed<Image> GetImage() const;
  bool HasImage() const;
  imgRequest* GetOwner() const;

  nsresult PerformClone(imgINotificationObserver* aObserver,
                        imgRequestProxy* (aAllocFn)(imgRequestProxy*),
                        imgRequestProxy** aClone);

public:
  NS_FORWARD_SAFE_NSITIMEDCHANNEL(TimedChannel())

protected:
  nsAutoPtr<ProxyBehaviour> mBehaviour;

private:
  friend class imgCacheValidator;
  friend imgRequestProxy* NewStaticProxy(imgRequestProxy* aThis);

  
  nsRefPtr<ImageURL> mURI;

  
  
  
  imgINotificationObserver* mListener;
  nsCOMPtr<nsILoadGroup> mLoadGroup;

  nsLoadFlags mLoadFlags;
  uint32_t    mLockCount;
  uint32_t    mAnimationConsumers;
  bool mCanceled;
  bool mIsInLoadGroup;
  bool mListenerIsStrongRef;
  bool mDecodeRequested;

  
  
  bool mDeferNotifications;
};



class imgRequestProxyStatic : public imgRequestProxy
{

public:
  imgRequestProxyStatic(Image* aImage, nsIPrincipal* aPrincipal);

  NS_IMETHOD GetImagePrincipal(nsIPrincipal** aPrincipal) override;

  using imgRequestProxy::Clone;

  virtual nsresult Clone(imgINotificationObserver* aObserver,
                         imgRequestProxy** aClone) override;

protected:
  friend imgRequestProxy* NewStaticProxy(imgRequestProxy*);

  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

#endif 
