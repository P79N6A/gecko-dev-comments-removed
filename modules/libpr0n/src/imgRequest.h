







































#ifndef imgRequest_h__
#define imgRequest_h__

#include "imgIDecoderObserver.h"

#include "nsIChannelEventSink.h"
#include "nsIContentSniffer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIRequest.h"
#include "nsIProperties.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsITimedChannel.h"

#include "nsCategoryCache.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsTObserverArray.h"
#include "nsWeakReference.h"
#include "ImageErrors.h"
#include "imgIRequest.h"
#include "imgStatusTracker.h"
#include "nsIAsyncVerifyRedirectCallback.h"

class imgCacheValidator;

class imgRequestProxy;
class imgCacheEntry;
class imgMemoryReporter;
class imgRequestNotifyRunnable;

namespace mozilla {
namespace imagelib {
class Image;
} 
} 

class imgRequest : public imgIDecoderObserver,
                   public nsIStreamListener,
                   public nsSupportsWeakReference,
                   public nsIChannelEventSink,
                   public nsIInterfaceRequestor,
                   public nsIAsyncVerifyRedirectCallback
{
public:
  imgRequest();
  virtual ~imgRequest();

  NS_DECL_ISUPPORTS

  nsresult Init(nsIURI *aURI,
                nsIURI *aCurrentURI,
                nsIRequest *aRequest,
                nsIChannel *aChannel,
                imgCacheEntry *aCacheEntry,
                void *aLoadId,
                nsIPrincipal* aLoadingPrincipal,
                PRInt32 aCORSMode);

  
  nsresult AddProxy(imgRequestProxy *proxy);

  
  nsresult RemoveProxy(imgRequestProxy *proxy, nsresult aStatus, bool aNotify);

  void SniffMimeType(const char *buf, PRUint32 len);

  
  
  
  void CancelAndAbort(nsresult aStatus);

  
  
  nsresult LockImage();
  nsresult UnlockImage();
  nsresult RequestDecode();

  inline void SetInnerWindowID(PRUint64 aInnerWindowId) {
    mInnerWindowId = aInnerWindowId;
  }

  inline PRUint64 InnerWindowID() const {
    return mInnerWindowId;
  }

  
  
  
  
  static void SetCacheValidation(imgCacheEntry* aEntry, nsIRequest* aRequest);

  
  PRInt32 GetCORSMode() const { return mCORSMode; }

  
  
  already_AddRefed<nsIPrincipal> GetLoadingPrincipal() const
  {
    nsCOMPtr<nsIPrincipal> principal = mLoadingPrincipal;
    return principal.forget();
  }

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
  }
  void Cancel(nsresult aStatus);
  void RemoveFromCache();

  nsresult GetURI(nsIURI **aURI);
  nsresult GetSecurityInfo(nsISupports **aSecurityInfo);

  inline const char *GetMimeType() const {
    return mContentType.get();
  }
  inline nsIProperties *Properties() {
    return mProperties;
  }

  
  
  
  imgStatusTracker& GetStatusTracker();
    
  
  
  
  void SetCacheEntry(imgCacheEntry *entry);

  
  bool HasCacheEntry() const;

  
  
  bool HaveProxyWithObserver(imgRequestProxy* aProxyToIgnore) const;

  
  
  PRInt32 Priority() const;

  
  
  void AdjustPriority(imgRequestProxy *aProxy, PRInt32 aDelta);

  
  bool HasTransferredData() const { return mGotData; }

  
  
  
  void SetIsInCache(bool cacheable);

  
  void UpdateCacheEntrySize();

public:
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGICONTAINEROBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

private:
  friend class imgMemoryReporter;

  nsCOMPtr<nsIRequest> mRequest;
  
  
  nsCOMPtr<nsIURI> mURI;
  
  nsCOMPtr<nsIURI> mCurrentURI;
  
  nsCOMPtr<nsIPrincipal> mLoadingPrincipal;
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
  
  nsAutoPtr<imgStatusTracker> mStatusTracker;
  nsRefPtr<mozilla::imagelib::Image> mImage;
  nsCOMPtr<nsIProperties> mProperties;
  nsCOMPtr<nsISupports> mSecurityInfo;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIInterfaceRequestor> mPrevChannelSink;

  nsTObserverArray<imgRequestProxy*> mObservers;

  nsCOMPtr<nsITimedChannel> mTimedChannel;

  nsCString mContentType;

  nsRefPtr<imgCacheEntry> mCacheEntry; 

  void *mLoadId;

  imgCacheValidator *mValidator;
  nsCategoryCache<nsIContentSniffer> mImageSniffers;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;

  
  PRUint64 mInnerWindowId;

  
  
  PRInt32 mCORSMode;

  
  
  bool mDecodeRequested : 1;

  bool mIsMultiPartChannel : 1;
  bool mGotData : 1;
  bool mIsInCache : 1;
};

#endif
