







































#include "imgRequest.h"







#undef LoadImage

#include "imgLoader.h"
#include "imgRequestProxy.h"
#include "imgContainer.h"

#include "imgILoader.h"
#include "ImageLogging.h"

#include "netCore.h"

#include "nsIChannel.h"
#include "nsICachingChannel.h"
#include "nsILoadGroup.h"
#include "nsIInputStream.h"
#include "nsIMultiPartChannel.h"
#include "nsIHttpChannel.h"

#include "nsIComponentManager.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"
#include "nsIScriptSecurityManager.h"

#include "nsICacheVisitor.h"

#include "nsString.h"
#include "nsXPIDLString.h"
#include "plstr.h" 
#include "nsNetUtil.h"
#include "nsIProtocolHandler.h"

#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"

#include "DiscardTracker.h"
#include "nsAsyncRedirectVerifyHelper.h"

#define DISCARD_PREF "image.mem.discardable"
#define DECODEONDRAW_PREF "image.mem.decodeondraw"

using namespace mozilla::imagelib;


static PRBool gDecodeOnDraw = PR_FALSE;
static PRBool gDiscardable = PR_FALSE;






static PRBool gRegisteredPrefObserver = PR_FALSE;


static void
ReloadPrefs(nsIPrefBranch *aBranch)
{
  
  PRBool discardable;
  nsresult rv = aBranch->GetBoolPref(DISCARD_PREF, &discardable);
  if (NS_SUCCEEDED(rv))
    gDiscardable = discardable;

  
  PRBool decodeondraw;
  rv = aBranch->GetBoolPref(DECODEONDRAW_PREF, &decodeondraw);
  if (NS_SUCCEEDED(rv))
    gDecodeOnDraw = decodeondraw;

  
  mozilla::imagelib::DiscardTracker::ReloadTimeout();
}


class imgRequestPrefObserver : public nsIObserver {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};
NS_IMPL_ISUPPORTS1(imgRequestPrefObserver, nsIObserver)


NS_IMETHODIMP
imgRequestPrefObserver::Observe(nsISupports     *aSubject,
                                const char      *aTopic,
                                const PRUnichar *aData)
{
  
  NS_ABORT_IF_FALSE(!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID), "invalid topic");

  
  if (strcmp(NS_LossyConvertUTF16toASCII(aData).get(), DISCARD_PREF) &&
      strcmp(NS_LossyConvertUTF16toASCII(aData).get(), DECODEONDRAW_PREF) &&
      strcmp(NS_LossyConvertUTF16toASCII(aData).get(), DISCARD_TIMEOUT_PREF))
    return NS_OK;

  
  nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(aSubject);
  if (!branch) {
    NS_WARNING("Couldn't get pref branch within imgRequestPrefObserver::Observe!");
    return NS_OK;
  }

  
  ReloadPrefs(branch);

  return NS_OK;
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

imgRequest::imgRequest() : 
  mCacheId(0), mValidator(nsnull), mImageSniffers("image-sniffing-services"),
  mDecodeRequested(PR_FALSE), mIsMultiPartChannel(PR_FALSE),
  mGotData(PR_FALSE), mIsInCache(PR_FALSE)
{}

imgRequest::~imgRequest()
{
  if (mKeyURI) {
    nsCAutoString spec;
    mKeyURI->GetSpec(spec);
    LOG_FUNC_WITH_PARAM(gImgLog, "imgRequest::~imgRequest()", "keyuri", spec.get());
  } else
    LOG_FUNC(gImgLog, "imgRequest::~imgRequest()");
}

