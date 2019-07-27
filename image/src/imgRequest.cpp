





#include "imgRequest.h"
#include "ImageLogging.h"

#include "imgLoader.h"
#include "imgRequestProxy.h"
#include "imgStatusTracker.h"
#include "ImageFactory.h"
#include "Image.h"
#include "RasterImage.h"

#include "nsIChannel.h"
#include "nsICachingChannel.h"
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
PRLogModuleInfo *
GetImgLog()
{
  static PRLogModuleInfo *sImgLog;
  if (!sImgLog)
    sImgLog = PR_NewLogModule("imgRequest");
  return sImgLog;
}
#endif

NS_IMPL_ISUPPORTS(imgRequest,
                  nsIStreamListener, nsIRequestObserver,
                  nsIThreadRetargetableStreamListener,
                  nsIChannelEventSink,
                  nsIInterfaceRequestor,
                  nsIAsyncVerifyRedirectCallback)

imgRequest::imgRequest(imgLoader* aLoader)
 : mLoader(aLoader)
 , mStatusTracker(new imgStatusTracker(nullptr))
 , mValidator(nullptr)
 , mInnerWindowId(0)
 , mCORSMode(imgIRequest::CORS_NONE)
 , mImageErrorCode(NS_OK)
 , mDecodeRequested(false)
 , mIsMultiPartChannel(false)
 , mGotData(false)
 , mIsInCache(false)
 , mResniffMimeType(false)
{ }

imgRequest::~imgRequest()
{
  if (mURI) {
    nsAutoCString spec;
    mURI->GetSpec(spec);
    LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequest::~imgRequest()", "keyuri", spec.get());
  } else
    LOG_FUNC(GetImgLog(), "imgRequest::~imgRequest()");
}

nsresult imgRequest::Init(nsIURI *aURI,
                          nsIURI *aCurrentURI,
                          nsIRequest *aRequest,
                          nsIChannel *aChannel,
                          imgCacheEntry *aCacheEntry,
                          void *aLoadId,
                          nsIPrincipal* aLoadingPrincipal,
                          int32_t aCORSMode)
{
  MOZ_ASSERT(NS_IsMainThread(), "Cannot use nsIURI off main thread!");

  LOG_FUNC(GetImgLog(), "imgRequest::Init");

  NS_ABORT_IF_FALSE(!mImage, "Multiple calls to init");
  NS_ABORT_IF_FALSE(aURI, "No uri");
  NS_ABORT_IF_FALSE(aCurrentURI, "No current uri");
  NS_ABORT_IF_FALSE(aRequest, "No request");
  NS_ABORT_IF_FALSE(aChannel, "No channel");

  mProperties = do_CreateInstance("@mozilla.org/properties;1");

  
  mURI = new ImageURL(aURI);
  mCurrentURI = aCurrentURI;
  mRequest = aRequest;
  mChannel = aChannel;
  mTimedChannel = do_QueryInterface(mChannel);

  mLoadingPrincipal = aLoadingPrincipal;
  mCORSMode = aCORSMode;

  mChannel->GetNotificationCallbacks(getter_AddRefs(mPrevChannelSink));

  NS_ASSERTION(mPrevChannelSink != this,
               "Initializing with a channel that already calls back to us!");

  mChannel->SetNotificationCallbacks(this);

  mCacheEntry = aCacheEntry;

  SetLoadId(aLoadId);

  return NS_OK;
}

already_AddRefed<imgStatusTracker>
imgRequest::GetStatusTracker()
{
  if (mImage && mGotData) {
    NS_ABORT_IF_FALSE(!mStatusTracker,
                      "Should have given mStatusTracker to mImage");
    return mImage->GetStatusTracker();
  } else {
    NS_ABORT_IF_FALSE(mStatusTracker,
                      "Should have mStatusTracker until we create mImage");
    nsRefPtr<imgStatusTracker> statusTracker = mStatusTracker;
    MOZ_ASSERT(statusTracker);
    return statusTracker.forget();
  }
}

