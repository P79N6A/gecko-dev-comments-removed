






































#include "imgRequest.h"

#include "imgLoader.h"
#include "imgRequestProxy.h"

#include "imgILoader.h"
#include "ImageErrors.h"
#include "ImageLogging.h"

#include "gfxIImageFrame.h"

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

#if defined(PR_LOGGING)
PRLogModuleInfo *gImgLog = PR_NewLogModule("imgRequest");
#endif

NS_IMPL_ISUPPORTS8(imgRequest, imgILoad,
                   imgIDecoderObserver, imgIContainerObserver,
                   nsIStreamListener, nsIRequestObserver,
                   nsISupportsWeakReference,
                   nsIChannelEventSink,
                   nsIInterfaceRequestor)

imgRequest::imgRequest() : 
  mLoading(PR_FALSE), mProcessing(PR_FALSE), mHadLastPart(PR_FALSE),
  mGotData(PR_FALSE), mImageStatus(imgIRequest::STATUS_NONE),
  mState(0), mCacheId(0), mValidator(nsnull), mIsMultiPartChannel(PR_FALSE),
  mImageSniffers("image-sniffing-services") 
{
  
}

imgRequest::~imgRequest()
{
  
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

  NS_ASSERTION(!mImage, "Multiple calls to init");
  NS_ASSERTION(aURI, "No uri");
  NS_ASSERTION(aRequest, "No request");
  NS_ASSERTION(aChannel, "No channel");

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

  return NS_OK;
}

