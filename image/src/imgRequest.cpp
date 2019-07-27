





#include "imgRequest.h"
#include "ImageLogging.h"

#include "imgLoader.h"
#include "imgRequestProxy.h"
#include "DecodePool.h"
#include "ProgressTracker.h"
#include "ImageFactory.h"
#include "Image.h"
#include "MultipartImage.h"
#include "RasterImage.h"

#include "nsIChannel.h"
#include "nsICachingChannel.h"
#include "nsIDocument.h"
#include "nsIThreadRetargetableRequest.h"
#include "nsIInputStream.h"
#include "nsIMultiPartChannel.h"
#include "nsIHttpChannel.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheChannel.h"
#include "nsMimeTypes.h"

#include "nsIInterfaceRequestorUtils.h"
#include "nsISupportsPrimitives.h"
#include "nsIScriptSecurityManager.h"
#include "nsContentUtils.h"

#include "nsICacheEntry.h"

#include "plstr.h" 
#include "nsNetUtil.h"
#include "nsIProtocolHandler.h"
#include "imgIRequest.h"

using namespace mozilla;
using namespace mozilla::image;

#if defined(PR_LOGGING)
PRLogModuleInfo*
GetImgLog()
{
  static PRLogModuleInfo* sImgLog;
  if (!sImgLog) {
    sImgLog = PR_NewLogModule("imgRequest");
  }
  return sImgLog;
}
#define LOG_TEST(level) (GetImgLog() && PR_LOG_TEST(GetImgLog(), (level)))
#else
#define LOG_TEST(level) false
#endif

NS_IMPL_ISUPPORTS(imgRequest,
                  nsIStreamListener, nsIRequestObserver,
                  nsIThreadRetargetableStreamListener,
                  nsIChannelEventSink,
                  nsIInterfaceRequestor,
                  nsIAsyncVerifyRedirectCallback)

imgRequest::imgRequest(imgLoader* aLoader)
 : mLoader(aLoader)
 , mValidator(nullptr)
 , mInnerWindowId(0)
 , mCORSMode(imgIRequest::CORS_NONE)
 , mReferrerPolicy(mozilla::net::RP_Default)
 , mImageErrorCode(NS_OK)
 , mMutex("imgRequest")
 , mProgressTracker(new ProgressTracker())
 , mIsMultiPartChannel(false)
 , mGotData(false)
 , mIsInCache(false)
 , mDecodeRequested(false)
 , mNewPartPending(false)
 , mHadInsecureRedirect(false)
{ }

imgRequest::~imgRequest()
{
  if (mLoader) {
    mLoader->RemoveFromUncachedImages(this);
  }
  if (mURI) {
    nsAutoCString spec;
    mURI->GetSpec(spec);
    LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequest::~imgRequest()",
                        "keyuri", spec.get());
  } else
    LOG_FUNC(GetImgLog(), "imgRequest::~imgRequest()");
}

nsresult
imgRequest::Init(nsIURI *aURI,
                 nsIURI *aCurrentURI,
                 bool aHadInsecureRedirect,
                 nsIRequest *aRequest,
                 nsIChannel *aChannel,
                 imgCacheEntry *aCacheEntry,
                 nsISupports* aCX,
                 nsIPrincipal* aLoadingPrincipal,
                 int32_t aCORSMode,
                 ReferrerPolicy aReferrerPolicy)
{
  MOZ_ASSERT(NS_IsMainThread(), "Cannot use nsIURI off main thread!");

  LOG_FUNC(GetImgLog(), "imgRequest::Init");

  MOZ_ASSERT(!mImage, "Multiple calls to init");
  MOZ_ASSERT(aURI, "No uri");
  MOZ_ASSERT(aCurrentURI, "No current uri");
  MOZ_ASSERT(aRequest, "No request");
  MOZ_ASSERT(aChannel, "No channel");

  mProperties = do_CreateInstance("@mozilla.org/properties;1");

  
  mURI = new ImageURL(aURI);
  mCurrentURI = aCurrentURI;
  mRequest = aRequest;
  mChannel = aChannel;
  mTimedChannel = do_QueryInterface(mChannel);

  mLoadingPrincipal = aLoadingPrincipal;
  mCORSMode = aCORSMode;
  mReferrerPolicy = aReferrerPolicy;

  
  
  
  
  if (aURI != aCurrentURI) {
    bool isHttps = false;
    bool isChrome = false;
    bool schemeLocal = false;
    if (NS_FAILED(aURI->SchemeIs("https", &isHttps)) ||
        NS_FAILED(aURI->SchemeIs("chrome", &isChrome)) ||
        NS_FAILED(NS_URIChainHasFlags(
                  aURI,
                  nsIProtocolHandler::URI_IS_LOCAL_RESOURCE , &schemeLocal))  ||
        (!isHttps && !isChrome && !schemeLocal)) {
      mHadInsecureRedirect = true;
    }
  }

  
  
  mHadInsecureRedirect = mHadInsecureRedirect || aHadInsecureRedirect;

  mChannel->GetNotificationCallbacks(getter_AddRefs(mPrevChannelSink));

  NS_ASSERTION(mPrevChannelSink != this,
               "Initializing with a channel that already calls back to us!");

  mChannel->SetNotificationCallbacks(this);

  mCacheEntry = aCacheEntry;

  SetLoadId(aCX);

  
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(aCX);
  if (doc) {
    mInnerWindowId = doc->InnerWindowID();
  }

  return NS_OK;
}