void imgRequest::SetCacheEntry(imgCacheEntry *entry)
{
  mCacheEntry = entry;
}

bool imgRequest::HasCacheEntry() const
{
  return mCacheEntry != nullptr;
}

void imgRequest::ResetCacheEntry()
{
  if (HasCacheEntry()) {
    mCacheEntry->SetDataSize(0);
  }
}

void imgRequest::AddProxy(imgRequestProxy *proxy)
{
  NS_PRECONDITION(proxy, "null imgRequestProxy passed in");
  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgRequest::AddProxy", "proxy", proxy);

  
  
  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  if (statusTracker->ConsumerCount() == 0) {
    NS_ABORT_IF_FALSE(mURI, "Trying to SetHasProxies without key uri.");
    mLoader->SetHasProxies(mURI);
  }

  statusTracker->AddConsumer(proxy);
}

nsresult imgRequest::RemoveProxy(imgRequestProxy *proxy, nsresult aStatus)
{
  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgRequest::RemoveProxy", "proxy", proxy);

  
  
  
  proxy->ClearAnimationConsumers();

  
  
  
  
  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  if (!statusTracker->RemoveConsumer(proxy, aStatus))
    return NS_OK;

  if (statusTracker->ConsumerCount() == 0) {
    
    
    
    if (mCacheEntry) {
      NS_ABORT_IF_FALSE(mURI, "Removing last observer without key uri.");

      mLoader->SetHasNoProxies(mURI, mCacheEntry);
    }
#if defined(PR_LOGGING)
    else {
      nsAutoCString spec;
      mURI->GetSpec(spec);
      LOG_MSG_WITH_PARAM(GetImgLog(), "imgRequest::RemoveProxy no cache entry", "uri", spec.get());
    }
#endif

    




    if (statusTracker->IsLoading() && NS_FAILED(aStatus)) {
      LOG_MSG(GetImgLog(), "imgRequest::RemoveProxy", "load in progress.  canceling");

      this->Cancel(NS_BINDING_ABORTED);
    }

    
    mCacheEntry = nullptr;
  }

  
  
  if (aStatus != NS_IMAGELIB_CHANGING_OWNER)
    proxy->RemoveFromLoadGroup(true);

  return NS_OK;
}

void imgRequest::CancelAndAbort(nsresult aStatus)
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
  imgRequestMainThreadCancel(imgRequest *aImgRequest, nsresult aStatus)
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

void imgRequest::Cancel(nsresult aStatus)
{
  

  LOG_SCOPE(GetImgLog(), "imgRequest::Cancel");

  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();

  statusTracker->MaybeUnblockOnload();

  statusTracker->RecordCancel();

  if (NS_IsMainThread()) {
    ContinueCancel(aStatus);
  } else {
    NS_DispatchToMainThread(new imgRequestMainThreadCancel(this, aStatus));
  }
}

void imgRequest::ContinueCancel(nsresult aStatus)
{
  MOZ_ASSERT(NS_IsMainThread());

  RemoveFromCache();

  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  if (mRequest && statusTracker->IsLoading()) {
     mRequest->Cancel(aStatus);
  }
}

class imgRequestMainThreadEvict : public nsRunnable
{
public:
  explicit imgRequestMainThreadEvict(imgRequest *aImgRequest)
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


void imgRequest::EvictFromCache()
{
  
  LOG_SCOPE(GetImgLog(), "imgRequest::EvictFromCache");

  if (NS_IsMainThread()) {
    ContinueEvict();
  } else {
    NS_DispatchToMainThread(new imgRequestMainThreadEvict(this));
  }
}


void imgRequest::ContinueEvict()
{
  MOZ_ASSERT(NS_IsMainThread());

  RemoveFromCache();
}

nsresult imgRequest::GetURI(ImageURL **aURI)
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

nsresult imgRequest::GetImageErrorCode()
{
  return mImageErrorCode;
}

nsresult imgRequest::GetSecurityInfo(nsISupports **aSecurityInfo)
{
  LOG_FUNC(GetImgLog(), "imgRequest::GetSecurityInfo");

  
  
  NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);
  return NS_OK;
}

