




































#include "nsInputStreamChannel.h"




nsresult
nsInputStreamChannel::OpenContentStream(PRBool async, nsIInputStream **result)
{
  NS_ENSURE_TRUE(mContentStream, NS_ERROR_NOT_INITIALIZED);

  
  

  PRInt64 len = ContentLength64();
  if (len < 0) {
    PRUint32 avail;
    nsresult rv = mContentStream->Available(&avail);
    if (rv == NS_BASE_STREAM_CLOSED) {
      
      avail = 0;
    } else if (NS_FAILED(rv)) {
      return rv;
    }
    SetContentLength64(avail);
  }

  EnableSynthesizedProgressEvents(PR_TRUE);
  
  NS_ADDREF(*result = mContentStream);
  return NS_OK;
}




NS_IMPL_ISUPPORTS_INHERITED1(nsInputStreamChannel,
                             nsBaseChannel,
                             nsIInputStreamChannel)




NS_IMETHODIMP
nsInputStreamChannel::SetURI(nsIURI *uri)
{
  NS_ENSURE_TRUE(!URI(), NS_ERROR_ALREADY_INITIALIZED);
  nsBaseChannel::SetURI(uri);
  return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::GetContentStream(nsIInputStream **stream)
{
  NS_IF_ADDREF(*stream = mContentStream);
  return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::SetContentStream(nsIInputStream *stream)
{
  NS_ENSURE_TRUE(!mContentStream, NS_ERROR_ALREADY_INITIALIZED);
  mContentStream = stream;
  return NS_OK;
}