void
imgRequest::ClearLoader() {
  mLoader = nullptr;
}

already_AddRefed<ProgressTracker>
imgRequest::GetProgressTracker() const
{
  MutexAutoLock lock(mMutex);

  if (mImage) {
    MOZ_ASSERT(!mProgressTracker,
               "Should have given mProgressTracker to mImage");
    return mImage->GetProgressTracker();
  } else {
    MOZ_ASSERT(mProgressTracker,
               "Should have mProgressTracker until we create mImage");
    nsRefPtr<ProgressTracker> progressTracker = mProgressTracker;
    MOZ_ASSERT(progressTracker);
    return progressTracker.forget();
  }
}

void imgRequest::SetCacheEntry(imgCacheEntry* entry)
{
  mCacheEntry = entry;
}

bool
imgRequest::HasCacheEntry() const
{
  return mCacheEntry != nullptr;
}

void
imgRequest::ResetCacheEntry()
{
  if (HasCacheEntry()) {
    mCacheEntry->SetDataSize(0);
  }
}

void
imgRequest::AddProxy(imgRequestProxy* proxy)
{
  NS_PRECONDITION(proxy, "null imgRequestProxy passed in");
  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgRequest::AddProxy", "proxy", proxy);

  
  
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  if (progressTracker->ObserverCount() == 0) {
    MOZ_ASSERT(mURI, "Trying to SetHasProxies without key uri.");
    if (mLoader) {
      mLoader->SetHasProxies(this);
    }
  }

  progressTracker->AddObserver(proxy);
}

nsresult
imgRequest::RemoveProxy(imgRequestProxy* proxy, nsresult aStatus)
{
  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgRequest::RemoveProxy", "proxy", proxy);

  
  
  
  proxy->ClearAnimationConsumers();

  
  
  
  
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  if (!progressTracker->RemoveObserver(proxy)) {
    return NS_OK;
  }

  if (progressTracker->ObserverCount() == 0) {
    
    
    
    if (mCacheEntry) {
      MOZ_ASSERT(mURI, "Removing last observer without key uri.");

      if (mLoader) {
        mLoader->SetHasNoProxies(this, mCacheEntry);
      }
    }
#if defined(PR_LOGGING)
    else {
      nsAutoCString spec;
      mURI->GetSpec(spec);
      LOG_MSG_WITH_PARAM(GetImgLog(),
                         "imgRequest::RemoveProxy no cache entry",
                         "uri", spec.get());
    }
#endif

    





    if (!(progressTracker->GetProgress() & FLAG_LAST_PART_COMPLETE) &&
        NS_FAILED(aStatus)) {
      LOG_MSG(GetImgLog(), "imgRequest::RemoveProxy",
              "load in progress.  canceling");

      this->Cancel(NS_BINDING_ABORTED);
    }

    
    mCacheEntry = nullptr;
  }

  
  
  if (aStatus != NS_IMAGELIB_CHANGING_OWNER) {
    proxy->RemoveFromLoadGroup(true);
  }

  return NS_OK;
}

void
imgRequest::CancelAndAbort(nsresult aStatus)
{
  LOG_SCOPE(GetImgLog(), "imgRequest::CancelAndAbort");

  Cancel(aStatus);

  
  
  
  if (mChannel) {
    mChannel->SetNotificationCallbacks(mPrevChannelSink);
    mPrevChannelSink = nullptr;
  }
}

