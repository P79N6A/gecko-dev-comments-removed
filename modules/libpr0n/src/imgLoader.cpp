






































#include "imgLoader.h"

#include "nsCOMPtr.h"

#include "nsNetUtil.h"
#include "nsIHttpChannel.h"
#include "nsICachingChannel.h"
#include "nsIProxyObjectManager.h"
#include "nsIServiceManager.h"
#include "nsThreadUtils.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"

#include "netCore.h"

#include "imgCache.h"
#include "imgRequest.h"
#include "imgRequestProxy.h"

#include "ImageErrors.h"
#include "ImageLogging.h"

#include "nsIComponentRegistrar.h"




#include "nsIHttpChannelInternal.h"  

#if defined(DEBUG_pavlov) || defined(DEBUG_timeless)
#include "nsISimpleEnumerator.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsXPIDLString.h"
#include "nsComponentManagerUtils.h"

static void PrintImageDecoders()
{
  nsCOMPtr<nsIComponentRegistrar> compMgr;
  if (NS_FAILED(NS_GetComponentRegistrar(getter_AddRefs(compMgr))) || !compMgr)
    return;
  nsCOMPtr<nsISimpleEnumerator> enumer;
  if (NS_FAILED(compMgr->EnumerateContractIDs(getter_AddRefs(enumer))) || !enumer)
    return;
  
  nsCString str;
  nsCOMPtr<nsISupports> s;
  PRBool more = PR_FALSE;
  while (NS_SUCCEEDED(enumer->HasMoreElements(&more)) && more) {
    enumer->GetNext(getter_AddRefs(s));
    if (s) {
      nsCOMPtr<nsISupportsCString> ss(do_QueryInterface(s));

      nsCAutoString xcs;
      ss->GetData(xcs);

      NS_NAMED_LITERAL_CSTRING(decoderContract, "@mozilla.org/image/decoder;2?type=");

      if (StringBeginsWith(xcs, decoderContract)) {
        printf("Have decoder for mime type: %s\n", xcs.get()+decoderContract.Length());
      }
    }
  }
}
#endif

NS_IMPL_ISUPPORTS2(imgLoader, imgILoader, nsIContentSniffer)

imgLoader::imgLoader()
{
  
#ifdef DEBUG_pavlov
  PrintImageDecoders();
#endif
}

imgLoader::~imgLoader()
{
  
}

#define LOAD_FLAGS_CACHE_MASK    (nsIRequest::LOAD_BYPASS_CACHE | \
                                  nsIRequest::LOAD_FROM_CACHE)

#define LOAD_FLAGS_VALIDATE_MASK (nsIRequest::VALIDATE_ALWAYS |   \
                                  nsIRequest::VALIDATE_NEVER |    \
                                  nsIRequest::VALIDATE_ONCE_PER_SESSION)


static PRBool RevalidateEntry(nsICacheEntryDescriptor *aEntry,
                              nsLoadFlags aFlags,
                              PRBool aHasExpired)
{
  PRBool bValidateEntry = PR_FALSE;

  NS_ASSERTION(!(aFlags & nsIRequest::LOAD_BYPASS_CACHE),
               "MUST not revalidate when BYPASS_CACHE is specified.");

  if (aFlags & nsIRequest::VALIDATE_ALWAYS) {
    bValidateEntry = PR_TRUE;
  }
  
  
  
  
  else if (aHasExpired) {
    
    
    
    
    
    if (aFlags & (nsIRequest::VALIDATE_NEVER | 
                  nsIRequest::VALIDATE_ONCE_PER_SESSION)) 
    {
      nsXPIDLCString value;

      aEntry->GetMetaDataElement("MustValidateIfExpired",
                                 getter_Copies(value));
      if (PL_strcmp(value, "true")) {
        bValidateEntry = PR_TRUE;
      }
    }
    
    
    
    
    else if (!(aFlags & nsIRequest::LOAD_FROM_CACHE)) {
      bValidateEntry = PR_TRUE;
    }
  }

  return bValidateEntry;
}


