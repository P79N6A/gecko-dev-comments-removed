







































#include "nsHttp.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/net/HttpChannelChild.h"

#include "nsStringStream.h"
#include "nsHttpHandler.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"


#define ENSURE_CALLED_BEFORE_ASYNC_OPEN()                                      \
  if (mIsPending)                                                              \
    DROP_DEAD();                                                               \
  if (mWasOpened)                                                              \
    DROP_DEAD();                                                               \
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);                           \
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);


namespace mozilla {
namespace net {


HttpChannelChild::HttpChannelChild()
  : mState(HCC_NEW)
  
  , mLoadFlags(LOAD_NORMAL)
  , mStatus(NS_OK)
  , mIsPending(PR_FALSE)
  , mWasOpened(PR_FALSE)
{
  LOG(("Creating HttpChannelChild @%x\n", this));

  
  NS_ADDREF(gHttpHandler);
}

HttpChannelChild::~HttpChannelChild()
{
  LOG(("Destroying HttpChannelChild @%x\n", this));

  
  NS_RELEASE(gHttpHandler);
}

nsresult
HttpChannelChild::Init(nsIURI *uri)
{
  


  LOG(("HttpChannelChild::Init [this=%x]\n", this));

  NS_PRECONDITION(uri, "null uri");

  nsresult rv = nsHashPropertyBag::Init();
  if (NS_FAILED(rv))
    return rv;

  mURI = uri;
  mOriginalURI = uri;
  mDocumentURI = nsnull;


  
  
  
  nsCAutoString host;
  PRInt32 port = -1;
  PRBool usingSSL = PR_FALSE;
  
  rv = mURI->SchemeIs("https", &usingSSL);
  if (NS_FAILED(rv)) return rv;

  rv = mURI->GetAsciiHost(host);
  if (NS_FAILED(rv)) return rv;

  
  if (host.IsEmpty())
    return NS_ERROR_MALFORMED_URI;

  rv = mURI->GetPort(&port);
  if (NS_FAILED(rv)) return rv;

  LOG(("host=%s port=%d\n", host.get(), port));

  rv = mURI->GetAsciiSpec(mSpec);
  if (NS_FAILED(rv)) return rv;
  LOG(("uri=%s\n", mSpec.get()));

#if 0
  
  mConnectionInfo = new nsHttpConnectionInfo(host, port,
                                             proxyInfo, usingSSL);
  if (!mConnectionInfo)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(mConnectionInfo);
#endif

  
  mRequestHead.SetMethod(nsHttp::Get);

#if 0
  
  
  

  
  
  
  nsCAutoString hostLine;
  if (strchr(host.get(), ':')) {
    
    hostLine.Assign('[');
    
    int scopeIdPos = host.FindChar('%');
    if (scopeIdPos == kNotFound)
      hostLine.Append(host);
    else if (scopeIdPos > 0)
      hostLine.Append(Substring(host, 0, scopeIdPos));
    else
      return NS_ERROR_MALFORMED_URI;
    hostLine.Append(']');
  }
  else
    hostLine.Assign(host);
  if (port != -1) {
    hostLine.Append(':');
    hostLine.AppendInt(port);
  }

  rv = mRequestHead.SetHeader(nsHttp::Host, hostLine);
  if (NS_FAILED(rv)) return rv;

  rv = gHttpHandler->
    AddStandardRequestHeaders(&mRequestHead.Headers(), caps,
                              !mConnectionInfo->UsingSSL() &&
                              mConnectionInfo->UsingHttpProxy());
#endif 

  return rv;
}





NS_IMPL_ADDREF_INHERITED(HttpChannelChild, nsHashPropertyBag)
NS_IMPL_RELEASE_INHERITED(HttpChannelChild, nsHashPropertyBag)

NS_INTERFACE_MAP_BEGIN(HttpChannelChild)
  NS_INTERFACE_MAP_ENTRY(nsIRequest)
  NS_INTERFACE_MAP_ENTRY(nsIChannel)
  NS_INTERFACE_MAP_ENTRY(nsIHttpChannel)
  NS_INTERFACE_MAP_ENTRY(nsIHttpChannelInternal)
  NS_INTERFACE_MAP_ENTRY(nsICachingChannel)
  NS_INTERFACE_MAP_ENTRY(nsIUploadChannel)
  NS_INTERFACE_MAP_ENTRY(nsIUploadChannel2)
  NS_INTERFACE_MAP_ENTRY(nsIEncodedChannel)
  NS_INTERFACE_MAP_ENTRY(nsIResumableChannel)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY(nsIProxiedChannel)
  NS_INTERFACE_MAP_ENTRY(nsITraceableChannel)
  NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheContainer)
  NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheChannel)
