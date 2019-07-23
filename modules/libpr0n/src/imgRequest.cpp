






































#include "imgRequest.h"

#include "imgLoader.h"
#include "imgCache.h"
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
#include "nsIProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"

#include "nsString.h"
#include "nsXPIDLString.h"
#include "plstr.h" 

#if defined(PR_LOGGING)
PRLogModuleInfo *gImgLog = PR_NewLogModule("imgRequest");
#endif

NS_IMPL_ISUPPORTS6(imgRequest, imgILoad,
                   imgIDecoderObserver, imgIContainerObserver,
                   nsIStreamListener, nsIRequestObserver,
                   nsISupportsWeakReference)

imgRequest::imgRequest() : 
  mObservers(0),
  mLoading(PR_FALSE), mProcessing(PR_FALSE), mHadLastPart(PR_FALSE),
  mNetworkStatus(0), mImageStatus(imgIRequest::STATUS_NONE), mState(0),
  mCacheId(0), mValidator(nsnull), mIsMultiPartChannel(PR_FALSE)
{
  
}

imgRequest::~imgRequest()
{
  
}

nsresult imgRequest::Init(nsIURI *aURI,
                          nsIRequest *aRequest,
                          nsICacheEntryDescriptor *aCacheEntry,
                          void *aCacheId,
                          void *aLoadId)
{
  LOG_FUNC(gImgLog, "imgRequest::Init");

  NS_ASSERTION(!mImage, "Multiple calls to init");
  NS_ASSERTION(aURI, "No uri");
  NS_ASSERTION(aRequest, "No request");

  mProperties = do_CreateInstance("@mozilla.org/properties;1");
  if (!mProperties)
    return NS_ERROR_OUT_OF_MEMORY;

  mURI = aURI;
  mRequest = aRequest;

  




  mLoading = PR_TRUE;

  mCacheEntry = aCacheEntry;

  mCacheId = aCacheId;

  SetLoadId(aLoadId);

  return NS_OK;
}

nsresult imgRequest::AddProxy(imgRequestProxy *proxy, PRBool aNotify)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::AddProxy", "proxy", proxy);

  mObservers.AppendElement(NS_STATIC_CAST(void*, proxy));

  if (aNotify)
    NotifyProxyListener(proxy);

  return NS_OK;
}

nsresult imgRequest::RemoveProxy(imgRequestProxy *proxy, nsresult aStatus, PRBool aNotify)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::RemoveProxy", "proxy", proxy);

  mObservers.RemoveElement(NS_STATIC_CAST(void*, proxy));

  





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

  if (mObservers.Count() == 0) {
    




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
    NS_ASSERTION(frame, "GetCurrentFrame gave back a null frame!");

    
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

void imgRequest::RemoveFromCache()
{
  LOG_SCOPE(gImgLog, "imgRequest::RemoveFromCache");

  if (mCacheEntry) {
    mCacheEntry->Doom();
    mCacheEntry = nsnull;
  }
}

PRBool imgRequest::HaveProxyWithObserver(imgRequestProxy* aProxyToIgnore) const
{
  for (PRInt32 i = 0; i < mObservers.Count(); ++i) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
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
  
  
  
  
  
  
  
  if (mObservers[0] != proxy)
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

  PRInt32 count = mObservers.Count();
  for (PRInt32 i = 0; i < count; i++) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    if (proxy) proxy->FrameChanged(container, newframe, dirtyRect);

    
    
    NS_ASSERTION(count == mObservers.Count(),
                 "The observer list changed while being iterated over!");
  }

  return NS_OK;
}




