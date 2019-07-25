







































#include "imgRequest.h"

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

#define DISCARD_PREF "image.mem.discardable"
#define DECODEONDRAW_PREF "image.mem.decodeondraw"


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
      strcmp(NS_LossyConvertUTF16toASCII(aData).get(), DECODEONDRAW_PREF))
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

NS_IMPL_ISUPPORTS7(imgRequest,
                   imgIDecoderObserver, imgIContainerObserver,
                   nsIStreamListener, nsIRequestObserver,
                   nsISupportsWeakReference,
                   nsIChannelEventSink,
                   nsIInterfaceRequestor)

imgRequest::imgRequest() : 
  mImageStatus(imgIRequest::STATUS_NONE), mState(0), mCacheId(0), 
  mValidator(nsnull), mImageSniffers("image-sniffing-services"),
  mDeferredLocks(0), mDecodeRequested(PR_FALSE),
  mIsMultiPartChannel(PR_FALSE), mLoading(PR_FALSE),
  mHadLastPart(PR_FALSE), mGotData(PR_FALSE), mIsInCache(PR_FALSE)
{
  
}

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
  if (!mProperties)
    return NS_ERROR_OUT_OF_MEMORY;


  mURI = aURI;
  mKeyURI = aKeyURI;
  mRequest = aRequest;
  mChannel = aChannel;
  mChannel->GetNotificationCallbacks(getter_AddRefs(mPrevChannelSink));

  NS_ASSERTION(mPrevChannelSink != this,
               "Initializing with a channel that already calls back to us!");

  mChannel->SetNotificationCallbacks(this);

  




  mLoading = PR_TRUE;

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

  return mObservers.AppendElementUnlessExists(proxy) ?
    NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult imgRequest::RemoveProxy(imgRequestProxy *proxy, nsresult aStatus, PRBool aNotify)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::RemoveProxy", "proxy", proxy);

  mObservers.RemoveElement(proxy);

  





  if (aNotify) {
    
    if (!(mState & stateDecodeStopped)) {
      proxy->OnStopContainer(mImage);
    }

    
    if (!(mState & stateRequestStopped)) {
      proxy->OnStopDecode(aStatus, nsnull);
    }
  }

  
  if (!(mState & stateRequestStopped)) {
    proxy->OnStopRequest(nsnull, nsnull, NS_BINDING_ABORTED, PR_TRUE);
  }

  if (mImage && !HaveProxyWithObserver(nsnull)) {
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

    




    if (mRequest && mLoading && NS_FAILED(aStatus)) {
      LOG_MSG(gImgLog, "imgRequest::RemoveProxy", "load in progress.  canceling");

      mImageStatus |= imgIRequest::STATUS_LOAD_PARTIAL;

      this->Cancel(NS_BINDING_ABORTED);
    }

    
    mCacheEntry = nsnull;
  }

  
  
  if (aStatus != NS_IMAGELIB_CHANGING_OWNER)
    proxy->RemoveFromLoadGroup(PR_TRUE);

  return NS_OK;
}

nsresult imgRequest::NotifyProxyListener(imgRequestProxy *proxy)
{
  nsCOMPtr<imgIRequest> kungFuDeathGrip(proxy);

  

  
  if (mState & stateRequestStarted)
    proxy->OnStartRequest(nsnull, nsnull);

  
  if (mState & stateHasSize)
    proxy->OnStartContainer(mImage);

  
  if (mState & stateDecodeStarted)
    proxy->OnStartDecode();

  
  PRUint32 nframes = 0;
  if (mImage)
    mImage->GetNumFrames(&nframes);

  if (nframes > 0) {
    PRUint32 frame;
    mImage->GetCurrentFrameIndex(&frame);
    proxy->OnStartFrame(frame);

    
    
    
    nsIntRect r;
    mImage->GetCurrentFrameRect(r);
    proxy->OnDataAvailable(frame, &r);

    if (mState & stateRequestStopped)
      proxy->OnStopFrame(frame);
  }

  if (mImage && !HaveProxyWithObserver(proxy) && proxy->HasObserver()) {
    LOG_MSG(gImgLog, "imgRequest::NotifyProxyListener", "resetting animation");

    mImage->ResetAnimation();
  }

  
  if (mState & stateDecodeStopped)
    proxy->OnStopContainer(mImage);

  if (mState & stateRequestStopped) {
    proxy->OnStopDecode(GetResultFromImageStatus(mImageStatus), nsnull);
    proxy->OnStopRequest(nsnull, nsnull,
                         GetResultFromImageStatus(mImageStatus),
                         mHadLastPart);
  }

  return NS_OK;
}