void imgRequest::RemoveFromCache()
{
  LOG_SCOPE(GetImgLog(), "imgRequest::RemoveFromCache");

  if (mIsInCache) {
    
    if (mCacheEntry)
      mLoader->RemoveFromCache(mCacheEntry);
    else
      mLoader->RemoveFromCache(mURI);
  }

  mCacheEntry = nullptr;
}

int32_t imgRequest::Priority() const
{
  int32_t priority = nsISupportsPriority::PRIORITY_NORMAL;
  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mRequest);
  if (p)
    p->GetPriority(&priority);
  return priority;
}

void imgRequest::AdjustPriority(imgRequestProxy *proxy, int32_t delta)
{
  
  
  
  
  
  
  
  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  if (!statusTracker->FirstConsumerIs(proxy))
    return;

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mChannel);
  if (p)
    p->AdjustPriority(delta);
}

void imgRequest::SetIsInCache(bool incache)
{
  LOG_FUNC_WITH_PARAM(GetImgLog(), "imgRequest::SetIsCacheable", "incache", incache);
  mIsInCache = incache;
}

void imgRequest::UpdateCacheEntrySize()
{
  if (mCacheEntry)
    mCacheEntry->SetDataSize(mImage->SizeOfData());
}

void imgRequest::SetCacheValidation(imgCacheEntry* aCacheEntry, nsIRequest* aRequest)
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

          
          
          if (aCacheEntry->GetExpiryTime() == 0)
            aCacheEntry->SetExpiryTime(expiration);
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

      
      
      
      if (bMustRevalidate)
        aCacheEntry->SetMustValidate(bMustRevalidate);
    }

    
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
    if (channel) {
      nsCOMPtr<nsIURI> uri;
      channel->GetURI(getter_AddRefs(uri));
      bool isfile = false;
      uri->SchemeIs("file", &isfile);
      if (isfile)
        aCacheEntry->SetMustValidate(isfile);
    }
  }
}