class imgRequestMainThreadCancel : public nsRunnable
{
public:
  imgRequestMainThreadCancel(imgRequest* aImgRequest, nsresult aStatus)
    : mImgRequest(aImgRequest)
    , mStatus(aStatus)
  {
    MOZ_ASSERT(!NS_IsMainThread(), "Create me off main thread only!");
    MOZ_ASSERT(aImgRequest);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread(), "I should be running on the main thread!");
    mImgRequest->ContinueCancel(mStatus);
    return NS_OK;
  }
private:
  nsRefPtr<imgRequest> mImgRequest;
  nsresult mStatus;
};

void
imgRequest::Cancel(nsresult aStatus)
{
  
  LOG_SCOPE(GetImgLog(), "imgRequest::Cancel");

  if (NS_IsMainThread()) {
    ContinueCancel(aStatus);
  } else {
    NS_DispatchToMainThread(new imgRequestMainThreadCancel(this, aStatus));
  }
}

void
imgRequest::ContinueCancel(nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  progressTracker->SyncNotifyProgress(FLAG_HAS_ERROR | FLAG_ONLOAD_UNBLOCKED);

  RemoveFromCache();

  if (mRequest && !(progressTracker->GetProgress() & FLAG_LAST_PART_COMPLETE)) {
     mRequest->Cancel(aStatus);
  }
}

class imgRequestMainThreadEvict : public nsRunnable
{
public:
  explicit imgRequestMainThreadEvict(imgRequest* aImgRequest)
    : mImgRequest(aImgRequest)
  {
    MOZ_ASSERT(!NS_IsMainThread(), "Create me off main thread only!");
    MOZ_ASSERT(aImgRequest);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread(), "I should be running on the main thread!");
    mImgRequest->ContinueEvict();
    return NS_OK;
  }
private:
  nsRefPtr<imgRequest> mImgRequest;
};


void
imgRequest::EvictFromCache()
{
  
  LOG_SCOPE(GetImgLog(), "imgRequest::EvictFromCache");

  if (NS_IsMainThread()) {
    ContinueEvict();
  } else {
    NS_DispatchToMainThread(new imgRequestMainThreadEvict(this));
  }
}


void
imgRequest::ContinueEvict()
{
  MOZ_ASSERT(NS_IsMainThread());

  RemoveFromCache();
}

void
imgRequest::RequestDecode()
{
  MutexAutoLock lock(mMutex);
  mDecodeRequested = true;
}

bool
imgRequest::IsDecodeRequested() const
{
  MutexAutoLock lock(mMutex);
  return mDecodeRequested;
}

