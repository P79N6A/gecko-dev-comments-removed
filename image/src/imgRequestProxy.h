





#ifndef imgRequestProxy_h__
#define imgRequestProxy_h__

#include "imgIRequest.h"
#include "nsISecurityInfoProvider.h"

#include "nsILoadGroup.h"
#include "nsISupportsPriority.h"
#include "nsITimedChannel.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"

#include "imgRequest.h"

#define NS_IMGREQUESTPROXY_CID \
{ /* 20557898-1dd2-11b2-8f65-9c462ee2bc95 */         \
     0x20557898,                                     \
     0x1dd2,                                         \
     0x11b2,                                         \
    {0x8f, 0x65, 0x9c, 0x46, 0x2e, 0xe2, 0xbc, 0x95} \
}

class imgINotificationObserver;
class imgRequestNotifyRunnable;
class imgStatusNotifyRunnable;
class nsIntRect;
class ProxyBehaviour;

namespace mozilla {
namespace image {
class Image;
class ImageURL;
} 
} 

class imgRequestProxy : public imgIRequest,
                        public nsISupportsPriority,
                        public nsISecurityInfoProvider,
                        public nsITimedChannel
{
public:
  typedef mozilla::image::ImageURL ImageURL;
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIREQUEST
  NS_DECL_NSIREQUEST
  NS_DECL_NSISUPPORTSPRIORITY
  NS_DECL_NSISECURITYINFOPROVIDER
  

  imgRequestProxy();
  virtual ~imgRequestProxy();

  
  
  nsresult Init(imgRequest* aOwner,
                nsILoadGroup *aLoadGroup,
                ImageURL* aURI,
                imgINotificationObserver *aObserver);

  nsresult ChangeOwner(imgRequest *aNewOwner); 
                                               

  void AddToLoadGroup();
  void RemoveFromLoadGroup(bool releaseLoadGroup);

  inline bool HasObserver() const {
    return mListener != nullptr;
  }

  
  
  
  
  void NotifyListener();

  
  
  
  void SyncNotifyListener();

  
  
  bool NotificationsDeferred() const
  {
    return mDeferNotifications;
  }
  void SetNotificationsDeferred(bool aDeferNotifications)
  {
    mDeferNotifications = aDeferNotifications;
  }

  
  void SetHasImage();

  
  
  
  
  void ClearAnimationConsumers();

  virtual nsresult Clone(imgINotificationObserver* aObserver, imgRequestProxy** aClone);
  nsresult GetStaticRequest(imgRequestProxy** aReturn);

  nsresult GetURI(ImageURL **aURI);

protected:
  friend class imgStatusTracker;
  friend class imgStatusNotifyRunnable;
  friend class imgRequestNotifyRunnable;

  class imgCancelRunnable;
  friend class imgCancelRunnable;

  class imgCancelRunnable : public nsRunnable
  {
    public:
      imgCancelRunnable(imgRequestProxy* owner, nsresult status)
        : mOwner(owner), mStatus(status)
      {}

      NS_IMETHOD Run() {
        mOwner->DoCancel(mStatus);
        return NS_OK;
      }

    private:
      nsRefPtr<imgRequestProxy> mOwner;
      nsresult mStatus;
  };

  
  
  

  
  void OnStartDecode     ();
  void OnStartContainer  ();
  void OnFrameUpdate     (const nsIntRect * aRect);
  void OnStopFrame       ();
  void OnStopDecode      ();
  void OnDiscard         ();
  void OnUnlockedDraw    ();
  void OnImageIsAnimated ();

  
  void OnStartRequest();
  void OnStopRequest(bool aLastPart);

  
  void BlockOnload();
  void UnblockOnload();

  
  void DoCancel(nsresult status);

  
  void NullOutListener();

  void DoRemoveFromLoadGroup() {
    RemoveFromLoadGroup(true);
  }

  
  
  
  
  already_AddRefed<imgStatusTracker> GetStatusTracker() const;

  nsITimedChannel* TimedChannel()
  {
    if (!GetOwner())
      return nullptr;
    return GetOwner()->mTimedChannel;
  }

  already_AddRefed<mozilla::image::Image> GetImage() const;
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

  
  
  bool mSentStartContainer;
};



class imgRequestProxyStatic : public imgRequestProxy
{

public:
  imgRequestProxyStatic(mozilla::image::Image* aImage,
                        nsIPrincipal* aPrincipal);

  NS_IMETHOD GetImagePrincipal(nsIPrincipal** aPrincipal) MOZ_OVERRIDE;

  using imgRequestProxy::Clone;

  virtual nsresult Clone(imgINotificationObserver* aObserver,
                         imgRequestProxy** aClone) MOZ_OVERRIDE;

protected:
  friend imgRequestProxy* NewStaticProxy(imgRequestProxy*);

  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

#endif 
