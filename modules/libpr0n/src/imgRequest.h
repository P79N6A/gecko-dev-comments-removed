







































#ifndef imgRequest_h__
#define imgRequest_h__

#include "imgIContainer.h"
#include "imgIDecoder.h"
#include "imgIDecoderObserver.h"

#include "nsIChannelEventSink.h"
#include "nsIContentSniffer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIRequest.h"
#include "nsIProperties.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"

#include "nsCategoryCache.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTObserverArray.h"
#include "nsWeakReference.h"
#include "ImageErrors.h"
#include "imgIRequest.h"

class imgCacheValidator;

class imgRequestProxy;
class imgCacheEntry;

enum {
  stateRequestStarted    = PR_BIT(0),
  stateHasSize           = PR_BIT(1),
  stateDecodeStarted     = PR_BIT(2),
  stateRequestStopped    = PR_BIT(4)
};

class imgRequest : public imgIDecoderObserver,
                   public nsIStreamListener,
                   public nsSupportsWeakReference,
                   public nsIChannelEventSink,
                   public nsIInterfaceRequestor
{
public:
  imgRequest();
  virtual ~imgRequest();

  NS_DECL_ISUPPORTS

  nsresult Init(nsIURI *aURI,
                nsIURI *aKeyURI,
                nsIRequest *aRequest,
                nsIChannel *aChannel,
                imgCacheEntry *aCacheEntry,
                void *aCacheId,
                void *aLoadId);

  
  nsresult AddProxy(imgRequestProxy *proxy);

  
  nsresult RemoveProxy(imgRequestProxy *proxy, nsresult aStatus, PRBool aNotify);
  nsresult NotifyProxyListener(imgRequestProxy *proxy);

  void SniffMimeType(const char *buf, PRUint32 len);

  
  
  
  PRBool IsReusable(void *aCacheId) { return !mLoading || (aCacheId == mCacheId); }

  
  
  
  void CancelAndAbort(nsresult aStatus);

  nsresult GetImage(imgIContainer **aImage);

  
  
  nsresult LockImage();
  nsresult UnlockImage();
  nsresult RequestDecode();
  static nsresult GetResultFromImageStatus(PRUint32 aStatus)
  {
    if (aStatus & imgIRequest::STATUS_ERROR)
      return NS_IMAGELIB_ERROR_FAILURE;
    if (aStatus & imgIRequest::STATUS_LOAD_COMPLETE)
      return NS_IMAGELIB_SUCCESS_LOAD_FINISHED;
    return NS_OK;
  }
private:
  friend class imgCacheEntry;
  friend class imgRequestProxy;
  friend class imgLoader;
  friend class imgCacheValidator;
  friend class imgCacheExpirationTracker;

  inline void SetLoadId(void *aLoadId) {
    mLoadId = aLoadId;
    mLoadTime = PR_Now();
  }
  inline PRUint32 GetImageStatus() const { return mImageStatus; }
  inline PRUint32 GetState() const { return mState; }
  void Cancel(nsresult aStatus);
  nsresult GetURI(nsIURI **aURI);
  nsresult GetKeyURI(nsIURI **aURI);
  nsresult GetPrincipal(nsIPrincipal **aPrincipal);
  nsresult GetSecurityInfo(nsISupports **aSecurityInfo);
  void RemoveFromCache();
  inline const char *GetMimeType() const {
    return mContentType.get();
  }
  inline nsIProperties *Properties() {
    return mProperties;
  }

  
  
  
  void SetCacheEntry(imgCacheEntry *entry);

  
  PRBool HasCacheEntry() const;

  
  
  PRBool HaveProxyWithObserver(imgRequestProxy* aProxyToIgnore) const;

  
  
  PRInt32 Priority() const;

  
  
  void AdjustPriority(imgRequestProxy *aProxy, PRInt32 aDelta);

  
  PRBool HasTransferredData() const { return mGotData; }

  
  
  
  void SetIsInCache(PRBool cacheable);

  
  void UpdateCacheEntrySize();

public:
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR

private:
  nsCOMPtr<nsIRequest> mRequest;
  
  nsCOMPtr<nsIURI> mURI;
  
  nsCOMPtr<nsIURI> mKeyURI;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<imgIContainer> mImage;
  nsCOMPtr<nsIProperties> mProperties;
  nsCOMPtr<nsISupports> mSecurityInfo;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIInterfaceRequestor> mPrevChannelSink;

  nsTObserverArray<imgRequestProxy*> mObservers;

  PRUint32 mImageStatus;
  PRUint32 mState;
  nsCString mContentType;

  nsRefPtr<imgCacheEntry> mCacheEntry; 

  void *mCacheId;

  void *mLoadId;
  PRTime mLoadTime;

  imgCacheValidator *mValidator;
  nsCategoryCache<nsIContentSniffer> mImageSniffers;

  
  
  PRUint32 mDeferredLocks;
  PRPackedBool mDecodeRequested : 1;

  PRPackedBool mIsMultiPartChannel : 1;
  PRPackedBool mLoading : 1;
  PRPackedBool mHadLastPart : 1;
  PRPackedBool mGotData : 1;
  PRPackedBool mIsInCache : 1;
};

#endif
