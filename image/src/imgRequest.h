





#ifndef mozilla_image_src_imgRequest_h
#define mozilla_image_src_imgRequest_h

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
#include "mozilla/Mutex.h"
#include "mozilla/net/ReferrerPolicy.h"

class imgCacheValidator;
class imgLoader;
class imgRequestProxy;
class imgCacheEntry;
class nsIApplicationCache;
class nsIProperties;
class nsIRequest;
class nsITimedChannel;
class nsIURI;

namespace mozilla {
namespace image {
class Image;
class ImageURL;
class ProgressTracker;
} 
} 

struct NewPartResult;

class imgRequest final : public nsIStreamListener,
                         public nsIThreadRetargetableStreamListener,
                         public nsIChannelEventSink,
                         public nsIInterfaceRequestor,
                         public nsIAsyncVerifyRedirectCallback
{
  typedef mozilla::image::Image Image;
  typedef mozilla::image::ImageURL ImageURL;
  typedef mozilla::image::ProgressTracker ProgressTracker;
  typedef mozilla::net::ReferrerPolicy ReferrerPolicy;

public:
  explicit imgRequest(imgLoader* aLoader);

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK

  nsresult Init(nsIURI* aURI,
                nsIURI* aCurrentURI,
                bool aHadInsecureRedirect,
                nsIRequest* aRequest,
                nsIChannel* aChannel,
                imgCacheEntry* aCacheEntry,
                nsISupports* aCX,
                nsIPrincipal* aLoadingPrincipal,
                int32_t aCORSMode,
                ReferrerPolicy aReferrerPolicy);

  void ClearLoader();

  
  void AddProxy(imgRequestProxy* proxy);

  nsresult RemoveProxy(imgRequestProxy* proxy, nsresult aStatus);

  
  
  
  void CancelAndAbort(nsresult aStatus);

  
  void ContinueCancel(nsresult aStatus);

  
  void ContinueEvict();

  
  void RequestDecode();

  inline uint64_t InnerWindowID() const {
    return mInnerWindowId;
  }

  
  
  
  
  static void SetCacheValidation(imgCacheEntry* aEntry, nsIRequest* aRequest);

  
  
  
  
  bool CacheChanged(nsIRequest* aNewRequest);

  bool GetMultipart() const;

  
  
  bool HadInsecureRedirect() const;

  
  int32_t GetCORSMode() const { return mCORSMode; }

  
  ReferrerPolicy GetReferrerPolicy() const { return mReferrerPolicy; }

  
  
  already_AddRefed<nsIPrincipal> GetLoadingPrincipal() const
  {
    nsCOMPtr<nsIPrincipal> principal = mLoadingPrincipal;
    return principal.forget();
  }

  
  
  
  already_AddRefed<ProgressTracker> GetProgressTracker() const;

  
  already_AddRefed<Image> GetImage() const;

  
  inline nsIPrincipal* GetPrincipal() const { return mPrincipal.get(); }

  
  void ResetCacheEntry();

  
  nsresult GetURI(ImageURL** aURI);
  nsresult GetCurrentURI(nsIURI** aURI);

  nsresult GetImageErrorCode(void);

  
  bool HasTransferredData() const;

  
  const char* GetMimeType() const { return mContentType.get(); }

  
  
  int32_t Priority() const;

  
  
  void AdjustPriority(imgRequestProxy* aProxy, int32_t aDelta);

  
  nsIRequest* GetRequest() const { return mRequest; }

  nsITimedChannel* GetTimedChannel() const { return mTimedChannel; }

  nsresult GetSecurityInfo(nsISupports** aSecurityInfoOut);

  imgCacheValidator* GetValidator() const { return mValidator; }
  void SetValidator(imgCacheValidator* aValidator) { mValidator = aValidator; }

  void* LoadId() const { return mLoadId; }
  void SetLoadId(void* aLoadId) { mLoadId = aLoadId; }

  
  
  
  void SetCacheEntry(imgCacheEntry* aEntry);

  
  bool HasCacheEntry() const;

  
  
  
  void SetIsInCache(bool aCacheable);

  void EvictFromCache();
  void RemoveFromCache();

  
  void SetProperties(const nsACString& aContentType,
                     const nsACString& aContentDisposition);

  nsIProperties* Properties() const { return mProperties; }

  bool HasConsumers() const;

private:
  friend class FinishPreparingForNewPartRunnable;

  virtual ~imgRequest();

  void FinishPreparingForNewPart(const NewPartResult& aResult);

  void Cancel(nsresult aStatus);

  
  void UpdateCacheEntrySize();

  
  bool IsDecodeRequested() const;

  
  imgLoader* mLoader;
  nsCOMPtr<nsIRequest> mRequest;
  
  
  
  nsRefPtr<ImageURL> mURI;
  
  nsCOMPtr<nsIURI> mCurrentURI;
  
  
  nsCOMPtr<nsIPrincipal> mLoadingPrincipal;
  
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIProperties> mProperties;
  nsCOMPtr<nsISupports> mSecurityInfo;
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsIInterfaceRequestor> mPrevChannelSink;
  nsCOMPtr<nsIApplicationCache> mApplicationCache;

  nsCOMPtr<nsITimedChannel> mTimedChannel;

  nsCString mContentType;

  
  nsRefPtr<imgCacheEntry> mCacheEntry;

  void* mLoadId;

  imgCacheValidator* mValidator;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;
  nsCOMPtr<nsIChannel> mNewRedirectChannel;

  
  uint64_t mInnerWindowId;

  
  
  int32_t mCORSMode;

  
  ReferrerPolicy mReferrerPolicy;

  nsresult mImageErrorCode;

  mutable mozilla::Mutex mMutex;

  
  
  
  nsRefPtr<ProgressTracker> mProgressTracker;
  nsRefPtr<Image> mImage;
  bool mIsMultiPartChannel : 1;
  bool mGotData : 1;
  bool mIsInCache : 1;
  bool mDecodeRequested : 1;
  bool mNewPartPending : 1;
  bool mHadInsecureRedirect : 1;
};

#endif 
