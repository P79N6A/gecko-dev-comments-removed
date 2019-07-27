





#ifndef imgRequest_h__
#define imgRequest_h__

#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIStreamListener.h"
#include "nsIThreadRetargetableStreamListener.h"
#include "nsIPrincipal.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsProxyRelease.h"
#include "nsStringGlue.h"
#include "nsError.h"
#include "nsIAsyncVerifyRedirectCallback.h"

class imgCacheValidator;
class imgStatusTracker;
class imgLoader;
class imgRequestProxy;
class imgCacheEntry;
class imgMemoryReporter;
class imgRequestNotifyRunnable;
class nsIApplicationCache;
class nsIProperties;
class nsIRequest;
class nsITimedChannel;
class nsIURI;

namespace mozilla {
namespace image {
class Image;
class ImageURL;
} 
} 

class imgRequest MOZ_FINAL : public nsIStreamListener,
                             public nsIThreadRetargetableStreamListener,
                             public nsIChannelEventSink,
                             public nsIInterfaceRequestor,
                             public nsIAsyncVerifyRedirectCallback
{
  virtual ~imgRequest();

public:
  typedef mozilla::image::ImageURL ImageURL;
  explicit imgRequest(imgLoader* aLoader);

  NS_DECL_THREADSAFE_ISUPPORTS

  nsresult Init(nsIURI *aURI,
                nsIURI *aCurrentURI,
                nsIRequest *aRequest,
                nsIChannel *aChannel,
                imgCacheEntry *aCacheEntry,
                void *aLoadId,
                nsIPrincipal* aLoadingPrincipal,
                int32_t aCORSMode);

  void ClearLoader();

  
  void AddProxy(imgRequestProxy *proxy);

  nsresult RemoveProxy(imgRequestProxy *proxy, nsresult aStatus);

  
  
  
  void CancelAndAbort(nsresult aStatus);

  
  void ContinueCancel(nsresult aStatus);

  
  void ContinueEvict();

  
  
  nsresult LockImage();
  nsresult UnlockImage();
  nsresult StartDecoding();
  nsresult RequestDecode();

  inline void SetInnerWindowID(uint64_t aInnerWindowId) {
    mInnerWindowId = aInnerWindowId;
  }

  inline uint64_t InnerWindowID() const {
    return mInnerWindowId;
  }

  
  
  
  
  static void SetCacheValidation(imgCacheEntry* aEntry, nsIRequest* aRequest);

  
  
  
  
  bool CacheChanged(nsIRequest* aNewRequest);

  bool GetMultipart() const { return mIsMultiPartChannel; }

  
  int32_t GetCORSMode() const { return mCORSMode; }

  
  
  already_AddRefed<nsIPrincipal> GetLoadingPrincipal() const
  {
    nsCOMPtr<nsIPrincipal> principal = mLoadingPrincipal;
    return principal.forget();
  }

  
  
  
  already_AddRefed<imgStatusTracker> GetStatusTracker();

  
  inline nsIPrincipal* GetPrincipal() const { return mPrincipal.get(); }

  
  void ResetCacheEntry();

  
  void UpdateCacheEntrySize();

  
  nsresult GetURI(ImageURL **aURI);

  nsresult GetImageErrorCode(void);

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
  void EvictFromCache();
  void RemoveFromCache();

  nsresult GetSecurityInfo(nsISupports **aSecurityInfo);

  inline const char *GetMimeType() const {
    return mContentType.get();
  }
  inline nsIProperties *Properties() {
    return mProperties;
  }

  
  
  
  void SetCacheEntry(imgCacheEntry *entry);

  
  bool HasCacheEntry() const;

  
  
  int32_t Priority() const;

  
  
  void AdjustPriority(imgRequestProxy *aProxy, int32_t aDelta);

  
  bool HasTransferredData() const { return mGotData; }

  
  
  
  void SetIsInCache(bool cacheable);

  bool IsBlockingOnload() const;
  void SetBlockingOnload(bool block) const;

  bool HasConsumers();

public:
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

  
  void SetProperties(nsIChannel* aChan);

private:
  friend class imgMemoryReporter;

  
  imgLoader* mLoader;
  nsCOMPtr<nsIRequest> mRequest;
  
  
  
  nsRefPtr<ImageURL> mURI;
  
  nsCOMPtr<nsIURI> mCurrentURI;
  
  nsCOMPtr<nsIPrincipal> mLoadingPrincipal;
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
  
  nsRefPtr<imgStatusTracker> mStatusTracker;
  nsRefPtr<mozilla::image::Image> mImage;
  nsCOMPtr<nsIProperties> mProperties;
  nsCOMPtr<nsISupports> mSecurityInfo;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIInterfaceRequestor> mPrevChannelSink;
  nsCOMPtr<nsIApplicationCache> mApplicationCache;

  nsCOMPtr<nsITimedChannel> mTimedChannel;

  nsCString mContentType;

  nsRefPtr<imgCacheEntry> mCacheEntry; 

  void *mLoadId;

  imgCacheValidator *mValidator;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;

  
  uint64_t mInnerWindowId;

  
  
  int32_t mCORSMode;

  nsresult mImageErrorCode;

  
  
  bool mDecodeRequested : 1;

  bool mIsMultiPartChannel : 1;
  bool mGotData : 1;
  bool mIsInCache : 1;
  bool mBlockingOnload : 1;
  bool mResniffMimeType : 1;
};

#endif