void imgRequest::Cancel(nsresult aStatus)
{
  

  LOG_SCOPE(gImgLog, "imgRequest::Cancel");

  if (mImage) {
    LOG_MSG(gImgLog, "imgRequest::Cancel", "stopping animation");

    mImage->StopAnimation();
  }

  if (!(mImageStatus & imgIRequest::STATUS_LOAD_PARTIAL))
    mImageStatus |= imgIRequest::STATUS_ERROR;

  RemoveFromCache();

  if (mRequest && mLoading)
    mRequest->Cancel(aStatus);
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

nsresult imgRequest::GetPrincipal(nsIPrincipal **aPrincipal)
{
  LOG_FUNC(gImgLog, "imgRequest::GetPrincipal");

  if (mPrincipal) {
    NS_ADDREF(*aPrincipal = mPrincipal);
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
    if (mImage)
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
imgRequest::GetImage(imgIContainer **aImage)
{
  LOG_FUNC(gImgLog, "imgRequest::GetImage");

  *aImage = mImage;
  NS_IF_ADDREF(*aImage);
  return NS_OK;
}

nsresult
imgRequest::LockImage()
{
  
  if (mImage) {
    NS_ABORT_IF_FALSE(mDeferredLocks == 0, "Have image, but deferred locks?");
    return mImage->LockImage();
  }

  
  mDeferredLocks++;

  return NS_OK;
}

nsresult
imgRequest::UnlockImage()
{
  
  if (mImage) {
    NS_ABORT_IF_FALSE(mDeferredLocks == 0, "Have image, but deferred locks?");
    return mImage->UnlockImage();
  }

  
  
  NS_ABORT_IF_FALSE(mDeferredLocks > 0, "lock/unlock calls must be matched!");
  mDeferredLocks--;

  return NS_OK;
}

nsresult
imgRequest::RequestDecode()
{
  
  if (mImage) {
    return mImage->RequestDecode();
  }

  
  mDecodeRequested = PR_TRUE;

  return NS_OK;
}




NS_IMETHODIMP imgRequest::FrameChanged(imgIContainer *container,
                                       nsIntRect * dirtyRect)
{
  LOG_SCOPE(gImgLog, "imgRequest::FrameChanged");

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->FrameChanged(container, dirtyRect);
  }

  return NS_OK;
}




NS_IMETHODIMP imgRequest::OnStartDecode(imgIRequest *request)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartDecode");

  mState |= stateDecodeStarted;

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnStartDecode();
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

  
  PRBool alreadySent = (mState & stateHasSize) != 0;

  mState |= stateHasSize;

  mImageStatus |= imgIRequest::STATUS_SIZE_AVAILABLE;

  if (!alreadySent) {
    nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
    while (iter.HasMore()) {
      iter.GetNext()->OnStartContainer(image);
    }
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStartFrame(imgIRequest *request,
                                       PRUint32 frame)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartFrame");

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnStartFrame(frame);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnDataAvailable(imgIRequest *request,
                                          PRBool aCurrentFrame,
                                          const nsIntRect * rect)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable");

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnDataAvailable(aCurrentFrame, rect);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopFrame(imgIRequest *request,
                                      PRUint32 frame)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopFrame");

  mImageStatus |= imgIRequest::STATUS_FRAME_COMPLETE;

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnStopFrame(frame);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopContainer(imgIRequest *request,
                                          imgIContainer *image)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopContainer");

  
  
  mState |= stateDecodeStopped;

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnStopContainer(image);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopDecode(imgIRequest *aRequest,
                                       nsresult aStatus,
                                       const PRUnichar *aStatusArg)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopDecode");

  
  
  UpdateCacheEntrySize();

  
  if (NS_SUCCEEDED(aStatus))
    mImageStatus |= imgIRequest::STATUS_DECODE_COMPLETE;
  
  else
    mImageStatus = imgIRequest::STATUS_ERROR;

  
  
  
  
  
  
  
  
  
  
  

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
  
  PRUint32 stateBitsToClear = stateDecodeStarted | stateDecodeStopped;
  mState &= ~stateBitsToClear;

  
  PRUint32 statusBitsToClear = imgIRequest::STATUS_FRAME_COMPLETE
                               | imgIRequest::STATUS_DECODE_COMPLETE;
  mImageStatus &= ~statusBitsToClear;

  
  UpdateCacheEntrySize();

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnDiscard();
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

  
  NS_ABORT_IF_FALSE(mIsMultiPartChannel || !mImage,
                    "Already have an image for non-multipart request");

  
  if (mIsMultiPartChannel && mImage) {

    
    mImage->NewSourceData();

    
    mImageStatus &= ~imgIRequest::STATUS_LOAD_PARTIAL;
    mImageStatus &= ~imgIRequest::STATUS_LOAD_COMPLETE;
    mImageStatus &= ~imgIRequest::STATUS_FRAME_COMPLETE;
    mState &= ~stateRequestStarted;
    mState &= ~stateDecodeStarted;
    mState &= ~stateDecodeStopped;
    mState &= ~stateRequestStopped;
  }

  






  if (!mRequest) {
    NS_ASSERTION(mpchan,
                 "We should have an mRequest here unless we're multipart");
    nsCOMPtr<nsIChannel> chan;
    mpchan->GetBaseChannel(getter_AddRefs(chan));
    mRequest = chan;
  }

  
  mState |= stateRequestStarted;

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel)
    channel->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

  
  mLoading = PR_TRUE;

  
  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnStartRequest(aRequest, ctxt);
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

  mState |= stateRequestStopped;

  
  mLoading = PR_FALSE;

  mHadLastPart = PR_TRUE;
  nsCOMPtr<nsIMultiPartChannel> mpchan(do_QueryInterface(aRequest));
  if (mpchan) {
    PRBool lastPart;
    nsresult rv = mpchan->GetIsLastPart(&lastPart);
    if (NS_SUCCEEDED(rv))
      mHadLastPart = lastPart;
  }

  
  
  
  
  if (mRequest) {
    mRequest = nsnull;  
  }

  
  if (mChannel) {
    mChannel->SetNotificationCallbacks(mPrevChannelSink);
    mPrevChannelSink = nsnull;
    mChannel = nsnull;
  }

  
  
  
  if (mImage) {

    
    nsresult rv = mImage->SourceDataComplete();

    
    
    
    
    if (NS_FAILED(rv) && NS_SUCCEEDED(status))
      status = rv;
  }

  
  
  
  if (NS_SUCCEEDED(status)) {

    
    mImageStatus |= imgIRequest::STATUS_LOAD_COMPLETE;

    
    
    UpdateCacheEntrySize();
  }
  else
    this->Cancel(status); 

  
  nsTObserverArray<imgRequestProxy*>::ForwardIterator sdIter(mObservers);
  while (sdIter.HasMore()) {
    sdIter.GetNext()->OnStopDecode(GetResultFromImageStatus(mImageStatus), nsnull);
  }

  nsTObserverArray<imgRequestProxy*>::ForwardIterator srIter(mObservers);
  while (srIter.HasMore()) {
    srIter.GetNext()->OnStopRequest(aRequest, ctxt, status, mHadLastPart);
  }

  return NS_OK;
}


static NS_METHOD sniff_mimetype_callback(nsIInputStream* in, void* closure, const char* fromRawSegment,
                                         PRUint32 toOffset, PRUint32 count, PRUint32 *writeCount);




NS_IMETHODIMP imgRequest::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::OnDataAvailable", "count", count);

  NS_ASSERTION(aRequest, "imgRequest::OnDataAvailable -- no request!");

  mGotData = PR_TRUE;
  nsresult rv;

  if (!mImage) {
    LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable |First time through... finding mimetype|");

    


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

    
    
    
    mImage = do_CreateInstance("@mozilla.org/image/container;3");
    if (!mImage) {
      this->Cancel(NS_ERROR_OUT_OF_MEMORY);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    rv = mImage->Init(this, mContentType.get(), containerFlags);
    if (NS_FAILED(rv)) { 

      
      
      
      
      
      
      mImage = nsnull;

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
    while (mDeferredLocks) {
      mImage->LockImage();
      mDeferredLocks--;
    }
  }

  
  PRUint32 bytesRead;
  rv = inStr->ReadSegments(imgContainer::WriteToContainer,
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
imgRequest::OnChannelRedirect(nsIChannel *oldChannel, nsIChannel *newChannel, PRUint32 flags)
{
  NS_ASSERTION(mRequest && mChannel, "Got an OnChannelRedirect after we nulled out mRequest!");
  NS_ASSERTION(mChannel == oldChannel, "Got a channel redirect for an unknown channel!");
  NS_ASSERTION(newChannel, "Got a redirect to a NULL channel!");

  nsresult rv = NS_OK;
  nsCOMPtr<nsIChannelEventSink> sink(do_GetInterface(mPrevChannelSink));
  if (sink) {
    rv = sink->OnChannelRedirect(oldChannel, newChannel, flags);
    if (NS_FAILED(rv))
      return rv;
  }

  mChannel = newChannel;

  
  
  
  nsCAutoString oldspec;
  if (mKeyURI)
    mKeyURI->GetSpec(oldspec);
  LOG_MSG_WITH_PARAM(gImgLog, "imgRequest::OnChannelRedirect", "old", oldspec.get());

  
  
  nsCOMPtr<nsIURI> uri;
  newChannel->GetURI(getter_AddRefs(uri));
  PRBool doesNotReturnData = PR_FALSE;
  rv = NS_URIChainHasFlags(uri, nsIProtocolHandler::URI_DOES_NOT_RETURN_DATA,
                           &doesNotReturnData);
  if (NS_FAILED(rv))
    return rv;
  if (doesNotReturnData)
    return NS_ERROR_ABORT;

  nsCOMPtr<nsIURI> newURI;
  newChannel->GetOriginalURI(getter_AddRefs(newURI));
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

  return rv;
}