namespace { 

already_AddRefed<nsIApplicationCache>
GetApplicationCache(nsIRequest* aRequest)
{
  nsresult rv;

  nsCOMPtr<nsIApplicationCacheChannel> appCacheChan = do_QueryInterface(aRequest);
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

  
  
  if (newAppCache == mApplicationCache)
    return false;

  
  
  if (newAppCache && mApplicationCache) {
    nsresult rv;

    nsAutoCString oldAppCacheClientId, newAppCacheClientId;
    rv = mApplicationCache->GetClientID(oldAppCacheClientId);
    NS_ENSURE_SUCCESS(rv, true);
    rv = newAppCache->GetClientID(newAppCacheClientId);
    NS_ENSURE_SUCCESS(rv, true);

    if (oldAppCacheClientId == newAppCacheClientId)
      return false;
  }

  
  
  
  return true;
}

nsresult
imgRequest::LockImage()
{
  return mImage->LockImage();
}

nsresult
imgRequest::UnlockImage()
{
  return mImage->UnlockImage();
}

nsresult
imgRequest::RequestDecode()
{
  
  if (mImage) {
    return mImage->RequestDecode();
  }

  
  mDecodeRequested = true;

  return NS_OK;
}

nsresult
imgRequest::StartDecoding()
{
  
  if (mImage) {
    return mImage->StartDecoding();
  }

  
  mDecodeRequested = true;

  return NS_OK;
}




NS_IMETHODIMP imgRequest::OnStartRequest(nsIRequest *aRequest, nsISupports *ctxt)
{
  LOG_SCOPE(GetImgLog(), "imgRequest::OnStartRequest");

  
  nsCOMPtr<nsIMultiPartChannel> mpchan(do_QueryInterface(aRequest));
  nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
  if (mpchan) {
    mIsMultiPartChannel = true;
    statusTracker->SetIsMultipart();
  } else {
    NS_ABORT_IF_FALSE(!mIsMultiPartChannel, "Something went wrong");
  }

  
  NS_ABORT_IF_FALSE(mIsMultiPartChannel || !mImage,
                    "Already have an image for non-multipart request");

  
  
  if (mIsMultiPartChannel && mImage) {
    mResniffMimeType = true;

    
    
    
    
    
    mImage->OnNewSourceData();
  }

  






  if (!mRequest) {
    NS_ASSERTION(mpchan,
                 "We should have an mRequest here unless we're multipart");
    nsCOMPtr<nsIChannel> chan;
    mpchan->GetBaseChannel(getter_AddRefs(chan));
    mRequest = chan;
  }

  
  statusTracker = GetStatusTracker();
  statusTracker->OnStartRequest();

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel)
    channel->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

  
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  if (chan) {
    nsCOMPtr<nsIScriptSecurityManager> secMan = nsContentUtils::GetSecurityManager();
    if (secMan) {
      nsresult rv = secMan->GetChannelPrincipal(chan,
                                                getter_AddRefs(mPrincipal));
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
  }

  SetCacheValidation(mCacheEntry, aRequest);

  mApplicationCache = GetApplicationCache(aRequest);

  
  if (statusTracker->ConsumerCount() == 0) {
    this->Cancel(NS_IMAGELIB_ERROR_FAILURE);
  }

  
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aRequest);
  nsCOMPtr<nsIThreadRetargetableRequest> retargetable =
    do_QueryInterface(aRequest);
  if (httpChannel && retargetable &&
      ImageFactory::CanRetargetOnDataAvailable(mURI, mIsMultiPartChannel)) {
    nsAutoCString mimeType;
    nsresult rv = httpChannel->GetContentType(mimeType);
    if (NS_SUCCEEDED(rv) && !mimeType.EqualsLiteral(IMAGE_SVG_XML)) {
      
      
      nsCOMPtr<nsIEventTarget> target = RasterImage::GetEventTarget();
      rv = retargetable->RetargetDeliveryTo(target);
    }
    PR_LOG(GetImgLog(), PR_LOG_WARNING,
           ("[this=%p] imgRequest::OnStartRequest -- "
            "RetargetDeliveryTo rv %d=%s\n",
            this, rv, NS_SUCCEEDED(rv) ? "succeeded" : "failed"));
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopRequest(nsIRequest *aRequest, nsISupports *ctxt, nsresult status)
{
  LOG_FUNC(GetImgLog(), "imgRequest::OnStopRequest");

  
  
  
  
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
  if (mpchan)
    mpchan->GetIsLastPart(&lastPart);

  bool isPartial = false;
  if (mImage && (status == NS_ERROR_NET_PARTIAL_TRANSFER)) {
    isPartial = true;
    status = NS_OK; 
  }

  
  
  
  if (mImage) {
    nsresult rv = mImage->OnImageDataComplete(aRequest, ctxt, status, lastPart);

    
    
    
    
    if (NS_FAILED(rv) && NS_SUCCEEDED(status))
      status = rv;
  }

  
  
  if (mImage && NS_SUCCEEDED(status) && !isPartial) {
    
    
    UpdateCacheEntrySize();
  }
  else if (isPartial) {
    
    this->EvictFromCache();
  }
  else {
    mImageErrorCode = status;

    
    
    this->Cancel(status);
  }

  if (!mImage) {
    
    
    nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
    statusTracker->OnStopRequest(lastPart, status);
  }

  mTimedChannel = nullptr;
  return NS_OK;
}

struct mimetype_closure
{
  nsACString* newType;
};


static NS_METHOD sniff_mimetype_callback(nsIInputStream* in, void* closure, const char* fromRawSegment,
                                         uint32_t toOffset, uint32_t count, uint32_t *writeCount);


NS_IMETHODIMP
imgRequest::CheckListenerChain()
{
  
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main thread!");
  return NS_OK;
}




NS_IMETHODIMP
imgRequest::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt,
                            nsIInputStream *inStr, uint64_t sourceOffset,
                            uint32_t count)
{
  LOG_SCOPE_WITH_PARAM(GetImgLog(), "imgRequest::OnDataAvailable", "count", count);

  NS_ASSERTION(aRequest, "imgRequest::OnDataAvailable -- no request!");

  nsresult rv;

  if (!mGotData || mResniffMimeType) {
    LOG_SCOPE(GetImgLog(), "imgRequest::OnDataAvailable |First time through... finding mimetype|");

    mGotData = true;

    
    
    bool resniffMimeType = mResniffMimeType;
    mResniffMimeType = false;

    mimetype_closure closure;
    nsAutoCString newType;
    closure.newType = &newType;

    


    uint32_t out;
    inStr->ReadSegments(sniff_mimetype_callback, &closure, count, &out);

    nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
    if (newType.IsEmpty()) {
      LOG_SCOPE(GetImgLog(), "imgRequest::OnDataAvailable |sniffing of mimetype failed|");

      rv = NS_ERROR_FAILURE;
      if (chan) {
        rv = chan->GetContentType(newType);
      }

      if (NS_FAILED(rv)) {
        PR_LOG(GetImgLog(), PR_LOG_ERROR,
               ("[this=%p] imgRequest::OnDataAvailable -- Content type unavailable from the channel\n",
                this));

        this->Cancel(NS_IMAGELIB_ERROR_FAILURE);

        return NS_BINDING_ABORTED;
      }

      LOG_MSG(GetImgLog(), "imgRequest::OnDataAvailable", "Got content type from the channel");
    }

    
    
    
    
    
    
    if (mContentType != newType || newType.EqualsLiteral(IMAGE_SVG_XML)) {
      mContentType = newType;

      
      
      
      if (resniffMimeType) {
        NS_ABORT_IF_FALSE(mIsMultiPartChannel, "Resniffing a non-multipart image");

        nsRefPtr<imgStatusTracker> freshTracker = new imgStatusTracker(nullptr);
        nsRefPtr<imgStatusTracker> oldStatusTracker = GetStatusTracker();
        freshTracker->AdoptConsumers(oldStatusTracker);
        mStatusTracker = freshTracker.forget();
      }

      SetProperties(chan);

      LOG_MSG_WITH_PARAM(GetImgLog(), "imgRequest::OnDataAvailable", "content type", mContentType.get());

      
      
      

      
      
      mImage = ImageFactory::CreateImage(aRequest, mStatusTracker, mContentType,
                                         mURI, mIsMultiPartChannel,
                                         static_cast<uint32_t>(mInnerWindowId));

      
      mStatusTracker = nullptr;

      
      
      nsRefPtr<imgStatusTracker> statusTracker = GetStatusTracker();
      statusTracker->OnDataAvailable();

      if (mImage->HasError() && !mIsMultiPartChannel) { 
        
        
        
        this->Cancel(NS_ERROR_FAILURE);
        return NS_BINDING_ABORTED;
      }

      NS_ABORT_IF_FALSE(statusTracker->HasImage(), "Status tracker should have an image!");
      NS_ABORT_IF_FALSE(mImage, "imgRequest should have an image!");

      if (mDecodeRequested)
        mImage->StartDecoding();
    }
  }

  
  rv = mImage->OnImageDataAvailable(aRequest, ctxt, inStr, sourceOffset, count);

  if (NS_FAILED(rv)) {
    PR_LOG(GetImgLog(), PR_LOG_WARNING,
           ("[this=%p] imgRequest::OnDataAvailable -- "
            "copy to RasterImage failed\n", this));
    this->Cancel(NS_IMAGELIB_ERROR_FAILURE);
    return NS_BINDING_ABORTED;
  }

  return NS_OK;
}

class SetPropertiesEvent : public nsRunnable
{
public:
  SetPropertiesEvent(imgRequest* aImgRequest, nsIChannel* aChan)
    : mImgRequest(aImgRequest)
    , mChan(aChan)
  {
    MOZ_ASSERT(!NS_IsMainThread(), "Should be created off the main thread");
    MOZ_ASSERT(aImgRequest, "aImgRequest cannot be null");
  }
  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread(), "Should run on the main thread only");
    MOZ_ASSERT(mImgRequest, "mImgRequest cannot be null");
    mImgRequest->SetProperties(mChan);
    return NS_OK;
  }
