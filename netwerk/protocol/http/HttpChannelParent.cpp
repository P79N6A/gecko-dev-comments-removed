







































#include "mozilla/net/HttpChannelParent.h"
#include "mozilla/dom/TabParent.h"
#include "nsHttpChannel.h"
#include "nsHttpHandler.h"
#include "nsNetUtil.h"
#include "nsISupportsPriority.h"
#include "nsIAuthPromptProvider.h"
#include "nsIDocShellTreeItem.h"
#include "nsIBadCertListener2.h"
#include "nsICacheEntryDescriptor.h"

namespace mozilla {
namespace net {


HttpChannelParent::HttpChannelParent(PIFrameEmbeddingParent* iframeEmbedding)
: mIPCClosed(false)
{
  
  nsIHttpProtocolHandler* handler;
  CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &handler);
  NS_ASSERTION(handler, "no http handler");

  mTabParent = do_QueryInterface(static_cast<nsITabParent*>(
      static_cast<TabParent*>(iframeEmbedding)));
}

HttpChannelParent::~HttpChannelParent()
{
  gHttpHandler->Release();
}

void
HttpChannelParent::ActorDestroy(ActorDestroyReason why)
{
  
  
  mIPCClosed = true;
}





NS_IMPL_ISUPPORTS4(HttpChannelParent, 
                   nsIRequestObserver, 
                   nsIStreamListener,
                   nsIInterfaceRequestor,
                   nsIProgressEventSink);





bool 
HttpChannelParent::RecvAsyncOpen(const IPC::URI&            aURI,
                                 const IPC::URI&            aOriginalURI,
                                 const IPC::URI&            aDocURI,
                                 const IPC::URI&            aReferrerURI,
                                 const PRUint32&            loadFlags,
                                 const RequestHeaderTuples& requestHeaders,
                                 const nsHttpAtom&          requestMethod,
                                 const nsCString&           uploadStreamData,
                                 const PRInt32&             uploadStreamInfo,
                                 const PRUint16&            priority,
                                 const PRUint8&             redirectionLimit,
                                 const PRBool&              allowPipelining,
                                 const PRBool&              forceAllowThirdPartyCookie)
{
  nsCOMPtr<nsIURI> uri = aURI;
  nsCOMPtr<nsIURI> originalUri = aOriginalURI;
  nsCOMPtr<nsIURI> docUri = aDocURI;
  nsCOMPtr<nsIURI> referrerUri = aReferrerURI;
  
  nsCString uriSpec;
  uri->GetSpec(uriSpec);
  LOG(("HttpChannelParent RecvAsyncOpen [this=%x uri=%s]\n", 
       this, uriSpec.get()));

  nsresult rv;

  nsCOMPtr<nsIIOService> ios(do_GetIOService(&rv));
  if (NS_FAILED(rv))
    return false;       

  rv = NS_NewChannel(getter_AddRefs(mChannel), uri, ios, nsnull, nsnull, loadFlags);
  if (NS_FAILED(rv))
    return false;       

  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
  httpChan->SetRemoteChannel(true);

  if (originalUri)
    httpChan->SetOriginalURI(originalUri);
  if (docUri)
    httpChan->SetDocumentURI(docUri);
  if (referrerUri)
    httpChan->SetReferrerInternal(referrerUri);
  if (loadFlags != nsIRequest::LOAD_NORMAL)
    httpChan->SetLoadFlags(loadFlags);

  for (PRUint32 i = 0; i < requestHeaders.Length(); i++) {
    httpChan->SetRequestHeader(requestHeaders[i].mHeader,
                               requestHeaders[i].mValue,
                               requestHeaders[i].mMerge);
  }

  httpChan->SetNotificationCallbacks(this);

  httpChan->SetRequestMethod(nsDependentCString(requestMethod.get()));

  if (uploadStreamInfo != eUploadStream_null) {
    nsCOMPtr<nsIInputStream> stream;
    rv = NS_NewPostDataStream(getter_AddRefs(stream), false, uploadStreamData, 0);
    if (!NS_SUCCEEDED(rv)) {
      return false;   
    }
    httpChan->InternalSetUploadStream(stream);
    
    
    httpChan->SetUploadStreamHasHeaders((PRBool) uploadStreamInfo);
  }

  if (priority != nsISupportsPriority::PRIORITY_NORMAL)
    httpChan->SetPriority(priority);
  httpChan->SetRedirectionLimit(redirectionLimit);
  httpChan->SetAllowPipelining(allowPipelining);
  httpChan->SetForceAllowThirdPartyCookie(forceAllowThirdPartyCookie);

  rv = httpChan->AsyncOpen(this, nsnull);
  if (NS_FAILED(rv))
    return false;       

  return true;
}

bool 
HttpChannelParent::RecvSetPriority(const PRUint16& priority)
{
  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(mChannel.get());
  httpChan->SetPriority(priority);
  return true;
}

bool
HttpChannelParent::RecvSetCacheTokenCachedCharset(const nsCString& charset)
{
  if (mCacheDescriptor)
    mCacheDescriptor->SetMetaDataElement("charset",
                                         PromiseFlatCString(charset).get());
  return true;
}

bool
HttpChannelParent::RecvOnStopRequestCompleted()
{
  mCacheDescriptor = nsnull;
  return true;
}





NS_IMETHODIMP
HttpChannelParent::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  LOG(("HttpChannelParent::OnStartRequest [this=%x]\n", this));