static nsresult NewImageChannel(nsIChannel **aResult,
                                nsIURI *aURI,
                                nsIURI *aInitialDocumentURI,
                                nsIURI *aReferringURI,
                                nsILoadGroup *aLoadGroup,
                                nsLoadFlags aLoadFlags)
{
  nsresult rv;
  nsCOMPtr<nsIChannel> newChannel;
  nsCOMPtr<nsIHttpChannel> newHttpChannel;
 
  nsCOMPtr<nsIInterfaceRequestor> callbacks;

  if (aLoadGroup) {
    
    
    
    
    
    
    
    aLoadGroup->GetNotificationCallbacks(getter_AddRefs(callbacks));
  }

  
  
  
  
  
  
  rv = NS_NewChannel(aResult,
                     aURI,        
                     nsnull,      
                     nsnull,      
                     callbacks,   
                     aLoadFlags);
  if (NS_FAILED(rv))
    return rv;

  
  newHttpChannel = do_QueryInterface(*aResult);
  if (newHttpChannel) {
    newHttpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
                                     NS_LITERAL_CSTRING("image/png,*/*;q=0.5"),
                                     PR_FALSE);

    nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal = do_QueryInterface(newHttpChannel);
    NS_ENSURE_TRUE(httpChannelInternal, NS_ERROR_UNEXPECTED);
    httpChannelInternal->SetDocumentURI(aInitialDocumentURI);
    newHttpChannel->SetReferrer(aReferringURI);
  }

  
  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(*aResult);
  if (p) {
    PRUint32 priority = nsISupportsPriority::PRIORITY_LOW;

    if (aLoadFlags & nsIRequest::LOAD_BACKGROUND)
      ++priority; 

    p->AdjustPriority(priority);
  }

  return NS_OK;
}