nsresult imgRequest::Init(nsIURI *aURI,
                          nsIURI *aKeyURI,
                          nsIRequest *aRequest,
                          nsIChannel *aChannel,
                          imgCacheEntry *aCacheEntry,
                          void *aCacheId,
                          void *aLoadId)
{
  LOG_FUNC(gImgLog, "imgRequest::Init");

  NS_ABORT_IF_FALSE(!mImage, "Multiple calls to init");
  NS_ABORT_IF_FALSE(aURI, "No uri");
  NS_ABORT_IF_FALSE(aKeyURI, "No key uri");
  NS_ABORT_IF_FALSE(aRequest, "No request");
  NS_ABORT_IF_FALSE(aChannel, "No channel");

  mProperties = do_CreateInstance("@mozilla.org/properties;1");
  nsCOMPtr<imgIContainer> comImg = do_CreateInstance("@mozilla.org/image/rasterimage;1");
  mImage = static_cast<Image*>(comImg.get());

  mURI = aURI;
  mKeyURI = aKeyURI;
  mRequest = aRequest;
  mChannel = aChannel;
  mChannel->GetNotificationCallbacks(getter_AddRefs(mPrevChannelSink));

  NS_ASSERTION(mPrevChannelSink != this,
               "Initializing with a channel that already calls back to us!");

  mChannel->SetNotificationCallbacks(this);

  mCacheEntry = aCacheEntry;

  mCacheId = aCacheId;

  SetLoadId(aLoadId);

  
  if (NS_UNLIKELY(!gRegisteredPrefObserver)) {
    imgRequestPrefObserver *observer = new imgRequestPrefObserver();
    if (observer) {
      nsCOMPtr<nsIPrefBranch2> branch = do_GetService(NS_PREFSERVICE_CONTRACTID);
      if (branch) {
        branch->AddObserver(DISCARD_PREF, observer, PR_FALSE);
        branch->AddObserver(DECODEONDRAW_PREF, observer, PR_FALSE);
        branch->AddObserver(DISCARD_TIMEOUT_PREF, observer, PR_FALSE);
        ReloadPrefs(branch);
        gRegisteredPrefObserver = PR_TRUE;
      }
    }
    else
      delete observer;
  }

  return NS_OK;
}

void imgRequest::SetCacheEntry(imgCacheEntry *entry)
{
  mCacheEntry = entry;
}

PRBool imgRequest::HasCacheEntry() const
{
  return mCacheEntry != nsnull;
}

nsresult imgRequest::AddProxy(imgRequestProxy *proxy)
{
  NS_PRECONDITION(proxy, "null imgRequestProxy passed in");
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::AddProxy", "proxy", proxy);

  
  
  if (mObservers.IsEmpty()) {
    NS_ABORT_IF_FALSE(mKeyURI, "Trying to SetHasProxies without key uri.");
    imgLoader::SetHasProxies(mKeyURI);
  }

  
  if (!HaveProxyWithObserver(proxy) && proxy->HasObserver()) {
    LOG_MSG(gImgLog, "imgRequest::AddProxy", "resetting animation");

    mImage->ResetAnimation();
  }

  proxy->SetPrincipal(mPrincipal);

  return mObservers.AppendElementUnlessExists(proxy) ?
    NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult imgRequest::RemoveProxy(imgRequestProxy *proxy, nsresult aStatus, PRBool aNotify)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::RemoveProxy", "proxy", proxy);

  mObservers.RemoveElement(proxy);

  
  
  
  

  mImage->GetStatusTracker().EmulateRequestFinished(proxy, aStatus, !aNotify);

  if (!HaveProxyWithObserver(nsnull)) {
    LOG_MSG(gImgLog, "imgRequest::RemoveProxy", "stopping animation");

    mImage->StopAnimation();
  }

  if (mObservers.IsEmpty()) {
    
    
    
    if (mCacheEntry) {
      NS_ABORT_IF_FALSE(mKeyURI, "Removing last observer without key uri.");

      imgLoader::SetHasNoProxies(mKeyURI, mCacheEntry);
    } 
#if defined(PR_LOGGING)
    else {
      nsCAutoString spec;
      mKeyURI->GetSpec(spec);
      LOG_MSG_WITH_PARAM(gImgLog, "imgRequest::RemoveProxy no cache entry", "uri", spec.get());
    }
#endif

    




    if (mImage->GetStatusTracker().IsLoading() && NS_FAILED(aStatus)) {
      LOG_MSG(gImgLog, "imgRequest::RemoveProxy", "load in progress.  canceling");

      this->Cancel(NS_BINDING_ABORTED);
    }

    
    mCacheEntry = nsnull;
  }

  
  
  if (aStatus != NS_IMAGELIB_CHANGING_OWNER)
    proxy->RemoveFromLoadGroup(PR_TRUE);

  return NS_OK;
}

PRBool imgRequest::IsReusable(void *aCacheId)
{
  return (mImage && mImage->GetStatusTracker().IsLoading()) ||
    (aCacheId == mCacheId);
}

void imgRequest::CancelAndAbort(nsresult aStatus)
{
  LOG_SCOPE(gImgLog, "imgRequest::CancelAndAbort");

  Cancel(aStatus);

  
  
  
  if (mChannel) {
    mChannel->SetNotificationCallbacks(mPrevChannelSink);
    mPrevChannelSink = nsnull;
  }
}

