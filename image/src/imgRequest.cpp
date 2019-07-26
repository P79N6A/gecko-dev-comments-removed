





#include "imgRequest.h"
#include "ImageLogging.h"







#undef LoadImage

#include "imgLoader.h"
#include "imgRequestProxy.h"
#include "RasterImage.h"
#include "VectorImage.h"

#include "imgILoader.h"

#include "netCore.h"

#include "nsIChannel.h"
#include "nsICachingChannel.h"
#include "nsILoadGroup.h"
#include "nsIInputStream.h"
#include "nsIMultiPartChannel.h"
#include "nsIHttpChannel.h"

#include "nsIComponentManager.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"
#include "nsIScriptSecurityManager.h"

#include "nsICacheVisitor.h"

#include "nsString.h"
#include "nsXPIDLString.h"
#include "plstr.h" 
#include "nsNetUtil.h"
#include "nsIProtocolHandler.h"

#include "mozilla/Preferences.h"

#include "DiscardTracker.h"
#include "nsAsyncRedirectVerifyHelper.h"

#define SVG_MIMETYPE "image/svg+xml"

using namespace mozilla;
using namespace mozilla::image;

static bool gInitializedPrefCaches = false;
static bool gDecodeOnDraw = false;
static bool gDiscardable = false;

static void
InitPrefCaches()
{
  Preferences::AddBoolVarCache(&gDiscardable, "image.mem.discardable");
  Preferences::AddBoolVarCache(&gDecodeOnDraw, "image.mem.decodeondraw");
  gInitializedPrefCaches = true;
}

#if defined(PR_LOGGING)
PRLogModuleInfo *gImgLog = PR_NewLogModule("imgRequest");
#endif

NS_IMPL_ISUPPORTS8(imgRequest,
                   imgIDecoderObserver, imgIContainerObserver,
                   nsIStreamListener, nsIRequestObserver,
                   nsISupportsWeakReference,
                   nsIChannelEventSink,
                   nsIInterfaceRequestor,
                   nsIAsyncVerifyRedirectCallback)

imgRequest::imgRequest(imgLoader* aLoader)
 : mLoader(aLoader)
 , mValidator(nullptr)
 , mImageSniffers("image-sniffing-services")
 , mInnerWindowId(0)
 , mCORSMode(imgIRequest::CORS_NONE)
 , mDecodeRequested(false)
 , mIsMultiPartChannel(false)
 , mGotData(false)
 , mIsInCache(false)
 , mBlockingOnload(false)
 , mResniffMimeType(false)
{
  
  if (NS_UNLIKELY(!gInitializedPrefCaches)) {
    InitPrefCaches();
  }
}