  nsHttpChannel *chan = static_cast<nsHttpChannel *>(aRequest);
  nsHttpResponseHead *responseHead = chan->GetResponseHead();

  PRBool isFromCache = false;
  chan->IsFromCache(&isFromCache);
  PRUint32 expirationTime;
  chan->GetCacheTokenExpirationTime(&expirationTime);
  nsCString cachedCharset;
  chan->GetCacheTokenCachedCharset(cachedCharset);

  
  
  chan->GetCacheToken(getter_AddRefs(mCacheDescriptor));

  if (mIPCClosed || 
      !SendOnStartRequest(responseHead ? *responseHead : nsHttpResponseHead(), 
                          !!responseHead, isFromCache,
                          mCacheDescriptor ? PR_TRUE : PR_FALSE,
                          expirationTime, cachedCharset)) {
    return NS_ERROR_UNEXPECTED; 
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelParent::OnStopRequest(nsIRequest *aRequest, 
                                 nsISupports *aContext, 
                                 nsresult aStatusCode)
{
  LOG(("HttpChannelParent::OnStopRequest: [this=%x status=%ul]\n", 
       this, aStatusCode));

  if (mIPCClosed || !SendOnStopRequest(aStatusCode))
    return NS_ERROR_UNEXPECTED; 
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelParent::OnDataAvailable(nsIRequest *aRequest, 
                                   nsISupports *aContext, 
                                   nsIInputStream *aInputStream, 
                                   PRUint32 aOffset, 
                                   PRUint32 aCount)
{
  LOG(("HttpChannelParent::OnDataAvailable [this=%x]\n", this));
 
  nsresult rv;

  nsCString data;
  data.SetLength(aCount);
  char * p = data.BeginWriting();
  PRUint32 bytesRead;
  rv = aInputStream->Read(p, aCount, &bytesRead);
  data.EndWriting();
  if (!NS_SUCCEEDED(rv) || bytesRead != aCount) {
    return rv;              
  }

  if (mIPCClosed || !SendOnDataAvailable(data, aOffset, bytesRead))
    return NS_ERROR_UNEXPECTED; 
  return NS_OK;
}





NS_IMETHODIMP 
HttpChannelParent::GetInterface(const nsIID& aIID, void **result)
{
  if (aIID.Equals(NS_GET_IID(nsIAuthPromptProvider))) {
    if (!mTabParent)
      return NS_NOINTERFACE;
    return mTabParent->QueryInterface(aIID, result);
  }

  
  
  if (

      
      
      
      
      
      
      aIID.Equals(NS_GET_IID(nsIAuthPrompt2)) ||
      aIID.Equals(NS_GET_IID(nsIAuthPrompt))  ||
      
      
      
      
      
      aIID.Equals(NS_GET_IID(nsIChannelEventSink)) || 
      aIID.Equals(NS_GET_IID(nsIHttpEventSink))  ||
      
      aIID.Equals(NS_GET_IID(nsIApplicationCacheContainer)) ||
      aIID.Equals(NS_GET_IID(nsIProgressEventSink)) ||
      
      aIID.Equals(NS_GET_IID(nsIDocShellTreeItem)) ||
      
      aIID.Equals(NS_GET_IID(nsIBadCertListener2))) 
  {
    return QueryInterface(aIID, result);
  } else {
    nsPrintfCString msg(2000, 
       "HttpChannelParent::GetInterface: interface UUID=%s not yet supported! "
       "Use 'grep -ri UUID <mozilla_src>' to find the name of the interface, "
       "check http://tinyurl.com/255ojvu to see if a bug has already been "
       "filed, and if not, add one and make it block bug 516730. Thanks!",
       aIID.ToString());
    NECKO_MAYBE_ABORT(msg);
    return NS_NOINTERFACE;
  }
}




 
NS_IMETHODIMP
HttpChannelParent::OnProgress(nsIRequest *aRequest, 
                              nsISupports *aContext, 
                              PRUint64 aProgress, 
                              PRUint64 aProgressMax)
{
  if (mIPCClosed || !SendOnProgress(aProgress, aProgressMax))
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelParent::OnStatus(nsIRequest *aRequest, 
                            nsISupports *aContext, 
                            nsresult aStatus, 
                            const PRUnichar *aStatusArg)
{
  if (mIPCClosed || !SendOnStatus(aStatus, nsString(aStatusArg)))
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}


}} 

