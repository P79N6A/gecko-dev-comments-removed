






































#include "imgIRequest.h"
#include "imgIDecoderObserver.h"
#include "nsISecurityInfoProvider.h"

#include "imgIContainer.h"
#include "imgIDecoder.h"
#include "nsIRequestObserver.h"
#include "nsIChannel.h"
#include "nsILoadGroup.h"
#include "nsISupportsPriority.h"
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

class imgRequestProxy : public imgIRequest, public nsISupportsPriority, public nsISecurityInfoProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIREQUEST
  NS_DECL_NSIREQUEST
  NS_DECL_NSISUPPORTSPRIORITY
  NS_DECL_NSISECURITYINFOPROVIDER

  imgRequestProxy();
  virtual ~imgRequestProxy();

  
  
  
  nsresult Init(imgRequest *request, nsILoadGroup *aLoadGroup, imgIDecoderObserver *aObserver);
  nsresult ChangeOwner(imgRequest *aNewOwner); 
                                               

  void AddToLoadGroup();
  void RemoveFromLoadGroup(PRBool releaseLoadGroup);

protected:
  friend class imgRequest;

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

  
  void FrameChanged(imgIContainer *aContainer, nsIntRect * aDirtyRect);

  
  void OnStartRequest(nsIRequest *request, nsISupports *ctxt);
  void OnStopRequest(nsIRequest *request, nsISupports *ctxt, nsresult statusCode, PRBool aLastPart); 

  inline PRBool HasObserver() const {
    return mListener != nsnull;
  }

  
  void DoCancel(nsresult status);

  
  void NullOutListener();
  
private:
  friend class imgCacheValidator;

  
  
  
  
  
  
  nsRefPtr<imgRequest> mOwner;

  
  
  
  imgIDecoderObserver* mListener;
  nsCOMPtr<nsILoadGroup> mLoadGroup;

  nsLoadFlags mLoadFlags;
  PRUint32    mLocksHeld;
  PRPackedBool mCanceled;
  PRPackedBool mIsInLoadGroup;
  PRPackedBool mListenerIsStrongRef;
  PRPackedBool mDecodeRequested;
};
