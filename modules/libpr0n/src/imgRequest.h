







































#ifndef imgRequest_h__
#define imgRequest_h__

#include "imgContainer.h"
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
#include "imgContainer.h"

class imgCacheValidator;

class imgRequestProxy;
class imgCacheEntry;
class imgMemoryReporter;
class imgRequestNotifyRunnable;

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

  void SniffMimeType(const char *buf, PRUint32 len);

  
  
  
  PRBool IsReusable(void *aCacheId) {
    return (mImage && mImage->GetStatusTracker().IsLoading()) ||
           (aCacheId == mCacheId);
  }

  
  
  
  void CancelAndAbort(nsresult aStatus);

  
  
  nsresult LockImage();
  nsresult UnlockImage();
  nsresult RequestDecode();

private:
  friend class imgCacheEntry;
  friend class imgRequestProxy;
  friend class imgLoader;
  friend class imgCacheValidator;
  friend class imgStatusTracker;
  friend class imgCacheExpirationTracker;
  friend class imgRequestNotifyRunnable;

  inline void SetLoadId(void *aLoadId) {
    mLoadId = aLoadId;
    mLoadTime = PR_Now();
  }
  void Cancel(nsresult aStatus);
  void RemoveFromCache();

  nsresult GetURI(nsIURI **aURI);
  nsresult GetKeyURI(nsIURI **aURI);
  nsresult GetSecurityInfo(nsISupports **aSecurityInfo);

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
  friend class imgMemoryReporter;

  nsCOMPtr<nsIRequest> mRequest;
  
  nsCOMPtr<nsIURI> mURI;
  
  nsCOMPtr<nsIURI> mKeyURI;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsRefPtr<imgContainer> mImage;
  nsCOMPtr<nsIProperties> mProperties;
  nsCOMPtr<nsISupports> mSecurityInfo;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIInterfaceRequestor> mPrevChannelSink;

  nsTObserverArray<imgRequestProxy*> mObservers;

  nsCString mContentType;

  nsRefPtr<imgCacheEntry> mCacheEntry; 

  void *mCacheId;

  void *mLoadId;
  PRTime mLoadTime;

  imgCacheValidator *mValidator;
  nsCategoryCache<nsIContentSniffer> mImageSniffers;

  
  
  PRPackedBool mDecodeRequested : 1;

  PRPackedBool mIsMultiPartChannel : 1;
  PRPackedBool mGotData : 1;
  PRPackedBool mIsInCache : 1;
};

#endif