nsresult imgRequest::GetURI(ImageURL** aURI)
{
  MOZ_ASSERT(aURI);

  LOG_FUNC(GetImgLog(), "imgRequest::GetURI");

  if (mURI) {
    *aURI = mURI;
    NS_ADDREF(*aURI);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

nsresult
imgRequest::GetCurrentURI(nsIURI** aURI)
{
  MOZ_ASSERT(aURI);

  LOG_FUNC(GetImgLog(), "imgRequest::GetCurrentURI");

  if (mCurrentURI) {
    *aURI = mCurrentURI;
    NS_ADDREF(*aURI);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

bool
imgRequest::IsChrome() const
{
  bool isChrome = false;
  if (NS_WARN_IF(NS_FAILED(mURI->SchemeIs("chrome", &isChrome)))) {
    return false;
  }
  return isChrome;
}

nsresult
imgRequest::GetImageErrorCode()
{
  return mImageErrorCode;
}

nsresult
imgRequest::GetSecurityInfo(nsISupports** aSecurityInfo)
{
  LOG_FUNC(GetImgLog(), "imgRequest::GetSecurityInfo");

  
  
  NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);
  return NS_OK;
}

void
imgRequest::RemoveFromCache()
{
  LOG_SCOPE(GetImgLog(), "imgRequest::RemoveFromCache");

  bool isInCache = false;

  {
    MutexAutoLock lock(mMutex);
    isInCache = mIsInCache;
  }

  if (isInCache && mLoader) {
    
    if (mCacheEntry) {
      mLoader->RemoveFromCache(mCacheEntry);
    } else {
      mLoader->RemoveFromCache(ImageCacheKey(mURI));
    }
  }

  mCacheEntry = nullptr;
}

bool
imgRequest::HasConsumers() const
{
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  return progressTracker && progressTracker->ObserverCount() > 0;
}

already_AddRefed<Image>
imgRequest::GetImage() const
{
  MutexAutoLock lock(mMutex);
  nsRefPtr<Image> image = mImage;
  return image.forget();
}

int32_t imgRequest::Priority() const
{
  int32_t priority = nsISupportsPriority::PRIORITY_NORMAL;
  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mRequest);
  if (p) {
    p->GetPriority(&priority);
  }
  return priority;
}

void
imgRequest::AdjustPriority(imgRequestProxy* proxy, int32_t delta)
{
  
  
  
  
  
  
  
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  if (!progressTracker->FirstObserverIs(proxy)) {
    return;
  }

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mChannel);
  if (p) {
    p->AdjustPriority(delta);
  }
}

bool
imgRequest::HasTransferredData() const
{
  MutexAutoLock lock(mMutex);
  return mGotData;
}

void
imgRequest::SetIsInCache(bool aInCache)
{
  LOG_FUNC_WITH_PARAM(GetImgLog(),
                      "imgRequest::SetIsCacheable", "aInCache", aInCache);
  MutexAutoLock lock(mMutex);
  mIsInCache = aInCache;
}

void
imgRequest::UpdateCacheEntrySize()
{
  if (!mCacheEntry) {
    return;
  }

  nsRefPtr<Image> image = GetImage();
  size_t size = image->SizeOfSourceWithComputedFallback(moz_malloc_size_of);
  mCacheEntry->SetDataSize(size);
}

void
imgRequest::SetCacheValidation(imgCacheEntry* aCacheEntry, nsIRequest* aRequest)
{
  
  if (aCacheEntry) {
    nsCOMPtr<nsICachingChannel> cacheChannel(do_QueryInterface(aRequest));
    if (cacheChannel) {
      nsCOMPtr<nsISupports> cacheToken;
      cacheChannel->GetCacheToken(getter_AddRefs(cacheToken));
      if (cacheToken) {
        nsCOMPtr<nsICacheEntry> entryDesc(do_QueryInterface(cacheToken));
        if (entryDesc) {
          uint32_t expiration;
          
          entryDesc->GetExpirationTime(&expiration);

          
          
          if (aCacheEntry->GetExpiryTime() == 0) {
            aCacheEntry->SetExpiryTime(expiration);
          }
        }
      }
    }

    
    
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
    if (httpChannel) {
      bool bMustRevalidate = false;

      httpChannel->IsNoStoreResponse(&bMustRevalidate);

      if (!bMustRevalidate) {
        httpChannel->IsNoCacheResponse(&bMustRevalidate);
      }

      if (!bMustRevalidate) {
        nsAutoCString cacheHeader;

        httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Cache-Control"),
                                            cacheHeader);
        if (PL_strcasestr(cacheHeader.get(), "must-revalidate")) {
          bMustRevalidate = true;
        }
      }

      
      
      
      if (bMustRevalidate) {
        aCacheEntry->SetMustValidate(bMustRevalidate);
      }
    }
  }
}

namespace { 

already_AddRefed<nsIApplicationCache>
GetApplicationCache(nsIRequest* aRequest)
{
  nsresult rv;

  nsCOMPtr<nsIApplicationCacheChannel> appCacheChan =
    do_QueryInterface(aRequest);
  if (!appCacheChan) {
    return nullptr;
  }

  bool fromAppCache;
  rv = appCacheChan->GetLoadedFromApplicationCache(&fromAppCache);
  NS_ENSURE_SUCCESS(rv, nullptr);

  if (!fromAppCache) {
    return nullptr;
  }

  nsCOMPtr<nsIApplicationCache> appCache;
  rv = appCacheChan->GetApplicationCache(getter_AddRefs(appCache));
  NS_ENSURE_SUCCESS(rv, nullptr);

  return appCache.forget();
}

} 

bool
imgRequest::CacheChanged(nsIRequest* aNewRequest)
{
  nsCOMPtr<nsIApplicationCache> newAppCache = GetApplicationCache(aNewRequest);

  
  
  if (newAppCache == mApplicationCache) {
    return false;
  }

  
  
  if (newAppCache && mApplicationCache) {
    nsresult rv;

    nsAutoCString oldAppCacheClientId, newAppCacheClientId;
    rv = mApplicationCache->GetClientID(oldAppCacheClientId);
    NS_ENSURE_SUCCESS(rv, true);
    rv = newAppCache->GetClientID(newAppCacheClientId);
    NS_ENSURE_SUCCESS(rv, true);

    if (oldAppCacheClientId == newAppCacheClientId) {
      return false;
    }
  }

  
  
  
  return true;
}