NS_INTERFACE_MAP_END_INHERITING(nsHashPropertyBag)





bool 
HttpChannelChild::RecvOnStartRequest(const nsHttpResponseHead& responseHead)
{
  LOG(("HttpChannelChild::RecvOnStartRequest [this=%x]\n", this));

  mState = HCC_ONSTART;

  mResponseHead = new nsHttpResponseHead(responseHead);

  nsresult rv = mChildListener->OnStartRequest(this, mChildListenerContext);
  if (!NS_SUCCEEDED(rv)) {
    
    
    
    
    
    return false;  
  }
  return true;
}

bool 
HttpChannelChild::RecvOnDataAvailable(const nsCString& data,
                                      const PRUint32& offset,
                                      const PRUint32& count)
{
  LOG(("HttpChannelChild::RecvOnDataAvailable [this=%x]\n", this));

  mState = HCC_ONDATA;

  
  
  
  
  
  nsCOMPtr<nsIInputStream> stringStream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stringStream),
                                      data.get(),
                                      count,
                                      NS_ASSIGNMENT_DEPEND);
  if (!NS_SUCCEEDED(rv)) {
    
    return false;  
  }
  rv = mChildListener->OnDataAvailable(this, mChildListenerContext,
                                       stringStream, offset, count);
  stringStream->Close();
  if (!NS_SUCCEEDED(rv)) {
    
    return false; 
  }
  return true;
}

bool 
HttpChannelChild::RecvOnStopRequest(const nsresult& statusCode)
{
  LOG(("HttpChannelChild::RecvOnStopRequest [this=%x status=%u]\n", 
           this, statusCode));

  mState = HCC_ONSTOP;

  mIsPending = PR_FALSE;
  mStatus = statusCode;
  nsresult rv = mChildListener->OnStopRequest(this, mChildListenerContext, 
                                              statusCode);
  mChildListener = 0;
  mChildListenerContext = 0;
  if (!NS_SUCCEEDED(rv)) {
    
    return false;  
  }
  return true;
}