NS_IMETHODIMP imgRequest::OnStartDecode(imgIRequest *request)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartDecode");

  mState |= onStartDecode;

  PRInt32 count = mObservers.Count();
  for (PRInt32 i = 0; i < count; i++) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    if (proxy) proxy->OnStartDecode();

    
    
    NS_ASSERTION(count == mObservers.Count(), 
                 "The observer list changed while being iterated over!");
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

  PRInt32 count = mObservers.Count();
  for (PRInt32 i = 0; i < count; i++) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    if (proxy) proxy->OnStartContainer(image);

    
    
    NS_ASSERTION(count == mObservers.Count(), 
                 "The observer list changed while being iterated over!");

  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStartFrame(imgIRequest *request,
                                       gfxIImageFrame *frame)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStartFrame");

  PRInt32 count = mObservers.Count();
  for (PRInt32 i = 0; i < count; i++) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    if (proxy) proxy->OnStartFrame(frame);

    
    
    NS_ASSERTION(count == mObservers.Count(), 
                 "The observer list changed while being iterated over!");
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnDataAvailable(imgIRequest *request,
                                          gfxIImageFrame *frame,
                                          const nsIntRect * rect)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnDataAvailable");

  PRInt32 count = mObservers.Count();
  for (PRInt32 i = 0; i < count; i++) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    if (proxy) proxy->OnDataAvailable(frame, rect);

    
    
    NS_ASSERTION(count == mObservers.Count(), 
                 "The observer list changed while being iterated over!");
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
    PRUint32 cacheSize = 0;

    mCacheEntry->GetDataSize(&cacheSize);

    PRUint32 imageSize = 0;
    PRUint32 alphaSize = 0;

    frame->GetImageDataLength(&imageSize);
    frame->GetAlphaDataLength(&alphaSize);

    mCacheEntry->SetDataSize(cacheSize + imageSize + alphaSize);
  }

  PRInt32 count = mObservers.Count();
  for (PRInt32 i = 0; i < count; i++) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    if (proxy) proxy->OnStopFrame(frame);

    
    
    NS_ASSERTION(count == mObservers.Count(), 
                 "The observer list changed while being iterated over!");
  }

  return NS_OK;
}


NS_IMETHODIMP imgRequest::OnStopContainer(imgIRequest *request,
                                          imgIContainer *image)
{
  LOG_SCOPE(gImgLog, "imgRequest::OnStopContainer");

  mState |= onStopContainer;

  PRInt32 count = mObservers.Count();
  for (PRInt32 i = 0; i < count; i++) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    if (proxy) proxy->OnStopContainer(image);

    
    
    NS_ASSERTION(count == mObservers.Count(), 
                 "The observer list changed while being iterated over!");
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

  PRInt32 count = mObservers.Count();
  for (PRInt32 i = 0; i < count; i++) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    if (proxy) proxy->OnStopDecode(GetResultFromImageStatus(mImageStatus), aStatusArg);

    
    
    NS_ASSERTION(count == mObservers.Count(), 
                 "The observer list changed while being iterated over!");
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

  

  mImageStatus = imgIRequest::STATUS_NONE;
  mState = onStartRequest;

  
  mLoading = PR_TRUE;

  
  PRInt32 count = mObservers.Count();
  for (PRInt32 i = 0; i < count; i++) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    if (proxy) proxy->OnStartRequest(aRequest, ctxt);

    
    
    NS_ASSERTION(count == mObservers.Count(), 
                 "The observer list changed while being iterated over!");
  }

  nsCOMPtr<nsIChannel> chan(do_QueryInterface(aRequest));

  
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

          
          mCacheEntry->SetExpirationTime(expiration);
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

      if (bMustRevalidate) {
        mCacheEntry->SetMetaDataElement("MustValidateIfExpired", "true");
      }
    }
  }


  
  if (mObservers.Count() == 0) {
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

  
  
  
  
  if (mRequest)
  {
    mRequest->GetStatus(&mNetworkStatus);
    mRequest = nsnull;  
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

  
  PRInt32 count = mObservers.Count();
  for (PRInt32 i = count-1; i>=0; i--) {
    imgRequestProxy *proxy = NS_STATIC_CAST(imgRequestProxy*, mObservers[i]);
    


    if (proxy) proxy->OnStopRequest(aRequest, ctxt, status, mHadLastPart);
  }

  return NS_OK;
}





static NS_METHOD sniff_mimetype_callback(nsIInputStream* in, void* closure, const char* fromRawSegment,
                                         PRUint32 toOffset, PRUint32 count, PRUint32 *writeCount);





NS_IMETHODIMP imgRequest::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgRequest::OnDataAvailable", "count", count);

  NS_ASSERTION(aRequest, "imgRequest::OnDataAvailable -- no request!");

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

    nsresult rv = mDecoder->Init(NS_STATIC_CAST(imgILoad*, this));
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
  imgRequest *request = NS_STATIC_CAST(imgRequest*, closure);

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
}

nsresult 
imgRequest::GetNetworkStatus()
{
  nsresult status;
  if (mRequest)
    mRequest->GetStatus(&status);
  else
    status = mNetworkStatus;

  return status;
}