bool
imgRequest::GetMultipart() const
{
  MutexAutoLock lock(mMutex);
  return mIsMultiPartChannel;
}

bool
imgRequest::HadInsecureRedirect() const
{
  MutexAutoLock lock(mMutex);
  return mHadInsecureRedirect;
}




NS_IMETHODIMP
imgRequest::OnStartRequest(nsIRequest* aRequest, nsISupports* ctxt)
{
  LOG_SCOPE(GetImgLog(), "imgRequest::OnStartRequest");

  nsRefPtr<Image> image;

  
  nsCOMPtr<nsIMultiPartChannel> multiPartChannel = do_QueryInterface(aRequest);
  MOZ_ASSERT(multiPartChannel || !mIsMultiPartChannel,
             "Stopped being multipart?"); {
    MutexAutoLock lock(mMutex);
    mNewPartPending = true;
    image = mImage;
    mIsMultiPartChannel = bool(multiPartChannel);
  }

  
  if (image && !multiPartChannel) {
    MOZ_ASSERT_UNREACHABLE("Already have an image for a non-multipart request");
    Cancel(NS_IMAGELIB_ERROR_FAILURE);
    return NS_ERROR_FAILURE;
  }

  






  if (!mRequest) {
    MOZ_ASSERT(multiPartChannel, "Should have mRequest unless we're multipart");
    nsCOMPtr<nsIChannel> baseChannel;
    multiPartChannel->GetBaseChannel(getter_AddRefs(baseChannel));
    mRequest = baseChannel;
  }

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel) {
    channel->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

    
    nsCOMPtr<nsIScriptSecurityManager>
      secMan = nsContentUtils::GetSecurityManager();
    if (secMan) {
      nsresult rv =
        secMan->GetChannelResultPrincipal(channel, getter_AddRefs(mPrincipal));
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }

  SetCacheValidation(mCacheEntry, aRequest);

  mApplicationCache = GetApplicationCache(aRequest);

  
  
  nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
  if (progressTracker->ObserverCount() == 0) {
    this->Cancel(NS_IMAGELIB_ERROR_FAILURE);
  }

  
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aRequest);
  nsCOMPtr<nsIThreadRetargetableRequest> retargetable =
    do_QueryInterface(aRequest);
  if (httpChannel && retargetable) {
    nsAutoCString mimeType;
    nsresult rv = httpChannel->GetContentType(mimeType);
    if (NS_SUCCEEDED(rv) && !mimeType.EqualsLiteral(IMAGE_SVG_XML)) {
      
      nsCOMPtr<nsIEventTarget> target =
        DecodePool::Singleton()->GetIOEventTarget();
      rv = retargetable->RetargetDeliveryTo(target);
    }
    PR_LOG(GetImgLog(), PR_LOG_WARNING,
           ("[this=%p] imgRequest::OnStartRequest -- "
            "RetargetDeliveryTo rv %d=%s\n",
            this, rv, NS_SUCCEEDED(rv) ? "succeeded" : "failed"));
  }

  return NS_OK;
}



NS_IMETHODIMP
imgRequest::OnStopRequest(nsIRequest* aRequest,
                          nsISupports* ctxt, nsresult status)
{
  LOG_FUNC(GetImgLog(), "imgRequest::OnStopRequest");
  MOZ_ASSERT(NS_IsMainThread(), "Can't send notifications off-main-thread");

  nsRefPtr<Image> image = GetImage();

  
  
  
  
  if (mRequest) {
    mRequest = nullptr;  
  }

  
  if (mChannel) {
    mChannel->SetNotificationCallbacks(mPrevChannelSink);
    mPrevChannelSink = nullptr;
    mChannel = nullptr;
  }

  bool lastPart = true;
  nsCOMPtr<nsIMultiPartChannel> mpchan(do_QueryInterface(aRequest));
  if (mpchan) {
    mpchan->GetIsLastPart(&lastPart);
  }

  bool isPartial = false;
  if (image && (status == NS_ERROR_NET_PARTIAL_TRANSFER)) {
    isPartial = true;
    status = NS_OK; 
  }

  
  
  
  if (image) {
    nsresult rv = image->OnImageDataComplete(aRequest, ctxt, status, lastPart);

    
    
    
    
    if (NS_FAILED(rv) && NS_SUCCEEDED(status)) {
      status = rv;
    }
  }

  
  
  if (image && NS_SUCCEEDED(status) && !isPartial) {
    
    
    UpdateCacheEntrySize();

  } else if (isPartial) {
    
    this->EvictFromCache();

  } else {
    mImageErrorCode = status;

    
    
    this->Cancel(status);
  }

  if (!image) {
    
    
    Progress progress =
      LoadCompleteProgress(lastPart,  false, status);

    nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
    progressTracker->SyncNotifyProgress(progress);
  }

  mTimedChannel = nullptr;
  return NS_OK;
}

