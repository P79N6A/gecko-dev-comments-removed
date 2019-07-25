







































#include "nsHttp.h"
#include "mozilla/net/HttpChannelParent.h"
#include "nsHttpChannel.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace net {


HttpChannelParent::HttpChannelParent()
{
}

HttpChannelParent::~HttpChannelParent()
{
}





NS_IMPL_ISUPPORTS3(HttpChannelParent, 
                   nsIRequestObserver, 
                   nsIStreamListener,
                   nsIInterfaceRequestor);





bool 
HttpChannelParent::RecvAsyncOpen(const nsCString& uriSpec, 
                                 const nsCString& charset,
                                 const nsCString& originalUriSpec, 
                                 const nsCString& originalCharset,
                                 const nsCString& docUriSpec, 
                                 const nsCString& docCharset,
                                 const PRUint32&  loadFlags)
{
  nsresult rv;

  nsCOMPtr<nsIIOService> ios(do_GetIOService(&rv));
  if (NS_FAILED(rv))
    return false;       

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), uriSpec, charset.get(), nsnull, ios);
  if (NS_FAILED(rv))
    return false;       

  
  
  LOG(("HttpChannelParent RecvAsyncOpen [this=%x uri=%s (%s)]\n", 
       this, uriSpec.get(), charset.get()));

  nsCOMPtr<nsIChannel> chan;
  rv = NS_NewChannel(getter_AddRefs(chan), uri, ios, nsnull, nsnull, loadFlags);
  if (NS_FAILED(rv))
    return false;       

  if (!originalUriSpec.IsEmpty()) {
    nsCOMPtr<nsIURI> originalUri;
    rv = NS_NewURI(getter_AddRefs(originalUri), originalUriSpec, 
                   originalCharset.get(), nsnull, ios);
    if (!NS_FAILED(rv))
      chan->SetOriginalURI(originalUri);
  }
  if (!docUriSpec.IsEmpty()) {
    nsCOMPtr<nsIURI> docUri;
    rv = NS_NewURI(getter_AddRefs(docUri), docUriSpec, 
                   docCharset.get(), nsnull, ios);
    if (!NS_FAILED(rv)) {
      nsCOMPtr<nsIHttpChannelInternal> iChan(do_QueryInterface(chan));
      if (iChan) 
        iChan->SetDocumentURI(docUri);
    }
  }
  if (loadFlags != nsIRequest::LOAD_NORMAL)
    chan->SetLoadFlags(loadFlags);
 
  
  


  rv = chan->AsyncOpen(this, nsnull);
  if (NS_FAILED(rv))
    return false;       

  return true;
}






NS_IMETHODIMP
HttpChannelParent::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  LOG(("HttpChannelParent::OnStartRequest [this=%x]\n", this));

  nsCOMPtr<nsIHttpChannel> chan(do_QueryInterface(aRequest));
  NS_ENSURE_TRUE(chan, NS_ERROR_FAILURE);

  




  PRInt32 contentLength_HACK;
  chan->GetContentLength(&contentLength_HACK);
  nsCAutoString contentType_HACK;
  chan->GetContentType(contentType_HACK);
  PRUint32 status_HACK;
  chan->GetResponseStatus(&status_HACK);
  nsCAutoString statusText_HACK;
  chan->GetResponseStatusText(statusText_HACK);

  if (!SendOnStartRequest(contentLength_HACK, contentType_HACK,
                          status_HACK, statusText_HACK)) 
  {
    
    
    
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