void imgRequest::Cancel(nsresult aStatus)
{
  

  LOG_SCOPE(gImgLog, "imgRequest::Cancel");

  LOG_MSG(gImgLog, "imgRequest::Cancel", "stopping animation");
  mImage->StopAnimation();

  mImage->GetStatusTracker().RecordCancel();

  RemoveFromCache();

  if (mRequest && mImage->GetStatusTracker().IsLoading())
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

nsresult imgRequest::GetKeyURI(nsIURI **aKeyURI)
{
  LOG_FUNC(gImgLog, "imgRequest::GetKeyURI");

  if (mKeyURI) {
    *aKeyURI = mKeyURI;
    NS_ADDREF(*aKeyURI);
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
      imgLoader::RemoveFromCache(mCacheEntry);
    else
      imgLoader::RemoveFromCache(mKeyURI);
  }

  mCacheEntry = nsnull;
}

PRBool imgRequest::HaveProxyWithObserver(imgRequestProxy* aProxyToIgnore) const
{
  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  imgRequestProxy* proxy;
  while (iter.HasMore()) {
    proxy = iter.GetNext();
    if (proxy == aProxyToIgnore) {
      continue;
    }
    
    if (proxy->HasObserver()) {
      return PR_TRUE;
    }
  }
  
  return PR_FALSE;
}

PRInt32 imgRequest::Priority() const
{
  PRInt32 priority = nsISupportsPriority::PRIORITY_NORMAL;
  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mRequest);
  if (p)
    p->GetPriority(&priority);
  return priority;
}

void imgRequest::AdjustPriority(imgRequestProxy *proxy, PRInt32 delta)
{
  
  
  
  
  
  
  
  if (mObservers.SafeElementAt(0, nsnull) != proxy)
    return;

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mRequest);
  if (p)
    p->AdjustPriority(delta);
}

void imgRequest::SetIsInCache(PRBool incache)
{
  LOG_FUNC_WITH_PARAM(gImgLog, "imgRequest::SetIsCacheable", "incache", incache);
  mIsInCache = incache;
}

void imgRequest::UpdateCacheEntrySize()
{
  if (mCacheEntry) {
    PRUint32 imageSize = 0;
    mImage->GetDataSize(&imageSize);
    mCacheEntry->SetDataSize(imageSize);

#ifdef DEBUG_joe
    nsCAutoString url;
    mURI->GetSpec(url);
    printf("CACHEPUT: %d %s %d\n", time(NULL), url.get(), imageSize);
#endif
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
  
  if (mImage->IsInitialized()) {
    return mImage->RequestDecode();
  }

  
  mDecodeRequested = PR_TRUE;

  return NS_OK;
}




NS_IMETHODIMP imgRequest::FrameChanged(imgIContainer *container,
                                       const nsIntRect *dirtyRect)
{
  LOG_SCOPE(gImgLog, "imgRequest::FrameChanged");

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

  mImage->GetStatusTracker().RecordStartDecode();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStartDecode(iter.GetNext());
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

  mImage->GetStatusTracker().RecordStartContainer(image);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStartContainer(iter.GetNext(), image);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStartFrame(imgIRequest *request,
                                       PRUint32 frame)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartFrame");

  mImage->GetStatusTracker().RecordStartFrame(frame);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStartFrame(iter.GetNext(), frame);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnDataAvailable(imgIRequest *request,
                                          PRBool aCurrentFrame,
                                          const nsIntRect * rect)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable");

  mImage->GetStatusTracker().RecordDataAvailable(aCurrentFrame, rect);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendDataAvailable(iter.GetNext(), aCurrentFrame, rect);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopFrame(imgIRequest *request,
                                      PRUint32 frame)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopFrame");

  mImage->GetStatusTracker().RecordStopFrame(frame);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStopFrame(iter.GetNext(), frame);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopContainer(imgIRequest *request,
                                          imgIContainer *image)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopContainer");

  mImage->GetStatusTracker().RecordStopContainer(image);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStopContainer(iter.GetNext(), image);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopDecode(imgIRequest *aRequest,
                                       nsresult aStatus,
                                       const PRUnichar *aStatusArg)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopDecode");

  
  
  UpdateCacheEntrySize();

  mImage->GetStatusTracker().RecordStopDecode(aStatus, aStatusArg);

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStopDecode(iter.GetNext(), aStatus,
                                              aStatusArg);
  }

  
  
  
  
  
  
  
  
  
  
  
  

  return NS_OK;
}