struct mimetype_closure
{
  nsACString* newType;
};


static NS_METHOD
sniff_mimetype_callback(nsIInputStream* in, void* closure,
                        const char* fromRawSegment, uint32_t toOffset,
                        uint32_t count, uint32_t* writeCount);


NS_IMETHODIMP
imgRequest::CheckListenerChain()
{
  
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread!");
  return NS_OK;
}



struct NewPartResult final
{
  explicit NewPartResult(Image* aExistingImage)
    : mImage(aExistingImage)
    , mIsFirstPart(!aExistingImage)
    , mSucceeded(false)
  { }

  nsAutoCString mContentType;
  nsAutoCString mContentDisposition;
  nsRefPtr<Image> mImage;
  const bool mIsFirstPart;
  bool mSucceeded;
};

static NewPartResult
PrepareForNewPart(nsIRequest* aRequest, nsIInputStream* aInStr, uint32_t aCount,
                  ImageURL* aURI, bool aIsMultipart, Image* aExistingImage,
                  ProgressTracker* aProgressTracker, uint32_t aInnerWindowId)
{
  NewPartResult result(aExistingImage);

  mimetype_closure closure;
  closure.newType = &result.mContentType;

  
  
  uint32_t out;
  aInStr->ReadSegments(sniff_mimetype_callback, &closure, aCount, &out);

  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  if (result.mContentType.IsEmpty()) {
    nsresult rv = NS_ERROR_FAILURE;
    if (chan) {
      rv = chan->GetContentType(result.mContentType);
      chan->GetContentDispositionHeader(result.mContentDisposition);
    }

    if (NS_FAILED(rv)) {
      PR_LOG(GetImgLog(),
             PR_LOG_ERROR, ("imgRequest::PrepareForNewPart "
                            "-- Content type unavailable from the channel\n"));
      return result;
    }
  }

  PR_LOG(GetImgLog(), PR_LOG_DEBUG,
         ("imgRequest::PrepareForNewPart -- Got content type %s\n",
          result.mContentType.get()));

  
  
  

  
  if (aIsMultipart) {
    
    nsRefPtr<ProgressTracker> progressTracker = new ProgressTracker();
    nsRefPtr<Image> partImage =
      ImageFactory::CreateImage(aRequest, progressTracker, result.mContentType,
                                aURI,  true,
                                aInnerWindowId);

    if (result.mIsFirstPart) {
      
      MOZ_ASSERT(aProgressTracker, "Shouldn't have given away tracker yet");
      result.mImage =
        ImageFactory::CreateMultipartImage(partImage, aProgressTracker);
    } else {
      
      auto multipartImage = static_cast<MultipartImage*>(aExistingImage);
      multipartImage->BeginTransitionToPart(partImage);
    }
  } else {
    MOZ_ASSERT(!aExistingImage, "New part for non-multipart channel?");
    MOZ_ASSERT(aProgressTracker, "Shouldn't have given away tracker yet");

    
    result.mImage =
      ImageFactory::CreateImage(aRequest, aProgressTracker, result.mContentType,
                                aURI,  false,
                                aInnerWindowId);
  }

  MOZ_ASSERT(result.mImage);
  if (!result.mImage->HasError() || aIsMultipart) {
    
    
    
    result.mSucceeded = true;
  }

  return result;
}

class FinishPreparingForNewPartRunnable final : public nsRunnable
{
public:
  FinishPreparingForNewPartRunnable(imgRequest* aImgRequest,
                                    NewPartResult&& aResult)
    : mImgRequest(aImgRequest)
    , mResult(aResult)
  {
    MOZ_ASSERT(aImgRequest);
  }

