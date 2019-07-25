







































#include "mozilla/net/HttpChannelParent.h"
#include "nsHttpChannel.h"
#include "nsHttpHandler.h"
#include "nsNetUtil.h"
#include "nsISupportsPriority.h"

namespace mozilla {
namespace net {


HttpChannelParent::HttpChannelParent()
{
  
  nsIHttpProtocolHandler* handler;
  CallGetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http", &handler);
  NS_ASSERTION(handler, "no http handler");
}

HttpChannelParent::~HttpChannelParent()
{
  NS_RELEASE(gHttpHandler);
}





NS_IMPL_ISUPPORTS3(HttpChannelParent, 
                   nsIRequestObserver, 
                   nsIStreamListener,
                   nsIInterfaceRequestor);





bool 
HttpChannelParent::RecvAsyncOpen(const nsCString&           uriSpec, 
                                 const nsCString&           charset,
                                 const nsCString&           originalUriSpec, 
                                 const nsCString&           originalCharset,
                                 const nsCString&           docUriSpec, 
                                 const nsCString&           docCharset,
                                 const nsCString&           referrerSpec,
                                 const nsCString&           referrerCharset,
                                 const PRUint32&            loadFlags,
                                 const RequestHeaderTuples& requestHeaders,
                                 const nsHttpAtom&          requestMethod,
                                 const PRUint16&            priority,
                                 const PRUint8&             redirectionLimit,
                                 const PRBool&              allowPipelining,
                                 const PRBool&              forceAllowThirdPartyCookie)
{
  LOG(("HttpChannelParent RecvAsyncOpen [this=%x uri=%s (%s)]\n", 
       this, uriSpec.get(), charset.get()));

  nsresult rv;

  nsCOMPtr<nsIIOService> ios(do_GetIOService(&rv));
  if (NS_FAILED(rv))
    return false;       

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), uriSpec, charset.get(), nsnull, ios);
  if (NS_FAILED(rv))
    return false;       

  nsCOMPtr<nsIChannel> chan;
  rv = NS_NewChannel(getter_AddRefs(chan), uri, ios, nsnull, nsnull, loadFlags);
  if (NS_FAILED(rv))
    return false;       

  nsHttpChannel *httpChan = static_cast<nsHttpChannel *>(chan.get());

  if (!originalUriSpec.IsEmpty()) {
    nsCOMPtr<nsIURI> originalUri;
    rv = NS_NewURI(getter_AddRefs(originalUri), originalUriSpec, 
                   originalCharset.get(), nsnull, ios);
    if (!NS_FAILED(rv))
      httpChan->SetOriginalURI(originalUri);
  }
  if (!docUriSpec.IsEmpty()) {
    nsCOMPtr<nsIURI> docUri;
    rv = NS_NewURI(getter_AddRefs(docUri), docUriSpec, 
                   docCharset.get(), nsnull, ios);
    if (!NS_FAILED(rv)) {
      httpChan->SetDocumentURI(docUri);
    }
  }
  if (!referrerSpec.IsEmpty()) {
    nsCOMPtr<nsIURI> referrerUri;
    rv = NS_NewURI(getter_AddRefs(referrerUri), referrerSpec,
                   referrerCharset.get(), nsnull, ios);
    if (!NS_FAILED(rv)) {
      httpChan->SetReferrerInternal(referrerUri);
    }
  }
  if (loadFlags != nsIRequest::LOAD_NORMAL)
    httpChan->SetLoadFlags(loadFlags);

  for (PRUint32 i = 0; i < requestHeaders.Length(); i++)
    httpChan->SetRequestHeader(requestHeaders[i].mHeader,
                               requestHeaders[i].mValue,
                               requestHeaders[i].mMerge);

  
  
  

  httpChan->SetRequestMethod(nsDependentCString(requestMethod.get()));
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
  
  
  return true;
}





NS_IMETHODIMP
HttpChannelParent::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  LOG(("HttpChannelParent::OnStartRequest [this=%x]\n", this));

  nsHttpChannel *chan = static_cast<nsHttpChannel *>(aRequest);
  nsHttpResponseHead *responseHead = chan->GetResponseHead();
  NS_ABORT_IF_FALSE(responseHead, "Missing HTTP responseHead!");

  if (!SendOnStartRequest(*responseHead)) {
    
    
    
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

  if (!SendOnStopRequest(aStatusCode)) {
    
    
    return NS_ERROR_UNEXPECTED; 
  }
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

  if (!SendOnDataAvailable(data, aOffset, bytesRead)) {
    
    
    return NS_ERROR_UNEXPECTED; 
  }
  return NS_OK;
}





NS_IMETHODIMP 
HttpChannelParent::GetInterface(const nsIID& uuid, void **result)
{
  DROP_DEAD();
}


}} 