NS_IMETHODIMP imgLoader::LoadImage(nsIURI *aURI, 
                                   nsIURI *aInitialDocumentURI,
                                   nsIURI *aReferrerURI,
                                   nsILoadGroup *aLoadGroup,
                                   imgIDecoderObserver *aObserver,
                                   nsISupports *aCX,
                                   nsLoadFlags aLoadFlags,
                                   nsISupports *cacheKey,
                                   imgIRequest *aRequest,
                                   imgIRequest **_retval)
{
  NS_ASSERTION(aURI, "imgLoader::LoadImage -- NULL URI pointer");

  
  
  
  *_retval = nsnull;

  if (!aURI)
    return NS_ERROR_NULL_POINTER;

#if defined(PR_LOGGING)
  nsCAutoString spec;
  aURI->GetAsciiSpec(spec);
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgLoader::LoadImage", "aURI", spec.get());
#endif

  
  imgRequest *request = nsnull;

  nsresult rv;
  nsLoadFlags requestFlags = nsIRequest::LOAD_NORMAL;

  
  if (aLoadGroup) {
    aLoadGroup->GetLoadFlags(&requestFlags);
  }
  
  
  
  
  
  
  
  if (aLoadFlags & LOAD_FLAGS_CACHE_MASK) {
    
    requestFlags = (requestFlags & ~LOAD_FLAGS_CACHE_MASK) |
                   (aLoadFlags & LOAD_FLAGS_CACHE_MASK);
  }
  if (aLoadFlags & LOAD_FLAGS_VALIDATE_MASK) {
    
    requestFlags = (requestFlags & ~LOAD_FLAGS_VALIDATE_MASK) |
                   (aLoadFlags & LOAD_FLAGS_VALIDATE_MASK);
  }
  if (aLoadFlags & nsIRequest::LOAD_BACKGROUND) {
    
    requestFlags |= nsIRequest::LOAD_BACKGROUND;
  }

  nsCOMPtr<nsICacheEntryDescriptor> entry;
  PRBool bCanCacheRequest = PR_TRUE;
  PRBool bHasExpired      = PR_FALSE;
  PRBool bValidateRequest = PR_FALSE;

  
  
  
  imgCache::Get(aURI, &bHasExpired,
                &request, getter_AddRefs(entry)); 

  if (request && entry) {

    
    
    
    if (!request->mCacheEntry) {
      request->mCacheEntry = entry;
    }

    
    
    
    
    
    
    void *key = (void*)aCX;
    if (request->mLoadId != key) {

      
      if (requestFlags & nsIRequest::LOAD_BYPASS_CACHE) {
        
        
        
        entry->Doom();
        entry = nsnull;
        request->RemoveFromCache();
        NS_RELEASE(request);
      } else {
        
        bValidateRequest = RevalidateEntry(entry, requestFlags, bHasExpired);

        PR_LOG(gImgLog, PR_LOG_DEBUG,
               ("imgLoader::LoadImage validating cache entry. " 
                "bValidateRequest = %d", bValidateRequest));
      }

    }
#if defined(PR_LOGGING)
    else if (!key) {
      PR_LOG(gImgLog, PR_LOG_DEBUG,
             ("imgLoader::LoadImage BYPASSING cache validation for %s " 
              "because of NULL LoadID", spec.get()));
    }
#endif
  }

  
  
  
  
  void *cacheId = NS_GetCurrentThread();
  if (request && !request->IsReusable(cacheId)) {
    
    
    
    
    
    
    
    
    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::LoadImage -- DANGER!! Unable to use cached "
            "imgRequest [request=%p]\n", this, request));

    entry = nsnull;
    NS_RELEASE(request);

    bCanCacheRequest = PR_FALSE;
  }

  
  
  
  
  
  
  
  
  
  if (request && bValidateRequest) {
    
    LOG_SCOPE(gImgLog, "imgLoader::LoadImage |cache hit| must validate");

    
    
    

    if (request->mValidator) {
      rv = CreateNewProxyForRequest(request, aLoadGroup, aObserver,
                                    requestFlags, aRequest, _retval);

      if (*_retval)
        request->mValidator->AddProxy(static_cast<imgRequestProxy*>(*_retval));

      NS_RELEASE(request);
      return rv;

    } else {
      nsCOMPtr<nsIChannel> newChannel;
      rv = NewImageChannel(getter_AddRefs(newChannel),
                           aURI,
                           aInitialDocumentURI,
                           aReferrerURI,
                           aLoadGroup,
                           requestFlags);
      if (NS_FAILED(rv)) {
        NS_RELEASE(request);
        return NS_ERROR_FAILURE;
      }

      nsCOMPtr<nsICachingChannel> cacheChan(do_QueryInterface(newChannel));

      if (cacheChan) {
        
        
        PRUint32 loadFlags;
        if (NS_SUCCEEDED(newChannel->GetLoadFlags(&loadFlags)))
            newChannel->SetLoadFlags(loadFlags | nsICachingChannel::LOAD_ONLY_IF_MODIFIED);

      }
      nsCOMPtr<imgIRequest> req;
      rv = CreateNewProxyForRequest(request, aLoadGroup, aObserver,
                                    requestFlags, aRequest, getter_AddRefs(req));
      if (NS_FAILED(rv)) {
        NS_RELEASE(request);
        return rv;
      }

      imgCacheValidator *hvc = new imgCacheValidator(request, aCX);
      if (!hvc) {
        NS_RELEASE(request);
        return NS_ERROR_OUT_OF_MEMORY;
      }

      NS_ADDREF(hvc);
      request->mValidator = hvc;

      hvc->AddProxy(static_cast<imgRequestProxy*>
                               (static_cast<imgIRequest*>(req.get())));

      rv = newChannel->AsyncOpen(static_cast<nsIStreamListener *>(hvc), nsnull);
      if (NS_SUCCEEDED(rv))
        NS_ADDREF(*_retval = req.get());

      NS_RELEASE(hvc);

      NS_RELEASE(request);

      return rv;
    }
  } else if (!request) {
    
    LOG_SCOPE(gImgLog, "imgLoader::LoadImage |cache miss|");

    nsCOMPtr<nsIChannel> newChannel;
    rv = NewImageChannel(getter_AddRefs(newChannel),
                         aURI,
                         aInitialDocumentURI,
                         aReferrerURI,
                         aLoadGroup,
                         requestFlags);
    if (NS_FAILED(rv))
      return NS_ERROR_FAILURE;

    NS_NEWXPCOM(request, imgRequest);
    if (!request) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(request);

    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::LoadImage -- Created new imgRequest [request=%p]\n", this, request));

    
    if (bCanCacheRequest) {
      imgCache::Put(aURI, request, getter_AddRefs(entry));
    }

    
    
    nsCOMPtr<nsILoadGroup> loadGroup =
        do_CreateInstance(NS_LOADGROUP_CONTRACTID);
    newChannel->SetLoadGroup(loadGroup);

    request->Init(aURI, loadGroup, entry, cacheId, aCX);

    
    ProxyListener *pl = new ProxyListener(static_cast<nsIStreamListener *>(request));
    if (!pl) {
      request->Cancel(NS_ERROR_OUT_OF_MEMORY);
      NS_RELEASE(request);
      return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(pl);

    PR_LOG(gImgLog, PR_LOG_DEBUG,
           ("[this=%p] imgLoader::LoadImage -- Calling channel->AsyncOpen()\n", this));

    nsresult openRes;
    openRes = newChannel->AsyncOpen(static_cast<nsIStreamListener *>(pl), nsnull);

    NS_RELEASE(pl);

    if (NS_FAILED(openRes)) {
      PR_LOG(gImgLog, PR_LOG_DEBUG,
             ("[this=%p] imgLoader::LoadImage -- AsyncOpen() failed: 0x%x\n",
              this, openRes));
      request->Cancel(openRes);
      NS_RELEASE(request);
      return openRes;
    }

  } else {
    
    
    LOG_MSG_WITH_PARAM(gImgLog, 
                       "imgLoader::LoadImage |cache hit|", "request", request);

    
    request->SetLoadId(aCX);
  }

  LOG_MSG(gImgLog, "imgLoader::LoadImage", "creating proxy request.");

  rv = CreateNewProxyForRequest(request, aLoadGroup, aObserver,
                                requestFlags, aRequest, _retval);

  imgRequestProxy *proxy = (imgRequestProxy *)*_retval;

  
  
  
  proxy->AddToLoadGroup();

  
  
  if (!bValidateRequest) {
    request->NotifyProxyListener(proxy);
  }

  NS_RELEASE(request);

  return rv;
}