  NS_IMETHOD Run() override
  {
    mImgRequest->FinishPreparingForNewPart(mResult);
    return NS_OK;
  }

private:
  nsRefPtr<imgRequest> mImgRequest;
  NewPartResult mResult;
};

void
imgRequest::FinishPreparingForNewPart(const NewPartResult& aResult)
{
  MOZ_ASSERT(NS_IsMainThread());

  mContentType = aResult.mContentType;

  SetProperties(aResult.mContentType, aResult.mContentDisposition);

  if (aResult.mIsFirstPart) {
    
    nsRefPtr<ProgressTracker> progressTracker = GetProgressTracker();
    progressTracker->OnImageAvailable();
    MOZ_ASSERT(progressTracker->HasImage());
  }

  if (IsDecodeRequested()) {
    aResult.mImage->RequestDecode();
  }
}

NS_IMETHODIMP
imgRequest::OnDataAvailable(nsIRequest* aRequest, nsISupports* aContext,
                            nsIInputStream* aInStr, uint64_t aOffset,
                            uint32_t aCount)
{
  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgRequest::OnDataAvailable",
                       "count", aCount);

  NS_ASSERTION(aRequest, "imgRequest::OnDataAvailable -- no request!");

  nsRefPtr<Image> image;
  nsRefPtr<ProgressTracker> progressTracker;
  bool isMultipart = false;
  bool newPartPending = false;

  
  {
    MutexAutoLock lock(mMutex);
    mGotData = true;
    image = mImage;
    progressTracker = mProgressTracker;
    isMultipart = mIsMultiPartChannel;
    newPartPending = mNewPartPending;
    mNewPartPending = false;
  }

  
  
  if (newPartPending) {
    NewPartResult result = PrepareForNewPart(aRequest, aInStr, aCount, mURI,
                                             isMultipart, image,
                                             progressTracker, mInnerWindowId);
    bool succeeded = result.mSucceeded;

    if (result.mImage) {
      image = result.mImage;

      
      {
        MutexAutoLock lock(mMutex);
        mImage = image;
        mProgressTracker = nullptr;
      }

      
      
      if (NS_IsMainThread()) {
        FinishPreparingForNewPart(result);
      } else {
        nsCOMPtr<nsIRunnable> runnable =
          new FinishPreparingForNewPartRunnable(this, Move(result));
        NS_DispatchToMainThread(runnable);
      }
    }

    if (!succeeded) {
      
      Cancel(NS_IMAGELIB_ERROR_FAILURE);
      return NS_BINDING_ABORTED;
    }
  }

  
  nsresult rv =
    image->OnImageDataAvailable(aRequest, aContext, aInStr, aOffset, aCount);

  if (NS_FAILED(rv)) {
    PR_LOG(GetImgLog(), PR_LOG_WARNING,
           ("[this=%p] imgRequest::OnDataAvailable -- "
            "copy to RasterImage failed\n", this));
    Cancel(NS_IMAGELIB_ERROR_FAILURE);
    return NS_BINDING_ABORTED;
  }

  return NS_OK;
}

void
imgRequest::SetProperties(const nsACString& aContentType,
                          const nsACString& aContentDisposition)
{
  
  nsCOMPtr<nsISupportsCString> contentType =
    do_CreateInstance("@mozilla.org/supports-cstring;1");
  if (contentType) {
    contentType->SetData(aContentType);
    mProperties->Set("type", contentType);
  }

  
  if (!aContentDisposition.IsEmpty()) {
    nsCOMPtr<nsISupportsCString> contentDisposition =
      do_CreateInstance("@mozilla.org/supports-cstring;1");
    if (contentDisposition) {
      contentDisposition->SetData(aContentDisposition);
      mProperties->Set("content-disposition", contentDisposition);
    }
  }
}

static NS_METHOD
sniff_mimetype_callback(nsIInputStream* in,
                        void* data,
                        const char* fromRawSegment,
                        uint32_t toOffset,
                        uint32_t count,
                        uint32_t* writeCount)
{
  mimetype_closure* closure = static_cast<mimetype_closure*>(data);

  NS_ASSERTION(closure, "closure is null!");

  if (count > 0) {
    imgLoader::GetMimeTypeFromContent(fromRawSegment, count, *closure->newType);
  }

  *writeCount = 0;
  return NS_ERROR_FAILURE;
}