private:
  nsRefPtr<imgRequest> mImgRequest;
  nsCOMPtr<nsIChannel> mChan;
};

void
imgRequest::SetProperties(nsIChannel* aChan)
{
  
  
  if (!NS_IsMainThread()) {
    NS_DispatchToMainThread(new SetPropertiesEvent(this, aChan));
    return;
  }
  
  nsCOMPtr<nsISupportsCString> contentType(do_CreateInstance("@mozilla.org/supports-cstring;1"));
  if (contentType) {
    contentType->SetData(mContentType);
    mProperties->Set("type", contentType);
  }

  
  nsAutoCString disposition;
  if (aChan) {
    aChan->GetContentDispositionHeader(disposition);
  }
  if (!disposition.IsEmpty()) {
    nsCOMPtr<nsISupportsCString> contentDisposition(do_CreateInstance("@mozilla.org/supports-cstring;1"));
    if (contentDisposition) {
      contentDisposition->SetData(disposition);
      mProperties->Set("content-disposition", contentDisposition);
    }
  }
}

static NS_METHOD sniff_mimetype_callback(nsIInputStream* in,
                                         void* data,
                                         const char* fromRawSegment,
                                         uint32_t toOffset,
                                         uint32_t count,
                                         uint32_t *writeCount)
{
  mimetype_closure* closure = static_cast<mimetype_closure*>(data);

  NS_ASSERTION(closure, "closure is null!");

  if (count > 0)
    imgLoader::GetMimeTypeFromContent(fromRawSegment, count, *closure->newType);

  *writeCount = 0;
  return NS_ERROR_FAILURE;
}