NS_IMETHODIMP
HttpChannelChild::GetName(nsACString& aName)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::IsPending(PRBool *retval)
{
  


  NS_ENSURE_ARG_POINTER(retval);
  *retval = mIsPending;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetStatus(nsresult *aStatus)
{
  


  NS_ENSURE_ARG_POINTER(aStatus);
  *aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::Cancel(nsresult aStatus)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::Suspend()
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::Resume()
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  


  NS_ENSURE_ARG_POINTER(aLoadFlags);
  *aLoadFlags = mLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();
  


  mLoadFlags = aLoadFlags;
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::GetOriginalURI(nsIURI **originalURI)
{
  


  NS_ENSURE_ARG_POINTER(originalURI);
  *originalURI = mOriginalURI;
  NS_ADDREF(*originalURI);
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::SetOriginalURI(nsIURI *originalURI)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();
  


  NS_ENSURE_ARG_POINTER(originalURI);
  mOriginalURI = originalURI;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetURI(nsIURI **URI)
{
  


  NS_ENSURE_ARG_POINTER(URI);
  *URI = mURI;
  NS_IF_ADDREF(*URI);
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetOwner(nsISupports **aOwner)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetOwner(nsISupports *aOwner)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetNotificationCallbacks(nsIInterfaceRequestor **callbacks)
{
  


  NS_IF_ADDREF(*callbacks = mCallbacks);
  return NS_OK;
}
NS_IMETHODIMP
HttpChannelChild::SetNotificationCallbacks(nsIInterfaceRequestor *callbacks)
{
  


  mCallbacks = callbacks;
  mProgressSink = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetSecurityInfo(nsISupports **aSecurityInfo)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetContentType(nsACString& value)
{
  if (!mResponseHead) {
    value.Truncate();
    return NS_ERROR_NOT_AVAILABLE;
  }
  if (mResponseHead->ContentType().IsEmpty()) {
    value.AssignLiteral(UNKNOWN_CONTENT_TYPE);
  } else {
    value = mResponseHead->ContentType();
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::SetContentType(const nsACString& aContentType)
{
  return BaseClassSetContentType_HACK(aContentType);
}

nsresult
HttpChannelChild::BaseClassSetContentType_HACK(const nsACString &value)
{
  if (mChildListener || mWasOpened) {
    if (!mResponseHead)
      return NS_ERROR_NOT_AVAILABLE;

    nsCAutoString contentTypeBuf, charsetBuf;
    PRBool hadCharset;
    net_ParseContentType(value, contentTypeBuf, charsetBuf, &hadCharset);

    mResponseHead->SetContentType(contentTypeBuf);

    
    if (hadCharset)
      mResponseHead->SetContentCharset(charsetBuf);
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetContentCharset(nsACString& aContentCharset)
{
  return BaseClassGetContentCharset_HACK(aContentCharset);
}

nsresult
HttpChannelChild::BaseClassGetContentCharset_HACK(nsACString &value)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;

  value = mResponseHead->ContentCharset();
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::SetContentCharset(const nsACString& aContentCharset)
{
  return BaseClassSetContentCharset_HACK(aContentCharset);
}

nsresult
HttpChannelChild::BaseClassSetContentCharset_HACK(const nsACString &value)
{
  if (mChildListener) {
    if (!mResponseHead)
      return NS_ERROR_NOT_AVAILABLE;

    mResponseHead->SetContentCharset(value);
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetContentLength(PRInt32 *aContentLength)
{
  *aContentLength = mResponseHead->ContentLength();
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::SetContentLength(PRInt32 aContentLength)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::Open(nsIInputStream **retval)
{
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_IN_PROGRESS);
  return NS_ImplementChannelOpen(this, retval);
}

NS_IMETHODIMP
HttpChannelChild::AsyncOpen(nsIStreamListener *listener, nsISupports *aContext)
{
  LOG(("HttpChannelChild::AsyncOpen [this=%x uri=%s]\n", this, mSpec.get()));

  NS_ENSURE_TRUE(gNeckoChild != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_ARG_POINTER(listener);
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  
  
  nsresult rv;
  rv = NS_CheckPortSafety(mURI);
  if (NS_FAILED(rv))
    return rv;

  
  this->AddRef();

  
  if (!gNeckoChild->SendPHttpChannelConstructor(this)) {
    
    DROP_DEAD();
  }
  mChildListener = listener;
  mChildListenerContext = aContext;

  

  

  

  
  nsCAutoString charset;
  mURI->GetOriginCharset(charset);
  nsCAutoString originalSpec;
  mOriginalURI->GetSpec(originalSpec);
  nsCAutoString originalCharset;
  mOriginalURI->GetOriginCharset(originalCharset);

  nsCAutoString docSpec;
  nsCAutoString docCharset;
  if (mDocumentURI) {
    mDocumentURI->GetSpec(docSpec);
    mDocumentURI->GetOriginCharset(docCharset);
  }

  if (!SendAsyncOpen(mSpec, charset, originalSpec, originalCharset, 
                     docSpec, docCharset, mLoadFlags, mRequestHeaders)) {
    
    
    mChildListener = 0;
    mChildListenerContext = 0;
    return NS_ERROR_FAILURE;
  }

  mIsPending = PR_TRUE;
  mWasOpened = PR_TRUE;
  mState = HCC_OPENED;

  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::GetRequestMethod(nsACString& method)
{
  


  method = mRequestHead.Method();
  return NS_OK;
}
NS_IMETHODIMP
HttpChannelChild::SetRequestMethod(const nsACString& method)
{
  



  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

  const nsCString& flatMethod = PromiseFlatCString(method);

  
  if (!nsHttp::IsValidToken(flatMethod))
    return NS_ERROR_INVALID_ARG;

  nsHttpAtom atom = nsHttp::ResolveAtom(flatMethod.get());
  if (!atom)
    return NS_ERROR_FAILURE;

  mRequestHead.SetMethod(atom);
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetReferrer(nsIURI **aReferrer)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetReferrer(nsIURI *aReferrer)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetRequestHeader(const nsACString& hdr, nsACString& val)
{
  return BaseClassGetRequestHeader_HACK(hdr, val);
}

nsresult
HttpChannelChild::BaseClassGetRequestHeader_HACK(const nsACString &header,
                                                 nsACString &value)
{
  
  

  nsHttpAtom atom = nsHttp::ResolveAtom(header);
  if (!atom)
    return NS_ERROR_NOT_AVAILABLE;

  return mRequestHead.GetHeader(atom, value);
}

NS_IMETHODIMP
HttpChannelChild::SetRequestHeader(const nsACString& aHeader, 
                                   const nsACString& aValue, 
                                   PRBool aMerge)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();

  nsresult rv = BaseClassSetRequestHeader_HACK(aHeader, aValue, aMerge);
  if (NS_FAILED(rv))
    return rv;

  RequestHeaderTuple* tuple = mRequestHeaders.AppendElement();
  if (!tuple)
    return NS_ERROR_OUT_OF_MEMORY;

  tuple->mHeader = aHeader;
  tuple->mValue = aValue;
  tuple->mMerge = aMerge;
  return NS_OK;
}

nsresult
HttpChannelChild::BaseClassSetRequestHeader_HACK(const nsACString &header,
                                                 const nsACString &value,
                                                 PRBool merge)
{
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);

  const nsCString &flatHeader = PromiseFlatCString(header);
  const nsCString &flatValue  = PromiseFlatCString(value);

  LOG(("nsHttpChannel::SetRequestHeader [this=%x header=\"%s\" value=\"%s\" merge=%u]\n",
       this, flatHeader.get(), flatValue.get(), merge));

  
  if (!nsHttp::IsValidToken(flatHeader))
    return NS_ERROR_INVALID_ARG;

  
  
  
  
  
  if (flatValue.FindCharInSet("\r\n") != kNotFound ||
      flatValue.Length() != strlen(flatValue.get()))
    return NS_ERROR_INVALID_ARG;

  nsHttpAtom atom = nsHttp::ResolveAtom(flatHeader.get());
  if (!atom) {
    NS_WARNING("failed to resolve atom");
    return NS_ERROR_NOT_AVAILABLE;
  }

  return mRequestHead.SetHeader(atom, flatValue, merge);
}

NS_IMETHODIMP
HttpChannelChild::VisitRequestHeaders(nsIHttpHeaderVisitor *aVisitor)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetAllowPipelining(PRBool *aAllowPipelining)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetAllowPipelining(PRBool aAllowPipelining)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetRedirectionLimit(PRUint32 *aRedirectionLimit)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetRedirectionLimit(PRUint32 aRedirectionLimit)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetResponseStatus(PRUint32 *value)
{
  NS_ENSURE_ARG_POINTER(value);
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  *value = mResponseHead->Status();
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetResponseStatusText(nsACString& value)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  value = mResponseHead->StatusText();
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetRequestSucceeded(PRBool *value)
{
  NS_PRECONDITION(value, "Don't ever pass a null arg to this function");
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  PRUint32 status = mResponseHead->Status();
  *value = (status / 100 == 2);
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetResponseHeader(const nsACString& header, nsACString& val)
{
  return BaseClassGetResponseHeader_HACK(header, val);
}

nsresult
HttpChannelChild::BaseClassGetResponseHeader_HACK(const nsACString &header,
                                                  nsACString &value)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  nsHttpAtom atom = nsHttp::ResolveAtom(header);
  if (!atom)
    return NS_ERROR_NOT_AVAILABLE;
  return mResponseHead->GetHeader(atom, value);
}

NS_IMETHODIMP
HttpChannelChild::SetResponseHeader(const nsACString& header, 
                                    const nsACString& value, 
                                    PRBool merge)
{
  return BaseClassSetResponseHeader_HACK(header, value, merge);
}

nsresult
HttpChannelChild::BaseClassSetResponseHeader_HACK(const nsACString &header,
                                                  const nsACString &value,
                                                  PRBool merge)
{
  LOG(("nsHttpChannel::SetResponseHeader [this=%x header=\"%s\" value=\"%s\" merge=%u]\n",
       this, PromiseFlatCString(header).get(), PromiseFlatCString(value).get(), merge));

  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  nsHttpAtom atom = nsHttp::ResolveAtom(header);
  if (!atom)
    return NS_ERROR_NOT_AVAILABLE;

  
  if (atom == nsHttp::Content_Type ||
      atom == nsHttp::Content_Length ||
      atom == nsHttp::Content_Encoding ||
      atom == nsHttp::Trailer ||
      atom == nsHttp::Transfer_Encoding)
    return NS_ERROR_ILLEGAL_VALUE;

  return mResponseHead->SetHeader(atom, value, merge);
}

NS_IMETHODIMP
HttpChannelChild::VisitResponseHeaders(nsIHttpHeaderVisitor *aVisitor)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::IsNoStoreResponse(PRBool *retval)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::IsNoCacheResponse(PRBool *retval)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetDocumentURI(nsIURI **aDocumentURI)
{
  


  NS_ENSURE_ARG_POINTER(aDocumentURI);
  *aDocumentURI = mDocumentURI;
  NS_IF_ADDREF(*aDocumentURI);
  return NS_OK;
}
NS_IMETHODIMP
HttpChannelChild::SetDocumentURI(nsIURI *aDocumentURI)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();
  


  mDocumentURI = aDocumentURI;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetRequestVersion(PRUint32 *major, PRUint32 *minor)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetResponseVersion(PRUint32 *major, PRUint32 *minor)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::SetCookie(const char *aCookieHeader)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::SetupFallbackChannel(const char *aFallbackKey)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetForceAllowThirdPartyCookie(PRBool *force)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::SetForceAllowThirdPartyCookie(PRBool force)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetCacheToken(nsISupports **aCacheToken)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetCacheToken(nsISupports *aCacheToken)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetOfflineCacheToken(nsISupports **aOfflineCacheToken)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetOfflineCacheToken(nsISupports *aOfflineCacheToken)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetCacheKey(nsISupports **aCacheKey)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetCacheKey(nsISupports *aCacheKey)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetCacheAsFile(PRBool *aCacheAsFile)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetCacheAsFile(PRBool aCacheAsFile)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetCacheForOfflineUse(PRBool *aCacheForOfflineUse)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetCacheForOfflineUse(PRBool aCacheForOfflineUse)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetOfflineCacheClientID(nsACString& id)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetOfflineCacheClientID(const nsACString& id)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetCacheFile(nsIFile **aCacheFile)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::IsFromCache(PRBool *retval)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::SetUploadStream(nsIInputStream *aStream, 
                                  const nsACString& aContentType, 
                                  PRInt32 aContentLength)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetUploadStream(nsIInputStream **aUploadStream)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::ExplicitSetUploadStream(nsIInputStream *aStream, 
                                          const nsACString& aContentType, 
                                          PRInt64 aContentLength, 
                                          const nsACString& aMethod, 
                                          PRBool aStreamHasHeaders)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetContentEncodings(nsIUTF8StringEnumerator **result)
{
  DROP_DEAD();
}


NS_IMETHODIMP
HttpChannelChild::GetApplyConversion(PRBool *aApplyConversion)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::SetApplyConversion(PRBool aApplyConversion)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::ResumeAt(PRUint64 startPos, const nsACString& entityID)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetEntityID(nsACString& aEntityID)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetPriority(PRInt32 *aPriority)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetPriority(PRInt32 aPriority)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::AdjustPriority(PRInt32 delta)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetProxyInfo(nsIProxyInfo **aProxyInfo)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::SetNewListener(nsIStreamListener *listener, 
                                 nsIStreamListener **oldListener)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetApplicationCache(nsIApplicationCache **aApplicationCache)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetApplicationCache(nsIApplicationCache *aApplicationCache)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetLoadedFromApplicationCache(PRBool *retval)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetInheritApplicationCache(PRBool *aInheritApplicationCache)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetInheritApplicationCache(PRBool aInheritApplicationCache)
{
  
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetChooseApplicationCache(PRBool *aChooseApplicationCache)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetChooseApplicationCache(PRBool aChooseApplicationCache)
{
  
  return NS_OK;
}



}} 