nsresult imgRequest::AddProxy(imgRequestProxy *proxy)
{
  NS_PRECONDITION(proxy, "null imgRequestProxy passed in");
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::AddProxy", "proxy", proxy);

  return mObservers.AppendElementUnlessExists(proxy) ?
    NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult imgRequest::RemoveProxy(imgRequestProxy *proxy, nsresult aStatus, PRBool aNotify)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::RemoveProxy", "proxy", proxy);

  mObservers.RemoveElement(proxy);

  





  if (aNotify) {
    
    if (!(mState & onStopDecode)) {
      proxy->OnStopDecode(aStatus, nsnull);
    }

  }

  
  if (!(mState & onStopRequest)) {
    proxy->OnStopRequest(nsnull, nsnull, NS_BINDING_ABORTED, PR_TRUE);
  }

  if (mImage && !HaveProxyWithObserver(nsnull)) {
    LOG_MSG(gImgLog, "imgRequest::RemoveProxy", "stopping animation");

    mImage->StopAnimation();
  }

  if (mObservers.IsEmpty()) {
    




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

  
  if (mState & onStartRequest)
    proxy->OnStartRequest(nsnull, nsnull);

  
  if (mState & onStartDecode)
    proxy->OnStartDecode();

  
  if (mState & onStartContainer)
    proxy->OnStartContainer(mImage);

  
  PRUint32 nframes = 0;
  if (mImage)
    mImage->GetNumFrames(&nframes);

  if (nframes > 0) {
    nsCOMPtr<gfxIImageFrame> frame;

    
    mImage->GetCurrentFrame(getter_AddRefs(frame));
    NS_ENSURE_TRUE(frame, NS_ERROR_OUT_OF_MEMORY);

    
    proxy->OnStartFrame(frame);

    if (!(mState & onStopContainer)) {
      
      nsIntRect r;
      frame->GetRect(r);  
      proxy->OnDataAvailable(frame, &r);
    } else {
      
      nsIntRect r;
      frame->GetRect(r);  
      proxy->OnDataAvailable(frame, &r);

      
      proxy->OnStopFrame(frame);
    }
  }

  
  if (mState & onStopContainer)
    proxy->OnStopContainer(mImage);

  
  if (mState & onStopDecode)
    proxy->OnStopDecode(GetResultFromImageStatus(mImageStatus), nsnull);

  if (mImage && !HaveProxyWithObserver(proxy) && proxy->HasObserver()) {
    LOG_MSG(gImgLog, "imgRequest::AddProxy", "resetting animation");

    mImage->ResetAnimation();
  }

  if (mState & onStopRequest) {
    proxy->OnStopRequest(nsnull, nsnull,
                         GetResultFromImageStatus(mImageStatus),
                         mHadLastPart);
  }

  return NS_OK;
}

nsresult imgRequest::GetResultFromImageStatus(PRUint32 aStatus) const
{
  nsresult rv = NS_OK;

  if (aStatus & imgIRequest::STATUS_ERROR)
    rv = NS_IMAGELIB_ERROR_FAILURE;
  else if (aStatus & imgIRequest::STATUS_LOAD_COMPLETE)
    rv = NS_IMAGELIB_SUCCESS_LOAD_FINISHED;

  return rv;
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

  if (aStatus != NS_IMAGELIB_ERROR_NO_DECODER) {
    RemoveFromCache();
  }

  if (mRequest && mLoading)
    mRequest->Cancel(aStatus);
}

void imgRequest::CancelAndAbort(nsresult aStatus)
{
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

  if (mCacheEntry) {
    imgLoader::RemoveFromCache(mURI);
    mCacheEntry = nsnull;
  }
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



NS_IMETHODIMP imgRequest::SetImage(imgIContainer *aImage)
{
  LOG_FUNC(gImgLog, "imgRequest::SetImage");

  mImage = aImage;

  return NS_OK;
}

NS_IMETHODIMP imgRequest::GetImage(imgIContainer **aImage)
{
  LOG_FUNC(gImgLog, "imgRequest::GetImage");

  *aImage = mImage;
  NS_IF_ADDREF(*aImage);
  return NS_OK;
}

NS_IMETHODIMP imgRequest::GetIsMultiPartChannel(PRBool *aIsMultiPartChannel)
{
  LOG_FUNC(gImgLog, "imgRequest::GetIsMultiPartChannel");

  *aIsMultiPartChannel = mIsMultiPartChannel;

  return NS_OK;
}




NS_IMETHODIMP imgRequest::FrameChanged(imgIContainer *container,
                                       gfxIImageFrame *newframe,
                                       nsIntRect * dirtyRect)
{
  LOG_SCOPE(gImgLog, "imgRequest::FrameChanged");

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->FrameChanged(container, newframe, dirtyRect);
  }

  return NS_OK;
}




NS_IMETHODIMP imgRequest::OnStartDecode(imgIRequest *request)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartDecode");

  mState |= onStartDecode;

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

  mState |= onStartContainer;

  mImageStatus |= imgIRequest::STATUS_SIZE_AVAILABLE;

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnStartContainer(image);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStartFrame(imgIRequest *request,
                                       gfxIImageFrame *frame)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartFrame");

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnStartFrame(frame);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnDataAvailable(imgIRequest *request,
                                          gfxIImageFrame *frame,
                                          const nsIntRect * rect)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable");

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnDataAvailable(frame, rect);
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopFrame(imgIRequest *request,
                                      gfxIImageFrame *frame)
{
  NS_ASSERTION(frame, "imgRequest::OnStopFrame called with NULL frame");
  if (!frame) return NS_ERROR_UNEXPECTED;

  LOG_SCOPE(gImgLog, "imgRequest::OnStopFrame");

  mImageStatus |= imgIRequest::STATUS_FRAME_COMPLETE;

  if (mCacheEntry) {
    PRUint32 cacheSize = mCacheEntry->GetDataSize();

    PRUint32 imageSize = 0;
    frame->GetImageDataLength(&imageSize);

    mCacheEntry->SetDataSize(cacheSize + imageSize);

#ifdef DEBUG_joe
    nsCAutoString url;
    mURI->GetSpec(url);

    printf("CACHEPUT: %d %s %d\n", time(NULL), url.get(), cacheSize + imageSize);
#endif
  }

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

  mState |= onStopContainer;

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

  NS_ASSERTION(!(mState & onStopDecode), "OnStopDecode called multiple times.");

  mState |= onStopDecode;

  if (NS_FAILED(aStatus) && !(mImageStatus & imgIRequest::STATUS_LOAD_PARTIAL)) {
    mImageStatus |= imgIRequest::STATUS_ERROR;
  }

  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnStopDecode(GetResultFromImageStatus(mImageStatus), aStatusArg);
  }

  return NS_OK;
}

NS_IMETHODIMP imgRequest::OnStopRequest(imgIRequest *aRequest,
                                        PRBool aLastPart)
{
  NS_NOTREACHED("imgRequest(imgIDecoderObserver)::OnStopRequest");
  return NS_OK;
}




