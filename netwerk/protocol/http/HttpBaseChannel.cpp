









































#include "mozilla/net/HttpBaseChannel.h"

#include "nsHttpHandler.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"

#include "nsICachingChannel.h"
#include "nsISeekableStream.h"
#include "nsIEncodedChannel.h"
#include "nsIResumableChannel.h"
#include "nsIApplicationCacheChannel.h"
#include "nsEscape.h"
#include "nsPrintfCString.h"

namespace mozilla {
namespace net {

HttpBaseChannel::HttpBaseChannel()
  : mStartPos(LL_MAXUINT)
  , mStatus(NS_OK)
  , mLoadFlags(LOAD_NORMAL)
  , mPriority(PRIORITY_NORMAL)
  , mCaps(0)
  , mRedirectionLimit(gHttpHandler->RedirectionLimit())
  , mApplyConversion(PR_TRUE)
  , mCanceled(PR_FALSE)
  , mIsPending(PR_FALSE)
  , mWasOpened(PR_FALSE)
  , mResponseHeadersModified(PR_FALSE)
  , mAllowPipelining(PR_TRUE)
  , mForceAllowThirdPartyCookie(PR_FALSE)
  , mUploadStreamHasHeaders(PR_FALSE)
  , mInheritApplicationCache(PR_TRUE)
  , mChooseApplicationCache(PR_FALSE)
  , mLoadedFromApplicationCache(PR_FALSE)
  , mChannelIsForDownload(PR_FALSE)
  , mRedirectedCachekeys(nsnull)
{
  LOG(("Creating HttpBaseChannel @%x\n", this));

  
  NS_ADDREF(gHttpHandler);
}

HttpBaseChannel::~HttpBaseChannel()
{
  LOG(("Destroying HttpBaseChannel @%x\n", this));

  
  CleanRedirectCacheChainIfNecessary();

  gHttpHandler->Release();
}

nsresult
HttpBaseChannel::Init(nsIURI *aURI,
                      PRUint8 aCaps,
                      nsProxyInfo *aProxyInfo)
{
  LOG(("HttpBaseChannel::Init [this=%p]\n", this));

  NS_PRECONDITION(aURI, "null uri");

  nsresult rv = nsHashPropertyBag::Init();
  if (NS_FAILED(rv)) return rv;

  mURI = aURI;
  mOriginalURI = aURI;
  mDocumentURI = nsnull;
  mCaps = aCaps;

  
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

  mConnectionInfo = new nsHttpConnectionInfo(host, port,
                                             aProxyInfo, usingSSL);
  if (!mConnectionInfo)
    return NS_ERROR_OUT_OF_MEMORY;

  
  mRequestHead.SetMethod(nsHttp::Get);

  
  nsCAutoString hostLine;
  rv = nsHttpHandler::GenerateHostPort(host, port, hostLine);
  if (NS_FAILED(rv)) return rv;

  rv = mRequestHead.SetHeader(nsHttp::Host, hostLine);
  if (NS_FAILED(rv)) return rv;

  rv = gHttpHandler->
      AddStandardRequestHeaders(&mRequestHead.Headers(), aCaps,
                                !mConnectionInfo->UsingSSL() &&
                                mConnectionInfo->UsingHttpProxy());

  return rv;
}





NS_IMPL_ISUPPORTS_INHERITED8(HttpBaseChannel,
                             nsHashPropertyBag, 
                             nsIRequest,
                             nsIChannel,
                             nsIEncodedChannel,
                             nsIHttpChannel,
                             nsIHttpChannelInternal,
                             nsIUploadChannel,
                             nsIUploadChannel2,
                             nsISupportsPriority)





NS_IMETHODIMP
HttpBaseChannel::GetName(nsACString& aName)
{
  aName = mSpec;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::IsPending(PRBool *aIsPending)
{
  NS_ENSURE_ARG_POINTER(aIsPending);
  *aIsPending = mIsPending;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetStatus(nsresult *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  *aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
  NS_ENSURE_ARG_POINTER(aLoadGroup);
  *aLoadGroup = mLoadGroup;
  NS_IF_ADDREF(*aLoadGroup);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
  mLoadGroup = aLoadGroup;
  mProgressSink = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  NS_ENSURE_ARG_POINTER(aLoadFlags);
  *aLoadFlags = mLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  mLoadFlags = aLoadFlags;
  return NS_OK;
}





NS_IMETHODIMP
HttpBaseChannel::GetOriginalURI(nsIURI **aOriginalURI)
{
  NS_ENSURE_ARG_POINTER(aOriginalURI);
  *aOriginalURI = mOriginalURI;
  NS_ADDREF(*aOriginalURI);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetOriginalURI(nsIURI *aOriginalURI)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();

  NS_ENSURE_ARG_POINTER(aOriginalURI);
  mOriginalURI = aOriginalURI;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetURI(nsIURI **aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  *aURI = mURI;
  NS_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetOwner(nsISupports **aOwner)
{
  NS_ENSURE_ARG_POINTER(aOwner);
  *aOwner = mOwner;
  NS_IF_ADDREF(*aOwner);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetOwner(nsISupports *aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks)
{
  *aCallbacks = mCallbacks;
  NS_IF_ADDREF(*aCallbacks);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks)
{
  mCallbacks = aCallbacks;
  mProgressSink = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetContentType(nsACString& aContentType)
{
  if (!mResponseHead) {
    aContentType.Truncate();
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (!mResponseHead->ContentType().IsEmpty()) {
    aContentType = mResponseHead->ContentType();
    return NS_OK;
  }

  aContentType.AssignLiteral(UNKNOWN_CONTENT_TYPE);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetContentType(const nsACString& aContentType)
{
  if (mListener || mWasOpened) {
    if (!mResponseHead)
      return NS_ERROR_NOT_AVAILABLE;

    nsCAutoString contentTypeBuf, charsetBuf;
    PRBool hadCharset;
    net_ParseContentType(aContentType, contentTypeBuf, charsetBuf, &hadCharset);

    mResponseHead->SetContentType(contentTypeBuf);

    
    if (hadCharset)
      mResponseHead->SetContentCharset(charsetBuf);

  } else {
    
    PRBool dummy;
    net_ParseContentType(aContentType, mContentTypeHint, mContentCharsetHint,
                         &dummy);
  }
  
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetContentCharset(nsACString& aContentCharset)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;

  aContentCharset = mResponseHead->ContentCharset();
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetContentCharset(const nsACString& aContentCharset)
{
  if (mListener) {
    if (!mResponseHead)
      return NS_ERROR_NOT_AVAILABLE;

    mResponseHead->SetContentCharset(aContentCharset);
  } else {
    
    mContentCharsetHint = aContentCharset;
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetContentLength(PRInt32 *aContentLength)
{
  NS_ENSURE_ARG_POINTER(aContentLength);

  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;

  
  *aContentLength = mResponseHead->ContentLength();
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetContentLength(PRInt32 value)
{
  NS_NOTYETIMPLEMENTED("HttpBaseChannel::SetContentLength");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
HttpBaseChannel::Open(nsIInputStream **aResult)
{
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_IN_PROGRESS);
  return NS_ImplementChannelOpen(this, aResult);
}





NS_IMETHODIMP
HttpBaseChannel::GetUploadStream(nsIInputStream **stream)
{
  NS_ENSURE_ARG_POINTER(stream);
  *stream = mUploadStream;
  NS_IF_ADDREF(*stream);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetUploadStream(nsIInputStream *stream,
                               const nsACString &contentType,
                               PRInt32 contentLength)
{
  
  
  
  
  
  

  if (stream) {
    if (contentType.IsEmpty()) {
      mUploadStreamHasHeaders = PR_TRUE;
      mRequestHead.SetMethod(nsHttp::Post); 
    } else {
      if (contentLength < 0) {
        
        
        stream->Available((PRUint32 *) &contentLength);
        if (contentLength < 0) {
          NS_ERROR("unable to determine content length");
          return NS_ERROR_FAILURE;
        }
      }
      
      nsCAutoString contentLengthStr;
      contentLengthStr.AppendInt(PRInt64(contentLength));
      SetRequestHeader(NS_LITERAL_CSTRING("Content-Length"), contentLengthStr, 
                       PR_FALSE);
      SetRequestHeader(NS_LITERAL_CSTRING("Content-Type"), contentType, 
                       PR_FALSE);
      mUploadStreamHasHeaders = PR_FALSE;
      mRequestHead.SetMethod(nsHttp::Put); 
    }
  } else {
    mUploadStreamHasHeaders = PR_FALSE;
    mRequestHead.SetMethod(nsHttp::Get); 
  }
  mUploadStream = stream;
  return NS_OK;
}





NS_IMETHODIMP
HttpBaseChannel::ExplicitSetUploadStream(nsIInputStream *aStream,
                                       const nsACString &aContentType,
                                       PRInt64 aContentLength,
                                       const nsACString &aMethod,
                                       PRBool aStreamHasHeaders)
{
  
  NS_ENSURE_TRUE(aStream, NS_ERROR_FAILURE);

  if (aContentLength < 0 && !aStreamHasHeaders) {
    PRUint32 streamLength;
    aStream->Available(&streamLength);
    aContentLength = streamLength;
    if (aContentLength < 0) {
      NS_ERROR("unable to determine content length");
      return NS_ERROR_FAILURE;
    }
  }

  nsresult rv = SetRequestMethod(aMethod);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aStreamHasHeaders) {
    
    nsCAutoString contentLengthStr;
    contentLengthStr.AppendInt(aContentLength);
    SetRequestHeader(NS_LITERAL_CSTRING("Content-Length"), contentLengthStr, 
                     PR_FALSE);
    SetRequestHeader(NS_LITERAL_CSTRING("Content-Type"), aContentType, 
                     PR_FALSE);
  }

  mUploadStreamHasHeaders = aStreamHasHeaders;
  mUploadStream = aStream;
  return NS_OK;
}





NS_IMETHODIMP
HttpBaseChannel::GetApplyConversion(PRBool *value)
{
  *value = mApplyConversion;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetApplyConversion(PRBool value)
{
  LOG(("HttpBaseChannel::SetApplyConversion [this=%p value=%d]\n", this, value));
  mApplyConversion = value;
  return NS_OK;
}

nsresult
HttpBaseChannel::ApplyContentConversions()
{
  if (!mResponseHead)
    return NS_OK;

  LOG(("HttpBaseChannel::ApplyContentConversions [this=%p]\n", this));

  if (!mApplyConversion) {
    LOG(("not applying conversion per mApplyConversion\n"));
    return NS_OK;
  }

  const char *val = mResponseHead->PeekHeader(nsHttp::Content_Encoding);
  if (gHttpHandler->IsAcceptableEncoding(val)) {
    nsCOMPtr<nsIStreamConverterService> serv;
    nsresult rv = gHttpHandler->
            GetStreamConverterService(getter_AddRefs(serv));
    
    
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIStreamListener> converter;
      nsCAutoString from(val);
      ToLowerCase(from);
      rv = serv->AsyncConvertData(from.get(),
                                  "uncompressed",
                                  mListener,
                                  mListenerContext,
                                  getter_AddRefs(converter));
      if (NS_SUCCEEDED(rv)) {
        LOG(("converter installed from \'%s\' to \'uncompressed\'\n", val));
        mListener = converter;
      }
    }
  } else if (val != nsnull) {
    LOG(("Unknown content encoding '%s', ignoring\n", val));
  }

  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetContentEncodings(nsIUTF8StringEnumerator** aEncodings)
{
  if (!mResponseHead) {
    *aEncodings = nsnull;
    return NS_OK;
  }
    
  const char *encoding = mResponseHead->PeekHeader(nsHttp::Content_Encoding);
  if (!encoding) {
    *aEncodings = nsnull;
    return NS_OK;
  }
  nsContentEncodings* enumerator = new nsContentEncodings(this, encoding);
  NS_ADDREF(*aEncodings = enumerator);
  return NS_OK;
}





HttpBaseChannel::nsContentEncodings::nsContentEncodings(nsIHttpChannel* aChannel,
                                                        const char* aEncodingHeader)
  : mEncodingHeader(aEncodingHeader)
  , mChannel(aChannel)
  , mReady(PR_FALSE)
{
  mCurEnd = aEncodingHeader + strlen(aEncodingHeader);
  mCurStart = mCurEnd;
}
    
HttpBaseChannel::nsContentEncodings::~nsContentEncodings()
{
}





NS_IMETHODIMP
HttpBaseChannel::nsContentEncodings::HasMore(PRBool* aMoreEncodings)
{
  if (mReady) {
    *aMoreEncodings = PR_TRUE;
    return NS_OK;
  }

  nsresult rv = PrepareForNext();
  *aMoreEncodings = NS_SUCCEEDED(rv);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::nsContentEncodings::GetNext(nsACString& aNextEncoding)
{
  aNextEncoding.Truncate();
  if (!mReady) {
    nsresult rv = PrepareForNext();
    if (NS_FAILED(rv)) {
      return NS_ERROR_FAILURE;
    }
  }

  const nsACString & encoding = Substring(mCurStart, mCurEnd);

  nsACString::const_iterator start, end;
  encoding.BeginReading(start);
  encoding.EndReading(end);

  PRBool haveType = PR_FALSE;
  if (CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("gzip"), start, end)) {
    aNextEncoding.AssignLiteral(APPLICATION_GZIP);
    haveType = PR_TRUE;
  }

  if (!haveType) {
    encoding.BeginReading(start);
    if (CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("compress"), start, end)) {
      aNextEncoding.AssignLiteral(APPLICATION_COMPRESS);
      haveType = PR_TRUE;
    }
  }
    
  if (!haveType) {
    encoding.BeginReading(start);
    if (CaseInsensitiveFindInReadable(NS_LITERAL_CSTRING("deflate"), start, end)) {
      aNextEncoding.AssignLiteral(APPLICATION_ZIP);
      haveType = PR_TRUE;
    }
  }

  
  mCurEnd = mCurStart;
  mReady = PR_FALSE;
  
  if (haveType)
    return NS_OK;

  NS_WARNING("Unknown encoding type");
  return NS_ERROR_FAILURE;
}





NS_IMPL_ISUPPORTS1(HttpBaseChannel::nsContentEncodings, nsIUTF8StringEnumerator)





nsresult
HttpBaseChannel::nsContentEncodings::PrepareForNext(void)
{
  NS_ASSERTION(mCurStart == mCurEnd, "Indeterminate state");
    
  
  
    
  while (mCurEnd != mEncodingHeader) {
    --mCurEnd;
    if (*mCurEnd != ',' && !nsCRT::IsAsciiSpace(*mCurEnd))
      break;
  }
  if (mCurEnd == mEncodingHeader)
    return NS_ERROR_NOT_AVAILABLE; 
  ++mCurEnd;
        
  
  
    
  mCurStart = mCurEnd - 1;
  while (mCurStart != mEncodingHeader &&
         *mCurStart != ',' && !nsCRT::IsAsciiSpace(*mCurStart))
    --mCurStart;
  if (*mCurStart == ',' || nsCRT::IsAsciiSpace(*mCurStart))
    ++mCurStart; 
        
  
  
  if (Substring(mCurStart, mCurEnd).Equals("identity",
                                           nsCaseInsensitiveCStringComparator())) {
    mCurEnd = mCurStart;
    return PrepareForNext();
  }
        
  mReady = PR_TRUE;
  return NS_OK;
}






NS_IMETHODIMP
HttpBaseChannel::GetRequestMethod(nsACString& aMethod)
{
  aMethod = mRequestHead.Method();
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetRequestMethod(const nsACString& aMethod)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();

  const nsCString& flatMethod = PromiseFlatCString(aMethod);

  
  if (!nsHttp::IsValidToken(flatMethod))
    return NS_ERROR_INVALID_ARG;

  nsHttpAtom atom = nsHttp::ResolveAtom(flatMethod.get());
  if (!atom)
    return NS_ERROR_FAILURE;

  mRequestHead.SetMethod(atom);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetReferrer(nsIURI **referrer)
{
  NS_ENSURE_ARG_POINTER(referrer);
  *referrer = mReferrer;
  NS_IF_ADDREF(*referrer);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetReferrer(nsIURI *referrer)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();

  
  mReferrer = nsnull;
  mRequestHead.ClearHeader(nsHttp::Referer);

  if (!referrer)
      return NS_OK;

  
  PRUint32 referrerLevel;
  if (mLoadFlags & LOAD_INITIAL_DOCUMENT_URI)
    referrerLevel = 1; 
  else
    referrerLevel = 2; 
  if (gHttpHandler->ReferrerLevel() < referrerLevel)
    return NS_OK;

  nsCOMPtr<nsIURI> referrerGrip;
  nsresult rv;
  PRBool match;

  
  
  
  
  
  
  
  
  rv = referrer->SchemeIs("wyciwyg", &match);
  if (NS_FAILED(rv)) return rv;
  if (match) {
    nsCAutoString path;
    rv = referrer->GetPath(path);
    if (NS_FAILED(rv)) return rv;

    PRUint32 pathLength = path.Length();
    if (pathLength <= 2) return NS_ERROR_FAILURE;

    
    
    
    PRInt32 slashIndex = path.FindChar('/', 2);
    if (slashIndex == kNotFound) return NS_ERROR_FAILURE;

    
    nsCAutoString charset;
    referrer->GetOriginCharset(charset);

    
    rv = NS_NewURI(getter_AddRefs(referrerGrip),
                   Substring(path, slashIndex + 1, pathLength - slashIndex - 1),
                   charset.get());
    if (NS_FAILED(rv)) return rv;

    referrer = referrerGrip.get();
  }

  
  
  
  static const char *const referrerWhiteList[] = {
    "http",
    "https",
    "ftp",
    "gopher",
    nsnull
  };
  match = PR_FALSE;
  const char *const *scheme = referrerWhiteList;
  for (; *scheme && !match; ++scheme) {
    rv = referrer->SchemeIs(*scheme, &match);
    if (NS_FAILED(rv)) return rv;
  }
  if (!match)
    return NS_OK; 

  
  
  
  
  
  
  rv = referrer->SchemeIs("https", &match);
  if (NS_FAILED(rv)) return rv;
  if (match) {
    rv = mURI->SchemeIs("https", &match);
    if (NS_FAILED(rv)) return rv;
    if (!match)
      return NS_OK;

    if (!gHttpHandler->SendSecureXSiteReferrer()) {
      nsCAutoString referrerHost;
      nsCAutoString host;

      rv = referrer->GetAsciiHost(referrerHost);
      if (NS_FAILED(rv)) return rv;

      rv = mURI->GetAsciiHost(host);
      if (NS_FAILED(rv)) return rv;

      
      if (!referrerHost.Equals(host))
        return NS_OK;
    }
  }

  nsCOMPtr<nsIURI> clone;
  
  
  
  
  
  rv = referrer->Clone(getter_AddRefs(clone));
  if (NS_FAILED(rv)) return rv;

  
  clone->SetUserPass(EmptyCString());

  
  nsCOMPtr<nsIURL> url = do_QueryInterface(clone);
  if (url)
    url->SetRef(EmptyCString());

  nsCAutoString spec;
  rv = clone->GetAsciiSpec(spec);
  if (NS_FAILED(rv)) return rv;

  
  mReferrer = clone;
  mRequestHead.SetHeader(nsHttp::Referer, spec);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetRequestHeader(const nsACString& aHeader,
                                  nsACString& aValue)
{
  
  
  nsHttpAtom atom = nsHttp::ResolveAtom(aHeader);
  if (!atom)
    return NS_ERROR_NOT_AVAILABLE;

  return mRequestHead.GetHeader(atom, aValue);
}

NS_IMETHODIMP
HttpBaseChannel::SetRequestHeader(const nsACString& aHeader,
                                  const nsACString& aValue,
                                  PRBool aMerge)
{
  const nsCString &flatHeader = PromiseFlatCString(aHeader);
  const nsCString &flatValue  = PromiseFlatCString(aValue);

  LOG(("HttpBaseChannel::SetRequestHeader [this=%p header=\"%s\" value=\"%s\" merge=%u]\n",
      this, flatHeader.get(), flatValue.get(), aMerge));

  
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

  return mRequestHead.SetHeader(atom, flatValue, aMerge);
}

NS_IMETHODIMP
HttpBaseChannel::VisitRequestHeaders(nsIHttpHeaderVisitor *visitor)
{
  return mRequestHead.Headers().VisitHeaders(visitor);
}

NS_IMETHODIMP
HttpBaseChannel::GetResponseHeader(const nsACString &header, nsACString &value)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;

  nsHttpAtom atom = nsHttp::ResolveAtom(header);
  if (!atom)
    return NS_ERROR_NOT_AVAILABLE;

  return mResponseHead->GetHeader(atom, value);
}

NS_IMETHODIMP
HttpBaseChannel::SetResponseHeader(const nsACString& header, 
                                   const nsACString& value, 
                                   PRBool merge)
{
  LOG(("HttpBaseChannel::SetResponseHeader [this=%p header=\"%s\" value=\"%s\" merge=%u]\n",
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

  mResponseHeadersModified = PR_TRUE;

  return mResponseHead->SetHeader(atom, value, merge);
}

NS_IMETHODIMP
HttpBaseChannel::VisitResponseHeaders(nsIHttpHeaderVisitor *visitor)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  return mResponseHead->Headers().VisitHeaders(visitor);
}

NS_IMETHODIMP
HttpBaseChannel::GetAllowPipelining(PRBool *value)
{
  NS_ENSURE_ARG_POINTER(value);
  *value = mAllowPipelining;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetAllowPipelining(PRBool value)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();

  mAllowPipelining = value;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetRedirectionLimit(PRUint32 *value)
{
  NS_ENSURE_ARG_POINTER(value);
  *value = mRedirectionLimit;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetRedirectionLimit(PRUint32 value)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();

  mRedirectionLimit = PR_MIN(value, 0xff);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::IsNoStoreResponse(PRBool *value)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  *value = mResponseHead->NoStore();
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::IsNoCacheResponse(PRBool *value)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  *value = mResponseHead->NoCache();
  if (!*value)
    *value = mResponseHead->ExpiresInPast();
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetResponseStatus(PRUint32 *aValue)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  *aValue = mResponseHead->Status();
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetResponseStatusText(nsACString& aValue)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  aValue = mResponseHead->StatusText();
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetRequestSucceeded(PRBool *aValue)
{
  if (!mResponseHead)
    return NS_ERROR_NOT_AVAILABLE;
  PRUint32 status = mResponseHead->Status();
  *aValue = (status / 100 == 2);
  return NS_OK;
}





NS_IMETHODIMP
HttpBaseChannel::GetDocumentURI(nsIURI **aDocumentURI)
{
  NS_ENSURE_ARG_POINTER(aDocumentURI);
  *aDocumentURI = mDocumentURI;
  NS_IF_ADDREF(*aDocumentURI);
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetDocumentURI(nsIURI *aDocumentURI)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();

  mDocumentURI = aDocumentURI;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetRequestVersion(PRUint32 *major, PRUint32 *minor)
{
  nsHttpVersion version = mRequestHead.Version();

  if (major) { *major = version / 10; }
  if (minor) { *minor = version % 10; }

  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetResponseVersion(PRUint32 *major, PRUint32 *minor)
{
  if (!mResponseHead)
  {
    *major = *minor = 0; 
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsHttpVersion version = mResponseHead->Version();

  if (major) { *major = version / 10; }
  if (minor) { *minor = version % 10; }

  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetCookie(const char *aCookieHeader)
{
  if (mLoadFlags & LOAD_ANONYMOUS)
    return NS_OK;

  
  if (!(aCookieHeader && *aCookieHeader))
    return NS_OK;

  nsICookieService *cs = gHttpHandler->GetCookieService();
  NS_ENSURE_TRUE(cs, NS_ERROR_FAILURE);

  return cs->SetCookieStringFromHttp(mURI, nsnull, nsnull, aCookieHeader,
                                     mResponseHead->PeekHeader(nsHttp::Date),
                                     this);
}

NS_IMETHODIMP
HttpBaseChannel::GetForceAllowThirdPartyCookie(PRBool *aForce)
{
  *aForce = mForceAllowThirdPartyCookie;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetForceAllowThirdPartyCookie(PRBool aForce)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();

  mForceAllowThirdPartyCookie = aForce;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetCanceled(PRBool *aCanceled)
{
  *aCanceled = mCanceled;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::GetChannelIsForDownload(PRBool *aChannelIsForDownload)
{
  *aChannelIsForDownload = mChannelIsForDownload;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetChannelIsForDownload(PRBool aChannelIsForDownload)
{
  mChannelIsForDownload = aChannelIsForDownload;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::SetCacheKeysRedirectChain(nsTArray<nsCString> *cacheKeys)
{
  mRedirectedCachekeys = cacheKeys;
  return NS_OK;
}





NS_IMETHODIMP
HttpBaseChannel::GetPriority(PRInt32 *value)
{
  *value = mPriority;
  return NS_OK;
}

NS_IMETHODIMP
HttpBaseChannel::AdjustPriority(PRInt32 delta)
{
  return SetPriority(mPriority + delta);
}





NS_IMETHODIMP
HttpBaseChannel::GetEntityID(nsACString& aEntityID)
{
  
  
  if (mRequestHead.Method() != nsHttp::Get) {
    return NS_ERROR_NOT_RESUMABLE;
  }

  
  
  
  
  const char* acceptRanges =
      mResponseHead->PeekHeader(nsHttp::Accept_Ranges);
  if (acceptRanges &&
      !nsHttp::FindToken(acceptRanges, "bytes", HTTP_HEADER_VALUE_SEPS)) {
    return NS_ERROR_NOT_RESUMABLE;
  }

  PRUint64 size = LL_MAXUINT;
  nsCAutoString etag, lastmod;
  if (mResponseHead) {
    size = mResponseHead->TotalEntitySize();
    const char* cLastMod = mResponseHead->PeekHeader(nsHttp::Last_Modified);
    if (cLastMod)
      lastmod = cLastMod;
    const char* cEtag = mResponseHead->PeekHeader(nsHttp::ETag);
    if (cEtag)
      etag = cEtag;
  }
  nsCString entityID;
  NS_EscapeURL(etag.BeginReading(), etag.Length(), esc_AlwaysCopy |
               esc_FileBaseName | esc_Forced, entityID);
  entityID.Append('/');
  entityID.AppendInt(PRInt64(size));
  entityID.Append('/');
  entityID.Append(lastmod);
  

  aEntityID = entityID;

  return NS_OK;
}







void
HttpBaseChannel::AddCookiesToRequest()
{
  if (mLoadFlags & LOAD_ANONYMOUS) {
    return;
  }

  bool useCookieService = 
    (XRE_GetProcessType() == GeckoProcessType_Default);
  nsXPIDLCString cookie;
  if (useCookieService) {
    nsICookieService *cs = gHttpHandler->GetCookieService();
    if (cs) {
      cs->GetCookieStringFromHttp(mURI,
                                  nsnull,
                                  this, getter_Copies(cookie));
    }

    if (cookie.IsEmpty()) {
      cookie = mUserSetCookieHeader;
    }
    else if (!mUserSetCookieHeader.IsEmpty()) {
      cookie.Append(NS_LITERAL_CSTRING("; ") + mUserSetCookieHeader);
    }
  }
  else {
    cookie = mUserSetCookieHeader;
  }

  
  
  SetRequestHeader(nsDependentCString(nsHttp::Cookie), cookie, PR_FALSE);
}

static PLDHashOperator
CopyProperties(const nsAString& aKey, nsIVariant *aData, void *aClosure)
{
  nsIWritablePropertyBag* bag = static_cast<nsIWritablePropertyBag*>
                                           (aClosure);
  bag->SetProperty(aKey, aData);
  return PL_DHASH_NEXT;
}

nsresult
HttpBaseChannel::SetupReplacementChannel(nsIURI       *newURI, 
                                         nsIChannel   *newChannel,
                                         PRBool        preserveMethod)
{
  LOG(("HttpBaseChannel::SetupReplacementChannel "
     "[this=%p newChannel=%p preserveMethod=%d]",
     this, newChannel, preserveMethod));
  PRUint32 newLoadFlags = mLoadFlags | LOAD_REPLACE;
  
  
  
  
  
  
  if (mConnectionInfo->UsingSSL())
    newLoadFlags &= ~INHIBIT_PERSISTENT_CACHING;

  
  newLoadFlags &= ~nsICachingChannel::LOAD_CHECK_OFFLINE_CACHE;

  newChannel->SetLoadGroup(mLoadGroup); 
  newChannel->SetNotificationCallbacks(mCallbacks);
  newChannel->SetLoadFlags(newLoadFlags);

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(newChannel);
  if (!httpChannel)
    return NS_OK; 

  if (preserveMethod) {
    nsCOMPtr<nsIUploadChannel> uploadChannel =
      do_QueryInterface(httpChannel);
    nsCOMPtr<nsIUploadChannel2> uploadChannel2 =
      do_QueryInterface(httpChannel);
    if (mUploadStream && (uploadChannel2 || uploadChannel)) {
      
      nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mUploadStream);
      if (seekable)
        seekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);

      
      if (uploadChannel2) {
        const char *ctype = mRequestHead.PeekHeader(nsHttp::Content_Type);
        if (!ctype)
          ctype = "";
        const char *clen  = mRequestHead.PeekHeader(nsHttp::Content_Length);
        PRInt64 len = clen ? nsCRT::atoll(clen) : -1;
        uploadChannel2->ExplicitSetUploadStream(
                                  mUploadStream, nsDependentCString(ctype), len,
                                  nsDependentCString(mRequestHead.Method()),
                                  mUploadStreamHasHeaders);
      } else {
        if (mUploadStreamHasHeaders) {
          uploadChannel->SetUploadStream(mUploadStream, EmptyCString(),
                           -1);
        } else {
          const char *ctype =
            mRequestHead.PeekHeader(nsHttp::Content_Type);
          const char *clen =
            mRequestHead.PeekHeader(nsHttp::Content_Length);
          if (!ctype) {
            ctype = "application/octet-stream";
          }
          if (clen) {
            uploadChannel->SetUploadStream(mUploadStream,
                                           nsDependentCString(ctype),
                                           atoi(clen));
          }
        }
      }
    }
    
    
    
    

    httpChannel->SetRequestMethod(nsDependentCString(mRequestHead.Method()));
  }
  
  if (mReferrer)
    httpChannel->SetReferrer(mReferrer);
  
  httpChannel->SetAllowPipelining(mAllowPipelining);
  
  httpChannel->SetRedirectionLimit(mRedirectionLimit - 1);

  nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(newChannel);
  if (httpInternal) {
    
    httpInternal->SetForceAllowThirdPartyCookie(mForceAllowThirdPartyCookie);

    
    
    
    
    if (newURI && (mURI == mDocumentURI))
      httpInternal->SetDocumentURI(newURI);
    else
      httpInternal->SetDocumentURI(mDocumentURI);

    
    
    if (mRedirectedCachekeys) {
        LOG(("HttpBaseChannel::SetupReplacementChannel "
             "[this=%p] transferring chain of redirect cache-keys", this));
        httpInternal->SetCacheKeysRedirectChain(mRedirectedCachekeys);
        mRedirectedCachekeys = nsnull;
    }
  }
  
  
  nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
    do_QueryInterface(newChannel);
  if (appCacheChannel) {
    appCacheChannel->SetApplicationCache(mApplicationCache);
    appCacheChannel->SetInheritApplicationCache(mInheritApplicationCache);
    
  }

  
  nsCOMPtr<nsIWritablePropertyBag> bag(do_QueryInterface(newChannel));
  if (bag)
    mPropertyHash.EnumerateRead(CopyProperties, bag.get());

  return NS_OK;
}



}  
}  