NS_IMETHODIMP imgLoader::LoadImageWithChannel(nsIChannel *channel, imgIDecoderObserver *aObserver, nsISupports *aCX, nsIStreamListener **listener, imgIRequest **_retval)
{
  NS_ASSERTION(channel, "imgLoader::LoadImageWithChannel -- NULL channel pointer");

  
  
  
  *_retval = nsnull;

  nsresult rv;
  imgRequest *request = nsnull;

  nsCOMPtr<nsIURI> uri;
  channel->GetURI(getter_AddRefs(uri));

  nsCOMPtr<nsICacheEntryDescriptor> entry;
  PRBool bHasExpired;

  imgCache::Get(uri, &bHasExpired, &request, getter_AddRefs(entry)); 

  nsLoadFlags requestFlags = nsIRequest::LOAD_NORMAL;

  channel->GetLoadFlags(&requestFlags);

  if (request) {
    PRBool bUseCacheCopy = PR_TRUE;

    
    if (requestFlags & nsIRequest::LOAD_BYPASS_CACHE) {
      bUseCacheCopy = PR_FALSE;
    }
    else if (RevalidateEntry(entry, requestFlags, bHasExpired)) {
      nsCOMPtr<nsICachingChannel> cacheChan(do_QueryInterface(channel));
      if (cacheChan) {
        cacheChan->IsFromCache(&bUseCacheCopy);
      } else {
        bUseCacheCopy = PR_FALSE;
      }
    }

    if (!bUseCacheCopy) {
      
      
      
      entry->Doom();
      entry = nsnull;
      request->RemoveFromCache();
      NS_RELEASE(request);
    }
  }

  nsCOMPtr<nsILoadGroup> loadGroup;
  channel->GetLoadGroup(getter_AddRefs(loadGroup));

  if (request) {
    

    channel->Cancel(NS_IMAGELIB_ERROR_LOAD_ABORTED); 

    *listener = nsnull; 
  } else {
    
    
    
    
    nsIThread *thread = NS_GetCurrentThread();

    NS_NEWXPCOM(request, imgRequest);
    if (!request) return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(request);

    imgCache::Put(uri, request, getter_AddRefs(entry));

    
    
    
    nsCOMPtr<nsIURI> originalURI;
    channel->GetOriginalURI(getter_AddRefs(originalURI));
    request->Init(originalURI, channel, entry, thread, aCX);

    ProxyListener *pl = new ProxyListener(static_cast<nsIStreamListener *>(request));
    if (!pl) {
      NS_RELEASE(request);
      return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(pl);

    *listener = static_cast<nsIStreamListener*>(pl);
    NS_ADDREF(*listener);

    NS_RELEASE(pl);
  }

  
  requestFlags &= 0xFFFF;

  rv = CreateNewProxyForRequest(request, loadGroup, aObserver,
                                requestFlags, nsnull, _retval);
  request->NotifyProxyListener(static_cast<imgRequestProxy*>(*_retval));

  NS_RELEASE(request);

  return rv;
}


nsresult
imgLoader::CreateNewProxyForRequest(imgRequest *aRequest, nsILoadGroup *aLoadGroup,
                                    imgIDecoderObserver *aObserver,
                                    nsLoadFlags aLoadFlags, imgIRequest *aProxyRequest,
                                    imgIRequest **_retval)
{
  LOG_SCOPE_WITH_PARAM(gImgLog, "imgLoader::CreateNewProxyForRequest", "imgRequest", aRequest);

  




  imgRequestProxy *proxyRequest;
  if (aProxyRequest) {
    proxyRequest = static_cast<imgRequestProxy *>(aProxyRequest);
  } else {
    NS_NEWXPCOM(proxyRequest, imgRequestProxy);
    if (!proxyRequest) return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(proxyRequest);

  


  proxyRequest->SetLoadFlags(aLoadFlags);

  
  nsresult rv = proxyRequest->Init(aRequest, aLoadGroup, aObserver);
  if (NS_FAILED(rv)) {
    NS_RELEASE(proxyRequest);
    return rv;
  }

  if (*_retval) {
    (*_retval)->Cancel(NS_IMAGELIB_ERROR_LOAD_ABORTED);
    NS_RELEASE(*_retval);
  }
  
  *_retval = static_cast<imgIRequest*>(proxyRequest);

  return NS_OK;
}

NS_IMETHODIMP imgLoader::SupportImageWithMimeType(const char* aMimeType, PRBool *_retval)
{
  *_retval = PR_FALSE;
  nsCOMPtr<nsIComponentRegistrar> reg;
  nsresult rv = NS_GetComponentRegistrar(getter_AddRefs(reg));
  if (NS_FAILED(rv))
    return rv;
  nsCAutoString mimeType(aMimeType);
  ToLowerCase(mimeType);
  nsCAutoString decoderId(NS_LITERAL_CSTRING("@mozilla.org/image/decoder;2?type=") + mimeType);
  return reg->IsContractIDRegistered(decoderId.get(),  _retval);
}

NS_IMETHODIMP imgLoader::GetMIMETypeFromContent(nsIRequest* aRequest,
                                                const PRUint8* aContents,
                                                PRUint32 aLength,
                                                nsACString& aContentType)
{
  return GetMimeTypeFromContent((const char*)aContents, aLength, aContentType);
}


nsresult imgLoader::GetMimeTypeFromContent(const char* aContents, PRUint32 aLength, nsACString& aContentType)
{
  
  if (aLength >= 4 && !nsCRT::strncmp(aContents, "GIF8", 4))  {
    aContentType.AssignLiteral("image/gif");
  }

  
  else if (aLength >= 4 && ((unsigned char)aContents[0]==0x89 &&
                   (unsigned char)aContents[1]==0x50 &&
                   (unsigned char)aContents[2]==0x4E &&
                   (unsigned char)aContents[3]==0x47))
  { 
    aContentType.AssignLiteral("image/png");
  }

  
  





  else if (aLength >= 3 &&
     ((unsigned char)aContents[0])==0xFF &&
     ((unsigned char)aContents[1])==0xD8 &&
     ((unsigned char)aContents[2])==0xFF)
  {
    aContentType.AssignLiteral("image/jpeg");
  }

  
  


  else if (aLength >= 5 &&
   ((unsigned char) aContents[0])==0x4a &&
   ((unsigned char) aContents[1])==0x47 &&
   ((unsigned char) aContents[4])==0x00 )
  {
    aContentType.AssignLiteral("image/x-jg");
  }

  else if (aLength >= 2 && !nsCRT::strncmp(aContents, "BM", 2)) {
    aContentType.AssignLiteral("image/bmp");
  }

  
  
  else if (aLength >= 4 && (!memcmp(aContents, "\000\000\001\000", 4) ||
                            !memcmp(aContents, "\000\000\002\000", 4))) {
    aContentType.AssignLiteral("image/x-icon");
  }

  else if (aLength >= 8 && !nsCRT::strncmp(aContents, "#define ", 8)) {
    aContentType.AssignLiteral("image/x-xbitmap");
  }
  else {
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}





#include "nsIRequest.h"
#include "nsIStreamConverterService.h"
#include "nsXPIDLString.h"

NS_IMPL_ISUPPORTS2(ProxyListener, nsIStreamListener, nsIRequestObserver)

ProxyListener::ProxyListener(nsIStreamListener *dest) :
  mDestListener(dest)
{
  
}

ProxyListener::~ProxyListener()
{
  
}





NS_IMETHODIMP ProxyListener::OnStartRequest(nsIRequest *aRequest, nsISupports *ctxt)
{
  if (!mDestListener)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel) {
    nsCAutoString contentType;
    nsresult rv = channel->GetContentType(contentType);

    if (!contentType.IsEmpty()) {
     



      if (NS_LITERAL_CSTRING("multipart/x-mixed-replace").Equals(contentType)) {

        nsCOMPtr<nsIStreamConverterService> convServ(do_GetService("@mozilla.org/streamConverters;1", &rv));
        if (NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIStreamListener> toListener(mDestListener);
          nsCOMPtr<nsIStreamListener> fromListener;

          rv = convServ->AsyncConvertData("multipart/x-mixed-replace",
                                          "*/*",
                                          toListener,
                                          nsnull,
                                          getter_AddRefs(fromListener));
          if (NS_SUCCEEDED(rv))
            mDestListener = fromListener;
        }
      }
    }
  }

  return mDestListener->OnStartRequest(aRequest, ctxt);
}


NS_IMETHODIMP ProxyListener::OnStopRequest(nsIRequest *aRequest, nsISupports *ctxt, nsresult status)
{
  if (!mDestListener)
    return NS_ERROR_FAILURE;

  return mDestListener->OnStopRequest(aRequest, ctxt, status);
}




NS_IMETHODIMP ProxyListener::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
  if (!mDestListener)
    return NS_ERROR_FAILURE;

  return mDestListener->OnDataAvailable(aRequest, ctxt, inStr, sourceOffset, count);
}









NS_IMPL_ISUPPORTS2(imgCacheValidator, nsIStreamListener, nsIRequestObserver)

imgCacheValidator::imgCacheValidator(imgRequest *request, void *aContext) :
  mContext(aContext)
{
  

  mRequest = request;
  NS_ADDREF(mRequest);
}

imgCacheValidator::~imgCacheValidator()
{
  
  if (mRequest) {
    mRequest->mValidator = nsnull;
    NS_RELEASE(mRequest);
  }
}

void imgCacheValidator::AddProxy(imgRequestProxy *aProxy)
{
  
  
  aProxy->AddToLoadGroup();

  mProxies.AppendObject(aProxy);
}




NS_IMETHODIMP imgCacheValidator::OnStartRequest(nsIRequest *aRequest, nsISupports *ctxt)
{
  nsCOMPtr<nsICachingChannel> cacheChan(do_QueryInterface(aRequest));
  if (cacheChan) {
    PRBool isFromCache;
    if (NS_SUCCEEDED(cacheChan->IsFromCache(&isFromCache)) && isFromCache) {

      PRUint32 count = mProxies.Count();
      for (PRInt32 i = count-1; i>=0; i--) {
        imgRequestProxy *proxy = static_cast<imgRequestProxy *>(mProxies[i]);
        mRequest->NotifyProxyListener(proxy);
      }

      mRequest->SetLoadId(mContext);
      mRequest->mValidator = nsnull;

      NS_RELEASE(mRequest); 

      return NS_OK;
    }
  }
  
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  nsCOMPtr<nsICacheEntryDescriptor> entry;
  nsCOMPtr<nsIURI> uri;

  
  mRequest->RemoveFromCache();

  mRequest->GetURI(getter_AddRefs(uri));

  mRequest->mValidator = nsnull;
  NS_RELEASE(mRequest); 

  imgRequest *request;
  NS_NEWXPCOM(request, imgRequest);
  if (!request) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(request);

  imgCache::Put(uri, request, getter_AddRefs(entry));

  
  
  
  nsCOMPtr<nsIURI> originalURI;
  channel->GetOriginalURI(getter_AddRefs(originalURI));
  request->Init(originalURI, channel, entry, NS_GetCurrentThread(), mContext);

  ProxyListener *pl = new ProxyListener(static_cast<nsIStreamListener *>(request));
  if (!pl) {
    NS_RELEASE(request);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mDestListener = static_cast<nsIStreamListener*>(pl);

  PRUint32 count = mProxies.Count();
  for (PRInt32 i = count-1; i>=0; i--) {
    imgRequestProxy *proxy = static_cast<imgRequestProxy *>(mProxies[i]);
    proxy->ChangeOwner(request);
    request->NotifyProxyListener(proxy);
  }

  NS_RELEASE(request);

  if (!mDestListener)
    return NS_OK;

  return mDestListener->OnStartRequest(aRequest, ctxt);
}


NS_IMETHODIMP imgCacheValidator::OnStopRequest(nsIRequest *aRequest, nsISupports *ctxt, nsresult status)
{
  if (!mDestListener)
    return NS_OK;

  return mDestListener->OnStopRequest(aRequest, ctxt, status);
}





static NS_METHOD dispose_of_data(nsIInputStream* in, void* closure,
                                 const char* fromRawSegment, PRUint32 toOffset,
                                 PRUint32 count, PRUint32 *writeCount)
{
  *writeCount = count;
  return NS_OK;
}


NS_IMETHODIMP imgCacheValidator::OnDataAvailable(nsIRequest *aRequest, nsISupports *ctxt, nsIInputStream *inStr, PRUint32 sourceOffset, PRUint32 count)
{
#ifdef DEBUG
  nsCOMPtr<nsICachingChannel> cacheChan(do_QueryInterface(aRequest));
  if (cacheChan) {
    PRBool isFromCache;
    if (NS_SUCCEEDED(cacheChan->IsFromCache(&isFromCache)) && isFromCache)
      NS_ERROR("OnDataAvailable not suppressed by LOAD_ONLY_IF_MODIFIED load flag");
  }
#endif

  if (!mDestListener) {
    
    PRUint32 _retval;
    inStr->ReadSegments(dispose_of_data, nsnull, count, &_retval);
    return NS_OK;
  }

  return mDestListener->OnDataAvailable(aRequest, ctxt, inStr, sourceOffset, count);
}