NS_IMETHODIMP
imgRequest::GetInterface(const nsIID & aIID, void **aResult)
{
  if (!mPrevChannelSink || aIID.Equals(NS_GET_IID(nsIChannelEventSink)))
    return QueryInterface(aIID, aResult);

  NS_ASSERTION(mPrevChannelSink != this,
               "Infinite recursion - don't keep track of channel sinks that are us!");
  return mPrevChannelSink->GetInterface(aIID, aResult);
}


NS_IMETHODIMP
imgRequest::AsyncOnChannelRedirect(nsIChannel *oldChannel,
                                   nsIChannel *newChannel, uint32_t flags,
                                   nsIAsyncVerifyRedirectCallback *callback)
{
  NS_ASSERTION(mRequest && mChannel, "Got a channel redirect after we nulled out mRequest!");
  NS_ASSERTION(mChannel == oldChannel, "Got a channel redirect for an unknown channel!");
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

#if defined(PR_LOGGING)
  nsAutoCString oldspec;
  if (mCurrentURI)
    mCurrentURI->GetSpec(oldspec);
  LOG_MSG_WITH_PARAM(GetImgLog(), "imgRequest::OnChannelRedirect", "old", oldspec.get());
#endif

  
  
  mChannel->GetURI(getter_AddRefs(mCurrentURI));
  bool doesNotReturnData = false;
  nsresult rv =
    NS_URIChainHasFlags(mCurrentURI, nsIProtocolHandler::URI_DOES_NOT_RETURN_DATA,
                        &doesNotReturnData);

  if (NS_SUCCEEDED(rv) && doesNotReturnData)
    rv = NS_ERROR_ABORT;

  if (NS_FAILED(rv)) {
    mRedirectCallback->OnRedirectVerifyCallback(rv);
    mRedirectCallback = nullptr;
    return NS_OK;
  }

  mRedirectCallback->OnRedirectVerifyCallback(NS_OK);
  mRedirectCallback = nullptr;
  return NS_OK;
}