NS_IMETHODIMP
imgRequest::GetInterface(const nsIID & aIID, void** aResult)
{
  if (!mPrevChannelSink || aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    return QueryInterface(aIID, aResult);
  }

  NS_ASSERTION(mPrevChannelSink != this,
    "Infinite recursion - don't keep track of channel sinks that are us!");
  return mPrevChannelSink->GetInterface(aIID, aResult);
}


NS_IMETHODIMP
imgRequest::AsyncOnChannelRedirect(nsIChannel* oldChannel,
                                   nsIChannel* newChannel, uint32_t flags,
                                   nsIAsyncVerifyRedirectCallback* callback)
{
  NS_ASSERTION(mRequest && mChannel,
               "Got a channel redirect after we nulled out mRequest!");
  NS_ASSERTION(mChannel == oldChannel,
               "Got a channel redirect for an unknown channel!");
  NS_ASSERTION(newChannel, "Got a redirect to a NULL channel!");

  SetCacheValidation(mCacheEntry, oldChannel);

  
  mRedirectCallback = callback;
  mNewRedirectChannel = newChannel;

  nsCOMPtr<nsIChannelEventSink> sink(do_GetInterface(mPrevChannelSink));
  if (sink) {
    nsresult rv = sink->AsyncOnChannelRedirect(oldChannel, newChannel, flags,
                                               this);
    if (NS_FAILED(rv)) {
        mRedirectCallback = nullptr;
        mNewRedirectChannel = nullptr;
    }
    return rv;
  }

  (void) OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

NS_IMETHODIMP
imgRequest::OnRedirectVerifyCallback(nsresult result)
{
  NS_ASSERTION(mRedirectCallback, "mRedirectCallback not set in callback");
  NS_ASSERTION(mNewRedirectChannel, "mNewRedirectChannel not set in callback");

  if (NS_FAILED(result)) {
      mRedirectCallback->OnRedirectVerifyCallback(result);
      mRedirectCallback = nullptr;
      mNewRedirectChannel = nullptr;
      return NS_OK;
  }

  mChannel = mNewRedirectChannel;
  mTimedChannel = do_QueryInterface(mChannel);
  mNewRedirectChannel = nullptr;

  if (LOG_TEST(PR_LOG_DEBUG)) {
    nsAutoCString spec;
    if (mCurrentURI) {
      mCurrentURI->GetSpec(spec);
    }
    LOG_MSG_WITH_PARAM(GetImgLog(),
                       "imgRequest::OnChannelRedirect", "old", spec.get());
  }

  
  
  
  bool isHttps = false;
  bool isChrome = false;
  bool schemeLocal = false;
  if (NS_FAILED(mCurrentURI->SchemeIs("https", &isHttps)) ||
      NS_FAILED(mCurrentURI->SchemeIs("chrome", &isChrome)) ||
      NS_FAILED(NS_URIChainHasFlags(mCurrentURI,
                                    nsIProtocolHandler::URI_IS_LOCAL_RESOURCE,
                                    &schemeLocal))  ||
      (!isHttps && !isChrome && !schemeLocal)) {
    MutexAutoLock lock(mMutex);
    mHadInsecureRedirect = true;
  }

  
  mChannel->GetURI(getter_AddRefs(mCurrentURI));

  if (LOG_TEST(PR_LOG_DEBUG)) {
    nsAutoCString spec;
    if (mCurrentURI) {
      mCurrentURI->GetSpec(spec);
    }
    LOG_MSG_WITH_PARAM(GetImgLog(), "imgRequest::OnChannelRedirect",
                       "new", spec.get());
  }

  
  
  bool doesNotReturnData = false;
  nsresult rv =
    NS_URIChainHasFlags(mCurrentURI,
                        nsIProtocolHandler::URI_DOES_NOT_RETURN_DATA,
                        &doesNotReturnData);

  if (NS_SUCCEEDED(rv) && doesNotReturnData) {
    rv = NS_ERROR_ABORT;
  }

  if (NS_FAILED(rv)) {
    mRedirectCallback->OnRedirectVerifyCallback(rv);
    mRedirectCallback = nullptr;
    return NS_OK;
  }

  mRedirectCallback->OnRedirectVerifyCallback(NS_OK);
  mRedirectCallback = nullptr;
  return NS_OK;
}
