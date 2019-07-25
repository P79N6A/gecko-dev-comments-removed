






































#ifndef imgRequestProxy_h__
#define imgRequestProxy_h__

#include "imgIRequest.h"
#include "imgIDecoderObserver.h"
#include "nsISecurityInfoProvider.h"

#include "nsIRequestObserver.h"
#include "nsIChannel.h"
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

class imgRequestNotifyRunnable;
class imgStatusNotifyRunnable;

namespace mozilla {
namespace imagelib {
class Image;
} 
} 

class imgRequestProxy : public imgIRequest, 
                        public nsISupportsPriority, 
                        public nsISecurityInfoProvider,
                        public nsITimedChannel
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIREQUEST
  NS_DECL_NSIREQUEST
  NS_DECL_NSISUPPORTSPRIORITY
  NS_DECL_NSISECURITYINFOPROVIDER
  

  imgRequestProxy();
  virtual ~imgRequestProxy();

  
  
  nsresult Init(imgRequest *request, nsILoadGroup *aLoadGroup,
                mozilla::imagelib::Image* aImage,
                nsIURI* aURI, imgIDecoderObserver *aObserver);

  nsresult ChangeOwner(imgRequest *aNewOwner); 
                                               

  void AddToLoadGroup();
  void RemoveFromLoadGroup(PRBool releaseLoadGroup);

  inline PRBool HasObserver() const {
    return mListener != nsnull;
  }

  void SetPrincipal(nsIPrincipal *aPrincipal);

  
  
  
  
  void NotifyListener();

  
  
  
  void SyncNotifyListener();

  
  
  PRBool NotificationsDeferred() const
  {
    return mDeferNotifications;
  }
  void SetNotificationsDeferred(PRBool aDeferNotifications)
  {
    mDeferNotifications = aDeferNotifications;
  }

  
  
  void SetImage(mozilla::imagelib::Image* aImage);

  
  
  
  
  void ClearAnimationConsumers();

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

  
  
  

  
  void OnStartDecode   ();
  void OnStartContainer(imgIContainer *aContainer);
  void OnStartFrame    (PRUint32 aFrame);
  void OnDataAvailable (PRBool aCurrentFrame, const nsIntRect * aRect);
  void OnStopFrame     (PRUint32 aFrame);
  void OnStopContainer (imgIContainer *aContainer);
  void OnStopDecode    (nsresult status, const PRUnichar *statusArg); 
  void OnDiscard       ();

  
  void FrameChanged(imgIContainer *aContainer,
                    const nsIntRect *aDirtyRect);

  
  void OnStartRequest();
  void OnStopRequest(PRBool aLastPart);

  
  void DoCancel(nsresult status);

  
  void NullOutListener();

  void DoRemoveFromLoadGroup() {
    RemoveFromLoadGroup(PR_TRUE);
  }

  
  
  
  
  imgStatusTracker& GetStatusTracker();

  nsITimedChannel* TimedChannel()
  {
    if (!mOwner)
      return nsnull;
    return mOwner->mTimedChannel;
  }

public:
  NS_FORWARD_SAFE_NSITIMEDCHANNEL(TimedChannel())

private:
  friend class imgCacheValidator;

  
  
  
  
  
  
  nsRefPtr<imgRequest> mOwner;

  
  nsCOMPtr<nsIURI> mURI;

  
  
  nsRefPtr<mozilla::imagelib::Image> mImage;

  
  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  
  
  imgIDecoderObserver* mListener;
  nsCOMPtr<nsILoadGroup> mLoadGroup;

  nsLoadFlags mLoadFlags;
  PRUint32    mLockCount;
  PRUint32    mAnimationConsumers;
  PRPackedBool mCanceled;
  PRPackedBool mIsInLoadGroup;
  PRPackedBool mListenerIsStrongRef;
  PRPackedBool mDecodeRequested;

  
  
  PRPackedBool mDeferNotifications;

  
  
  PRPackedBool mSentStartContainer;
};

#endif 