imgRequest::~imgRequest()
{
  if (mURI) {
    nsAutoCString spec;
    mURI->GetSpec(spec);
    LOG_FUNC_WITH_PARAM(gImgLog, "imgRequest::~imgRequest()", "keyuri", spec.get());
  } else
    LOG_FUNC(gImgLog, "imgRequest::~imgRequest()");
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
  LOG_FUNC(gImgLog, "imgRequest::Init");

  NS_ABORT_IF_FALSE(!mImage, "Multiple calls to init");
  NS_ABORT_IF_FALSE(aURI, "No uri");
  NS_ABORT_IF_FALSE(aCurrentURI, "No current uri");
  NS_ABORT_IF_FALSE(aRequest, "No request");
  NS_ABORT_IF_FALSE(aChannel, "No channel");

  mProperties = do_CreateInstance("@mozilla.org/properties;1");

  mStatusTracker = new imgStatusTracker(nullptr);

  mURI = aURI;
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

imgStatusTracker&
imgRequest::GetStatusTracker()
{
  if (mImage && mGotData) {
    NS_ABORT_IF_FALSE(!mStatusTracker,
                      "Should have given mStatusTracker to mImage");
    return mImage->GetStatusTracker();
  } else {
    NS_ABORT_IF_FALSE(mStatusTracker,
                      "Should have mStatusTracker until we create mImage");
    return *mStatusTracker;
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

void imgRequest::AddProxy(imgRequestProxy *proxy)
{
  NS_PRECONDITION(proxy, "null imgRequestProxy passed in");
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::AddProxy", "proxy", proxy);

  
  
  if (mObservers.IsEmpty()) {
    NS_ABORT_IF_FALSE(mURI, "Trying to SetHasProxies without key uri.");
    mLoader->SetHasProxies(mURI);
  }

  
  if (mImage && !HaveProxyWithObserver(proxy) && proxy->HasObserver()) {
    LOG_MSG(gImgLog, "imgRequest::AddProxy", "resetting animation");

    mImage->ResetAnimation();
  }

  proxy->SetPrincipal(mPrincipal);

  mObservers.AppendElementUnlessExists(proxy);
}

nsresult imgRequest::RemoveProxy(imgRequestProxy *proxy, nsresult aStatus)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::RemoveProxy", "proxy", proxy);

  
  
  
  proxy->ClearAnimationConsumers();

  if (!mObservers.RemoveElement(proxy)) {
    
    return NS_OK;
  }

  
  
  
  

  imgStatusTracker& statusTracker = GetStatusTracker();
  statusTracker.EmulateRequestFinished(proxy, aStatus);

  if (mObservers.IsEmpty()) {
    
    
    
    if (mCacheEntry) {
      NS_ABORT_IF_FALSE(mURI, "Removing last observer without key uri.");

      mLoader->SetHasNoProxies(mURI, mCacheEntry);
    } 
#if defined(PR_LOGGING)
    else {
      nsAutoCString spec;
      mURI->GetSpec(spec);
      LOG_MSG_WITH_PARAM(gImgLog, "imgRequest::RemoveProxy no cache entry", "uri", spec.get());
    }
#endif

    




    if (statusTracker.IsLoading() && NS_FAILED(aStatus)) {
      LOG_MSG(gImgLog, "imgRequest::RemoveProxy", "load in progress.  canceling");

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
  LOG_SCOPE(gImgLog, "imgRequest::CancelAndAbort");

  Cancel(aStatus);

  
  
  
  if (mChannel) {
    mChannel->SetNotificationCallbacks(mPrevChannelSink);
    mPrevChannelSink = nullptr;
  }
}

void imgRequest::Cancel(nsresult aStatus)
{
  

  LOG_SCOPE(gImgLog, "imgRequest::Cancel");

  imgStatusTracker& statusTracker = GetStatusTracker();

  if (mBlockingOnload) {
    mBlockingOnload = false;

    statusTracker.RecordUnblockOnload();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
    while (iter.HasMore()) {
      statusTracker.SendUnblockOnload(iter.GetNext());
    }
  }

  statusTracker.RecordCancel();

  RemoveFromCache();

  if (mRequest && statusTracker.IsLoading())
    mRequest->Cancel(aStatus);
}

nsresult imgRequest::GetURI(nsIURI **aURI)
{
  LOG_FUNC(gImgLog, "imgRequest::GetURI");

  if (mURI) {
    *aURI = mURI;
    NS_ADDREF(*aURI);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

nsresult imgRequest::GetSecurityInfo(nsISupports **aSecurityInfo)
{
  LOG_FUNC(gImgLog, "imgRequest::GetSecurityInfo");

  
  
  NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);
  return NS_OK;
}

void imgRequest::RemoveFromCache()
{
  LOG_SCOPE(gImgLog, "imgRequest::RemoveFromCache");

  if (mIsInCache) {
    
    if (mCacheEntry)
      mLoader->RemoveFromCache(mCacheEntry);
    else
      mLoader->RemoveFromCache(mURI);
  }

  mCacheEntry = nullptr;
}

bool imgRequest::HaveProxyWithObserver(imgRequestProxy* aProxyToIgnore) const
{
  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  imgRequestProxy* proxy;
  while (iter.HasMore()) {
    proxy = iter.GetNext();
    if (proxy == aProxyToIgnore) {
      continue;
    }
    
    if (proxy->HasObserver()) {
      return true;
    }
  }
  
  return false;
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
  
  
  
  
  
  
  
  if (mObservers.SafeElementAt(0, nullptr) != proxy)
    return;

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mRequest);
  if (p)
    p->AdjustPriority(delta);
}

void imgRequest::SetIsInCache(bool incache)
{
  LOG_FUNC_WITH_PARAM(gImgLog, "imgRequest::SetIsCacheable", "incache", incache);
  mIsInCache = incache;
}

void imgRequest::UpdateCacheEntrySize()
{
  if (mCacheEntry) {
    mCacheEntry->SetDataSize(mImage->SizeOfData());

#ifdef DEBUG_joe
    nsAutoCString url;
    mURI->GetSpec(url);
    printf("CACHEPUT: %d %s %d\n", time(NULL), url.get(), imageSize);
#endif
  }
}

void imgRequest::SetCacheValidation(imgCacheEntry* aCacheEntry, nsIRequest* aRequest)
{
  
  if (aCacheEntry) {
    nsCOMPtr<nsICachingChannel> cacheChannel(do_QueryInterface(aRequest));
    if (cacheChannel) {
      nsCOMPtr<nsISupports> cacheToken;
      cacheChannel->GetCacheToken(getter_AddRefs(cacheToken));
      if (cacheToken) {
        nsCOMPtr<nsICacheEntryInfo> entryDesc(do_QueryInterface(cacheToken));
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








NS_IMETHODIMP imgRequest::FrameChanged(imgIRequest *request,
                                       imgIContainer *container,
                                       const nsIntRect *dirtyRect)
{
  LOG_SCOPE(gImgLog, "imgRequest::FrameChanged");
  NS_ABORT_IF_FALSE(mImage,
                    "FrameChanged callback before we've created our image");

  mImage->GetStatusTracker().RecordFrameChanged(container, dirtyRect);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendFrameChanged(iter.GetNext(), container, dirtyRect);
  }

  return NS_OK;
}




NS_IMETHODIMP imgRequest::OnStartDecode(imgIRequest *request)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartDecode");
  NS_ABORT_IF_FALSE(mImage,
                    "OnStartDecode callback before we've created our image");


  imgStatusTracker& tracker = mImage->GetStatusTracker();
  tracker.RecordStartDecode();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    tracker.SendStartDecode(iter.GetNext());
  }

  if (!mIsMultiPartChannel) {
    MOZ_ASSERT(!mBlockingOnload);
    mBlockingOnload = true;

    tracker.RecordBlockOnload();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
    while (iter.HasMore()) {
      tracker.SendBlockOnload(iter.GetNext());
    }
  }

  




  if (mCacheEntry)
    mCacheEntry->SetDataSize(0);

  return NS_OK;
}

NS_IMETHODIMP imgRequest::OnStartRequest(imgIRequest *aRequest)
{
  NS_NOTREACHED("imgRequest(imgIDecoderObserver)::OnStartRequest");
  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStartContainer(imgIRequest *request, imgIContainer *image)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartContainer");

  NS_ASSERTION(image, "imgRequest::OnStartContainer called with a null image!");
  if (!image) return NS_ERROR_UNEXPECTED;

  NS_ABORT_IF_FALSE(mImage,
                    "OnStartContainer callback before we've created our image");
  NS_ABORT_IF_FALSE(image == mImage,
                    "OnStartContainer callback from an image we don't own");
  mImage->GetStatusTracker().RecordStartContainer(image);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStartContainer(iter.GetNext(), image);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStartFrame(imgIRequest *request,
                                       uint32_t frame)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartFrame");
  NS_ABORT_IF_FALSE(mImage,
                    "OnStartFrame callback before we've created our image");

  mImage->GetStatusTracker().RecordStartFrame(frame);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStartFrame(iter.GetNext(), frame);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnDataAvailable(imgIRequest *request,
                                          bool aCurrentFrame,
                                          const nsIntRect * rect)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable");
  NS_ABORT_IF_FALSE(mImage,
                    "OnDataAvailable callback before we've created our image");

  mImage->GetStatusTracker().RecordDataAvailable(aCurrentFrame, rect);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendDataAvailable(iter.GetNext(), aCurrentFrame, rect);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopFrame(imgIRequest *request,
                                      uint32_t frame)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopFrame");
  NS_ABORT_IF_FALSE(mImage,
                    "OnStopFrame callback before we've created our image");

  imgStatusTracker& tracker = mImage->GetStatusTracker();
  tracker.RecordStopFrame(frame);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    tracker.SendStopFrame(iter.GetNext(), frame);
  }

  if (mBlockingOnload) {
    mBlockingOnload = false;

    tracker.RecordUnblockOnload();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
    while (iter.HasMore()) {
      tracker.SendUnblockOnload(iter.GetNext());
    }
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopContainer(imgIRequest *request,
                                          imgIContainer *image)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopContainer");
  NS_ABORT_IF_FALSE(mImage,
                    "OnDataContainer callback before we've created our image");

  imgStatusTracker& tracker = mImage->GetStatusTracker();
  tracker.RecordStopContainer(image);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    tracker.SendStopContainer(iter.GetNext(), image);
  }

  
  
  
  
  
  
  if (mBlockingOnload) {
    mBlockingOnload = false;

    tracker.RecordUnblockOnload();

    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
    while (iter.HasMore()) {
      tracker.SendUnblockOnload(iter.GetNext());
    }
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopDecode(imgIRequest *aRequest,
                                       nsresult aStatus,
                                       const PRUnichar *aStatusArg)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopDecode");
  NS_ABORT_IF_FALSE(mImage,
                    "OnDataDecode callback before we've created our image");

  
  
  UpdateCacheEntrySize();

  mImage->GetStatusTracker().RecordStopDecode(aStatus, aStatusArg);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStopDecode(iter.GetNext(), aStatus,
                                              aStatusArg);
  }

  if (NS_FAILED(aStatus)) {
    
    

    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os)
      os->NotifyObservers(mURI, "net:failed-to-process-uri-content", nullptr);
  }

  
  
  
  
  
  
  
  
  
  
  
  

  return NS_OK;
}

NS_IMETHODIMP imgRequest::OnStopRequest(imgIRequest *aRequest,
                                        bool aLastPart)
{
  NS_NOTREACHED("imgRequest(imgIDecoderObserver)::OnStopRequest");
  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnDiscard(imgIRequest *aRequest)
{
  NS_ABORT_IF_FALSE(mImage,
                    "OnDiscard callback before we've created our image");

  mImage->GetStatusTracker().RecordDiscard();

  
  UpdateCacheEntrySize();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendDiscard(iter.GetNext());
  }

  return NS_OK;
}

NS_IMETHODIMP imgRequest::OnImageIsAnimated(imgIRequest *aRequest)
{
  NS_ABORT_IF_FALSE(mImage,
                    "OnImageIsAnimated callback before we've created our image");
  mImage->GetStatusTracker().RecordImageIsAnimated();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendImageIsAnimated(iter.GetNext());
  }

  return NS_OK;
}




NS_IMETHODIMP imgRequest::OnStartRequest(nsIRequest *aRequest, nsISupports *ctxt)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartRequest");

  
  nsCOMPtr<nsIMultiPartChannel> mpchan(do_QueryInterface(aRequest));
  if (mpchan)
      mIsMultiPartChannel = true;

  
  NS_ABORT_IF_FALSE(mIsMultiPartChannel || !mImage,
                    "Already have an image for non-multipart request");

  
  
  if (mIsMultiPartChannel && mImage) {
    mResniffMimeType = true;
    if (mImage->GetType() == imgIContainer::TYPE_RASTER) {
        
        
        
        
        
        static_cast<RasterImage*>(mImage.get())->NewSourceData();
      }
  }

  






  if (!mRequest) {
    NS_ASSERTION(mpchan,
                 "We should have an mRequest here unless we're multipart");
    nsCOMPtr<nsIChannel> chan;
    mpchan->GetBaseChannel(getter_AddRefs(chan));
    mRequest = chan;
  }

  imgStatusTracker& statusTracker = GetStatusTracker();
  statusTracker.RecordStartRequest();

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel)
    channel->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    statusTracker.SendStartRequest(iter.GetNext());
  }

  
  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
  if (chan) {
    nsCOMPtr<nsIScriptSecurityManager> secMan =
      do_GetService("@mozilla.org/scriptsecuritymanager;1");
    if (secMan) {
      nsresult rv = secMan->GetChannelPrincipal(chan,
                                                getter_AddRefs(mPrincipal));
      if (NS_FAILED(rv)) {
        return rv;
      }

      
      nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
      while (iter.HasMore()) {
        iter.GetNext()->SetPrincipal(mPrincipal);
      }
    }
  }

  SetCacheValidation(mCacheEntry, aRequest);

  
  if (mObservers.IsEmpty()) {
    this->Cancel(NS_IMAGELIB_ERROR_FAILURE);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopRequest(nsIRequest *aRequest, nsISupports *ctxt, nsresult status)
{
  LOG_FUNC(gImgLog, "imgRequest::OnStopRequest");

  bool lastPart = true;
  nsCOMPtr<nsIMultiPartChannel> mpchan(do_QueryInterface(aRequest));
  if (mpchan)
    mpchan->GetIsLastPart(&lastPart);

  
  
  
  
  if (mRequest) {
    mRequest = nullptr;  
  }

  
  if (mChannel) {
    mChannel->SetNotificationCallbacks(mPrevChannelSink);
    mPrevChannelSink = nullptr;
    mChannel = nullptr;
  }

  
  
  
  if (mImage) {
    nsresult rv;
    if (mImage->GetType() == imgIContainer::TYPE_RASTER) {
      
      rv = static_cast<RasterImage*>(mImage.get())->SourceDataComplete();
    } else { 
      nsCOMPtr<nsIStreamListener> imageAsStream = do_QueryInterface(mImage);
      NS_ABORT_IF_FALSE(imageAsStream,
                        "SVG-typed Image failed QI to nsIStreamListener");
      rv = imageAsStream->OnStopRequest(aRequest, ctxt, status);
    }

    
    
    
    
    if (NS_FAILED(rv) && NS_SUCCEEDED(status))
      status = rv;
  }

  imgStatusTracker& statusTracker = GetStatusTracker();
  statusTracker.RecordStopRequest(lastPart, status);

  
  
  if (mImage && NS_SUCCEEDED(status)) {
    
    
    UpdateCacheEntrySize();
  }
  else {
    
    this->Cancel(status);
  }

  
  nsTObserverArray<imgRequestProxy*>::ForwardIterator srIter(mObservers);
  while (srIter.HasMore()) {
    statusTracker.SendStopRequest(srIter.GetNext(), lastPart, status);
  }

  mTimedChannel = nullptr;
  return NS_OK;
}

struct mimetype_closure
{
  imgRequest* request;
  nsACString* newType;
};


static NS_METHOD sniff_mimetype_callback(nsIInputStream* in, void* closure, const char* fromRawSegment,
                                         uint32_t toOffset, uint32_t count, uint32_t *writeCount);




NS_IMETHODIMP
imgRequest::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt,
                            nsIInputStream *inStr, uint64_t sourceOffset,
                            uint32_t count)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::OnDataAvailable", "count", count);

  NS_ASSERTION(aRequest, "imgRequest::OnDataAvailable -- no request!");

  nsresult rv;

  if (!mGotData || mResniffMimeType) {
    LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable |First time through... finding mimetype|");

    mGotData = true;

    mimetype_closure closure;
    nsAutoCString newType;
    closure.request = this;
    closure.newType = &newType;

    


    uint32_t out;
    inStr->ReadSegments(sniff_mimetype_callback, &closure, count, &out);

#ifdef DEBUG
    
#endif

    nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));
    if (newType.IsEmpty()) {
      LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable |sniffing of mimetype failed|");

      rv = NS_ERROR_FAILURE;
      if (chan) {
        rv = chan->GetContentType(newType);
      }

      if (NS_FAILED(rv)) {
        PR_LOG(gImgLog, PR_LOG_ERROR,
               ("[this=%p] imgRequest::OnDataAvailable -- Content type unavailable from the channel\n",
                this));

        this->Cancel(NS_IMAGELIB_ERROR_FAILURE);

        return NS_BINDING_ABORTED;
      }

      LOG_MSG(gImgLog, "imgRequest::OnDataAvailable", "Got content type from the channel");
    }

    
    
    
    
    
    
    if (mContentType != newType || newType.EqualsLiteral(SVG_MIMETYPE)) {
      mContentType = newType;

      
      
      
      if (mResniffMimeType) {
        NS_ABORT_IF_FALSE(mIsMultiPartChannel, "Resniffing a non-multipart image");
        mStatusTracker = new imgStatusTracker(nullptr);
      }

      mResniffMimeType = false;

      
      if (mContentType.EqualsLiteral(SVG_MIMETYPE)) {
        mImage = new VectorImage(mStatusTracker.forget());
      } else {
        mImage = new RasterImage(mStatusTracker.forget());
      }
      mImage->SetInnerWindowID(mInnerWindowId);

      
      nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
      while (iter.HasMore()) {
        iter.GetNext()->SetImage(mImage);
      }

      
      nsCOMPtr<nsISupportsCString> contentType(do_CreateInstance("@mozilla.org/supports-cstring;1"));
      if (contentType) {
        contentType->SetData(mContentType);
        mProperties->Set("type", contentType);
      }

      
      nsAutoCString disposition;
      if (chan) {
        chan->GetContentDispositionHeader(disposition);
      }
      if (!disposition.IsEmpty()) {
        nsCOMPtr<nsISupportsCString> contentDisposition(do_CreateInstance("@mozilla.org/supports-cstring;1"));
        if (contentDisposition) {
          contentDisposition->SetData(disposition);
          mProperties->Set("content-disposition", contentDisposition);
        }
      }

      LOG_MSG_WITH_PARAM(gImgLog, "imgRequest::OnDataAvailable", "content type", mContentType.get());

      
      
      

      
      bool isDiscardable = gDiscardable;
      bool doDecodeOnDraw = gDecodeOnDraw;

      
      
      bool isChrome = false;
      rv = mURI->SchemeIs("chrome", &isChrome);
      if (NS_SUCCEEDED(rv) && isChrome)
        isDiscardable = doDecodeOnDraw = false;

      
      
      bool isResource = false;
      rv = mURI->SchemeIs("resource", &isResource);
      if (NS_SUCCEEDED(rv) && isResource)
        isDiscardable = doDecodeOnDraw = false;

      
      
      if (mIsMultiPartChannel)
        isDiscardable = doDecodeOnDraw = false;

      
      uint32_t imageFlags = Image::INIT_FLAG_NONE;
      if (isDiscardable)
        imageFlags |= Image::INIT_FLAG_DISCARDABLE;
      if (doDecodeOnDraw)
        imageFlags |= Image::INIT_FLAG_DECODE_ON_DRAW;
      if (mIsMultiPartChannel)
        imageFlags |= Image::INIT_FLAG_MULTIPART;

      
      nsAutoCString uriString;
      rv = mURI->GetSpec(uriString);
      if (NS_FAILED(rv))
        uriString.Assign("<unknown image URI>");

      
      
      
      rv = mImage->Init(this, mContentType.get(), uriString.get(), imageFlags);

      
      
      if (NS_FAILED(rv) && !mIsMultiPartChannel) { 

        this->Cancel(rv);
        return NS_BINDING_ABORTED;
      }

      if (mImage->GetType() == imgIContainer::TYPE_RASTER) {
        
        nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
        if (httpChannel) {
          nsAutoCString contentLength;
          rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("content-length"),
                                              contentLength);
          if (NS_SUCCEEDED(rv)) {
            int32_t len = contentLength.ToInteger(&rv);

            
            
            if (len > 0) {
              uint32_t sizeHint = (uint32_t) len;
              sizeHint = NS_MIN<uint32_t>(sizeHint, 20000000); 
              RasterImage* rasterImage = static_cast<RasterImage*>(mImage.get());
              rv = rasterImage->SetSourceSizeHint(sizeHint);
              if (NS_FAILED(rv)) {
                
                rv = nsMemory::HeapMinimize(true);
                nsresult rv2 = rasterImage->SetSourceSizeHint(sizeHint);
                
                if (NS_FAILED(rv) || NS_FAILED(rv2)) {
                  NS_WARNING("About to hit OOM in imagelib!");
                }
              }
            }
          }
        }
      }

      if (mImage->GetType() == imgIContainer::TYPE_RASTER) {
        
        if (mDecodeRequested) {
          mImage->StartDecoding();
        }
      } else { 
        nsCOMPtr<nsIStreamListener> imageAsStream = do_QueryInterface(mImage);
        NS_ABORT_IF_FALSE(imageAsStream,
                          "SVG-typed Image failed QI to nsIStreamListener");
        imageAsStream->OnStartRequest(aRequest, nullptr);
      }
    }
  }

  if (mImage->GetType() == imgIContainer::TYPE_RASTER) {
    
    
    uint32_t bytesRead;
    rv = inStr->ReadSegments(RasterImage::WriteToRasterImage,
                             static_cast<void*>(mImage),
                             count, &bytesRead);
    NS_ABORT_IF_FALSE(bytesRead == count || mImage->HasError(),
  "WriteToRasterImage should consume everything or the image must be in error!");
  } else { 
    nsCOMPtr<nsIStreamListener> imageAsStream = do_QueryInterface(mImage);
    rv = imageAsStream->OnDataAvailable(aRequest, ctxt, inStr,
                                        sourceOffset, count);
  }
  if (NS_FAILED(rv)) {
    PR_LOG(gImgLog, PR_LOG_WARNING,
           ("[this=%p] imgRequest::OnDataAvailable -- "
            "copy to RasterImage failed\n", this));
    this->Cancel(NS_IMAGELIB_ERROR_FAILURE);
    return NS_BINDING_ABORTED;
  }

  return NS_OK;
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
    closure->request->SniffMimeType(fromRawSegment, count, *closure->newType);

  *writeCount = 0;
  return NS_ERROR_FAILURE;
}

void
imgRequest::SniffMimeType(const char *buf, uint32_t len, nsACString& newType)
{
  imgLoader::GetMimeTypeFromContent(buf, len, newType);

  
  
  if (!newType.IsEmpty())
    return;

  
  
  
  const nsCOMArray<nsIContentSniffer>& sniffers = mImageSniffers.GetEntries();
  uint32_t length = sniffers.Count();
  for (uint32_t i = 0; i < length; ++i) {
    nsresult rv =
      sniffers[i]->GetMIMETypeFromContent(nullptr, (const uint8_t *) buf, len, newType);
    if (NS_SUCCEEDED(rv) && !newType.IsEmpty()) {
      return;
    }
  }
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
  LOG_MSG_WITH_PARAM(gImgLog, "imgRequest::OnChannelRedirect", "old", oldspec.get());
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