NS_IMETHODIMP imgRequest::OnStopRequest(imgIRequest *aRequest,
                                        PRBool aLastPart)
{
  NS_NOTREACHED("imgRequest(imgIDecoderObserver)::OnStopRequest");
  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnDiscard(imgIRequest *aRequest)
{
  mImage->GetStatusTracker().RecordDiscard();

  
  UpdateCacheEntrySize();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendDiscard(iter.GetNext());
  }

  return NS_OK;
}




NS_IMETHODIMP imgRequest::OnStartRequest(nsIRequest *aRequest, nsISupports *ctxt)
{
  nsresult rv;

  LOG_SCOPE(gImgLog, "imgRequest::OnStartRequest");

  
  nsCOMPtr<nsIMultiPartChannel> mpchan(do_QueryInterface(aRequest));
  if (mpchan)
      mIsMultiPartChannel = PR_TRUE;

  
  NS_ABORT_IF_FALSE(mIsMultiPartChannel || !mImage->IsInitialized(),
                    "Already have an image for non-multipart request");

  
  if (mIsMultiPartChannel && mImage->IsInitialized()) {

    
    mImage->NewSourceData();
  }

  






  if (!mRequest) {
    NS_ASSERTION(mpchan,
                 "We should have an mRequest here unless we're multipart");
    nsCOMPtr<nsIChannel> chan;
    mpchan->GetBaseChannel(getter_AddRefs(chan));
    mRequest = chan;
  }

  mImage->GetStatusTracker().RecordStartRequest();

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel)
    channel->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    mImage->GetStatusTracker().SendStartRequest(iter.GetNext());
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

  
  if (mCacheEntry) {
    nsCOMPtr<nsICachingChannel> cacheChannel(do_QueryInterface(aRequest));
    if (cacheChannel) {
      nsCOMPtr<nsISupports> cacheToken;
      cacheChannel->GetCacheToken(getter_AddRefs(cacheToken));
      if (cacheToken) {
        nsCOMPtr<nsICacheEntryInfo> entryDesc(do_QueryInterface(cacheToken));
        if (entryDesc) {
          PRUint32 expiration;
          
          entryDesc->GetExpirationTime(&expiration);

          
          mCacheEntry->SetExpiryTime(expiration);
        }
      }
    }
    
    
    
    
    
    
    
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
    if (httpChannel) {
      PRBool bMustRevalidate = PR_FALSE;

      rv = httpChannel->IsNoStoreResponse(&bMustRevalidate);

      if (!bMustRevalidate) {
        rv = httpChannel->IsNoCacheResponse(&bMustRevalidate);
      }

      if (!bMustRevalidate) {
        nsCAutoString cacheHeader;

        rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Cache-Control"),
                                            cacheHeader);
        if (PL_strcasestr(cacheHeader.get(), "must-revalidate")) {
          bMustRevalidate = PR_TRUE;
        }
      }

      mCacheEntry->SetMustValidateIfExpired(bMustRevalidate);
    }
  }


  
  if (mObservers.IsEmpty()) {
    this->Cancel(NS_IMAGELIB_ERROR_FAILURE);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopRequest(nsIRequest *aRequest, nsISupports *ctxt, nsresult status)
{
  LOG_FUNC(gImgLog, "imgRequest::OnStopRequest");

  PRBool lastPart = PR_TRUE;
  nsCOMPtr<nsIMultiPartChannel> mpchan(do_QueryInterface(aRequest));
  if (mpchan)
    mpchan->GetIsLastPart(&lastPart);

  
  
  
  
  if (mRequest) {
    mRequest = nsnull;  
  }

  
  if (mChannel) {
    mChannel->SetNotificationCallbacks(mPrevChannelSink);
    mPrevChannelSink = nsnull;
    mChannel = nsnull;
  }

  
  
  
  if (mImage->IsInitialized()) {

    
    nsresult rv = mImage->SourceDataComplete();

    
    
    
    
    if (NS_FAILED(rv) && NS_SUCCEEDED(status))
      status = rv;
  }

  mImage->GetStatusTracker().RecordStopRequest(lastPart, status);

  
  
  if (mImage->IsInitialized() && NS_SUCCEEDED(status)) {
    
    
    UpdateCacheEntrySize();
  }
  else {
    
    this->Cancel(status);
  }

  
  nsTObserverArray<imgRequestProxy*>::ForwardIterator srIter(mObservers);
  while (srIter.HasMore()) {
    mImage->GetStatusTracker().SendStopRequest(srIter.GetNext(), lastPart, status);
  }

  return NS_OK;
}


static NS_METHOD sniff_mimetype_callback(nsIInputStream* in, void* closure, const char* fromRawSegment,
                                         PRUint32 toOffset, PRUint32 count, PRUint32 *writeCount);




NS_IMETHODIMP imgRequest::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::OnDataAvailable", "count", count);

  NS_ASSERTION(aRequest, "imgRequest::OnDataAvailable -- no request!");

  nsresult rv;

  if (!mGotData) {
    LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable |First time through... finding mimetype|");

    mGotData = PR_TRUE;

    


    PRUint32 out;
    inStr->ReadSegments(sniff_mimetype_callback, this, count, &out);

#ifdef NS_DEBUG
    
#endif

    if (mContentType.IsEmpty()) {
      LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable |sniffing of mimetype failed|");

      nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));

      rv = NS_ERROR_FAILURE;
      if (chan) {
        rv = chan->GetContentType(mContentType);
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

    
    nsCOMPtr<nsISupportsCString> contentType(do_CreateInstance("@mozilla.org/supports-cstring;1"));
    if (contentType) {
      contentType->SetData(mContentType);
      mProperties->Set("type", contentType);
    }

    
    nsCAutoString disposition;
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
    if (httpChannel) {
      httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("content-disposition"), disposition);
    } else {
      nsCOMPtr<nsIMultiPartChannel> multiPartChannel(do_QueryInterface(aRequest));
      if (multiPartChannel) {
        multiPartChannel->GetContentDisposition(disposition);
      }
    }
    if (!disposition.IsEmpty()) {
      nsCOMPtr<nsISupportsCString> contentDisposition(do_CreateInstance("@mozilla.org/supports-cstring;1"));
      if (contentDisposition) {
        contentDisposition->SetData(disposition);
        mProperties->Set("content-disposition", contentDisposition);
      }
    }

    LOG_MSG_WITH_PARAM(gImgLog, "imgRequest::OnDataAvailable", "content type", mContentType.get());

    
    
    

    
    PRBool isDiscardable = gDiscardable;
    PRBool doDecodeOnDraw = gDecodeOnDraw;

    
    
    PRBool isChrome = PR_FALSE;
    rv = mURI->SchemeIs("chrome", &isChrome);
    if (NS_SUCCEEDED(rv) && isChrome)
      isDiscardable = doDecodeOnDraw = PR_FALSE;

    
    
    PRBool isResource = PR_FALSE;
    rv = mURI->SchemeIs("resource", &isResource);
    if (NS_SUCCEEDED(rv) && isResource)
      isDiscardable = doDecodeOnDraw = PR_FALSE;

    
    
    if (mIsMultiPartChannel)
      isDiscardable = doDecodeOnDraw = PR_FALSE;

    
    PRUint32 containerFlags = imgIContainer::INIT_FLAG_NONE;
    if (isDiscardable)
      containerFlags |= imgIContainer::INIT_FLAG_DISCARDABLE;
    if (doDecodeOnDraw)
      containerFlags |= imgIContainer::INIT_FLAG_DECODE_ON_DRAW;
    if (mIsMultiPartChannel)
      containerFlags |= imgIContainer::INIT_FLAG_MULTIPART;

    
    
    
    rv = mImage->Init(this, mContentType.get(), containerFlags);
    if (NS_FAILED(rv)) { 

      this->Cancel(rv);
      return NS_BINDING_ABORTED;
    }

    
    if (httpChannel) {
      nsCAutoString contentLength;
      rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("content-length"),
                                          contentLength);
      if (NS_SUCCEEDED(rv)) {
        PRInt32 len = contentLength.ToInteger(&rv);

        
        
        if (len > 0) {
          PRUint32 sizeHint = (PRUint32) len;
          sizeHint = PR_MIN(sizeHint, 20000000); 
          mImage->SetSourceSizeHint(sizeHint);
        }
      }
    }

    
    if (mDecodeRequested) {
      mImage->RequestDecode();
    }
  }

  
  PRUint32 bytesRead;
  rv = inStr->ReadSegments(RasterImage::WriteToContainer,
                           static_cast<void*>(mImage),
                           count, &bytesRead);
  if (NS_FAILED(rv)) {
    PR_LOG(gImgLog, PR_LOG_WARNING,
           ("[this=%p] imgRequest::OnDataAvailable -- "
            "copy to container failed\n", this));
    this->Cancel(NS_IMAGELIB_ERROR_FAILURE);
    return NS_BINDING_ABORTED;
  }
  NS_ABORT_IF_FALSE(bytesRead == count, "WriteToContainer should consume everything!");

  return NS_OK;
}