NS_IMETHODIMP imgRequest::OnStartRequest(nsIRequest *aRequest, nsISupports *ctxt)
{
  nsresult rv;

  LOG_SCOPE(gImgLog, "imgRequest::OnStartRequest");

  NS_ASSERTION(!mDecoder, "imgRequest::OnStartRequest -- we already have a decoder");

  nsCOMPtr<nsIMultiPartChannel> mpchan(do_QueryInterface(aRequest));
  if (mpchan)
      mIsMultiPartChannel = PR_TRUE;

  






  if (!mRequest) {
    NS_ASSERTION(mpchan,
                 "We should have an mRequest here unless we're multipart");
    nsCOMPtr<nsIChannel> chan;
    mpchan->GetBaseChannel(getter_AddRefs(chan));
    mRequest = chan;
  }

  

  mImageStatus = imgIRequest::STATUS_NONE;
  mState = onStartRequest;

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

  mState |= onStopRequest;

  
  mLoading = PR_FALSE;

  
  mProcessing = PR_FALSE;

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

  
  if (NS_FAILED(status) || !mImage) {
    this->Cancel(status); 
  } else {
    mImageStatus |= imgIRequest::STATUS_LOAD_COMPLETE;
  }

  if (mDecoder) {
    mDecoder->Flush();
    mDecoder->Close();
    mDecoder = nsnull; 
  }

  
  
  if (!(mState & onStopDecode)) {
    this->OnStopDecode(nsnull, status, nsnull);
  }

  
  nsTObserverArray<imgRequestProxy*>::ForwardIterator iter(mObservers);
  while (iter.HasMore()) {
    iter.GetNext()->OnStopRequest(aRequest, ctxt, status, mHadLastPart);
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

  if (!mProcessing) {
    LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable |First time through... finding mimetype|");

    
    mProcessing = PR_TRUE;

    


    PRUint32 out;
    inStr->ReadSegments(sniff_mimetype_callback, this, count, &out);

#ifdef NS_DEBUG
    
#endif

    if (mContentType.IsEmpty()) {
      LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable |sniffing of mimetype failed|");

      nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));

      nsresult rv = NS_ERROR_FAILURE;
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

    nsCAutoString conid(NS_LITERAL_CSTRING("@mozilla.org/image/decoder;2?type=") + mContentType);

    mDecoder = do_CreateInstance(conid.get());

    if (!mDecoder) {
      PR_LOG(gImgLog, PR_LOG_WARNING,
             ("[this=%p] imgRequest::OnDataAvailable -- Decoder not available\n", this));

      
      this->Cancel(NS_IMAGELIB_ERROR_NO_DECODER);

      return NS_IMAGELIB_ERROR_NO_DECODER;
    }

    nsresult rv = mDecoder->Init(static_cast<imgILoad*>(this));
    if (NS_FAILED(rv)) {
      PR_LOG(gImgLog, PR_LOG_WARNING,
             ("[this=%p] imgRequest::OnDataAvailable -- mDecoder->Init failed\n", this));

      this->Cancel(NS_IMAGELIB_ERROR_FAILURE);

      return NS_BINDING_ABORTED;
    }
  }

  if (!mDecoder) {
    PR_LOG(gImgLog, PR_LOG_WARNING,
           ("[this=%p] imgRequest::OnDataAvailable -- no decoder\n", this));

    this->Cancel(NS_IMAGELIB_ERROR_NO_DECODER);

    return NS_BINDING_ABORTED;
  }

  PRUint32 wrote;
  nsresult rv = mDecoder->WriteFrom(inStr, count, &wrote);

  if (NS_FAILED(rv)) {
    PR_LOG(gImgLog, PR_LOG_WARNING,
           ("[this=%p] imgRequest::OnDataAvailable -- mDecoder->WriteFrom failed\n", this));

    this->Cancel(NS_IMAGELIB_ERROR_FAILURE);

    return NS_BINDING_ABORTED;
  }

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

  RemoveFromCache();

  mChannel = newChannel;

  newChannel->GetOriginalURI(getter_AddRefs(mKeyURI));

  
  if (mKeyURI && mCacheEntry)
    imgLoader::PutIntoCache(mKeyURI, mCacheEntry);

  return rv;
}