static NS_METHOD sniff_mimetype_callback(nsIInputStream* in,
                                         void* closure,
                                         const char* fromRawSegment,
                                         PRUint32 toOffset,
                                         PRUint32 count,
                                         PRUint32 *writeCount)
{
  imgRequest *request = static_cast<imgRequest*>(closure);

  NS_ASSERTION(request, "request is null!");

  if (count > 0)
    request->SniffMimeType(fromRawSegment, count);

  *writeCount = 0;
  return NS_ERROR_FAILURE;
}

void
imgRequest::SniffMimeType(const char *buf, PRUint32 len)
{
  imgLoader::GetMimeTypeFromContent(buf, len, mContentType);

  
  
  if (!mContentType.IsEmpty())
    return;

  
  
  
  const nsCOMArray<nsIContentSniffer>& sniffers = mImageSniffers.GetEntries();
  PRUint32 length = sniffers.Count();
  for (PRUint32 i = 0; i < length; ++i) {
    nsresult rv =
      sniffers[i]->GetMIMETypeFromContent(nsnull, (const PRUint8 *) buf, len, mContentType);
    if (NS_SUCCEEDED(rv) && !mContentType.IsEmpty()) {
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
                                   nsIChannel *newChannel, PRUint32 flags,
                                   nsIAsyncVerifyRedirectCallback *callback)
{
  NS_ASSERTION(mRequest && mChannel, "Got a channel redirect after we nulled out mRequest!");
  NS_ASSERTION(mChannel == oldChannel, "Got a channel redirect for an unknown channel!");
  NS_ASSERTION(newChannel, "Got a redirect to a NULL channel!");

  
  mRedirectCallback = callback;
  mNewRedirectChannel = newChannel;

  nsCOMPtr<nsIChannelEventSink> sink(do_GetInterface(mPrevChannelSink));
  if (sink) {
    nsresult rv = sink->AsyncOnChannelRedirect(oldChannel, newChannel, flags,
                                               this);
    if (NS_FAILED(rv)) {
        mRedirectCallback = nsnull;
        mNewRedirectChannel = nsnull;
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
      mRedirectCallback = nsnull;
      mNewRedirectChannel = nsnull;
      return NS_OK;
  }

  mChannel = mNewRedirectChannel;
  mNewRedirectChannel = nsnull;

  
  
  
  nsCAutoString oldspec;
  if (mKeyURI)
    mKeyURI->GetSpec(oldspec);
  LOG_MSG_WITH_PARAM(gImgLog, "imgRequest::OnChannelRedirect", "old", oldspec.get());

  
  
  nsCOMPtr<nsIURI> uri;
  mChannel->GetURI(getter_AddRefs(uri));
  PRBool doesNotReturnData = PR_FALSE;
  nsresult rv =
    NS_URIChainHasFlags(uri, nsIProtocolHandler::URI_DOES_NOT_RETURN_DATA,
                        &doesNotReturnData);

  if (NS_SUCCEEDED(rv) && doesNotReturnData)
    rv = NS_ERROR_ABORT;

  if (NS_FAILED(rv)) {
    mRedirectCallback->OnRedirectVerifyCallback(rv);
    mRedirectCallback = nsnull;
    return NS_OK;
  }

  nsCOMPtr<nsIURI> newURI;
  mChannel->GetOriginalURI(getter_AddRefs(newURI));
  nsCAutoString newspec;
  if (newURI)
    newURI->GetSpec(newspec);
  LOG_MSG_WITH_PARAM(gImgLog, "imgRequest::OnChannelRedirect", "new", newspec.get());

  if (oldspec != newspec) {
    if (mIsInCache) {
      
      
      
      if (mCacheEntry)
        imgLoader::RemoveFromCache(mCacheEntry);
      else
        imgLoader::RemoveFromCache(mKeyURI);
    }

    mKeyURI = newURI;
 
    if (mIsInCache) {
      
      
      if (mKeyURI && mCacheEntry)
        imgLoader::PutIntoCache(mKeyURI, mCacheEntry);
    }
  }

  mRedirectCallback->OnRedirectVerifyCallback(NS_OK);
  mRedirectCallback = nsnull;
  return NS_OK;
}
