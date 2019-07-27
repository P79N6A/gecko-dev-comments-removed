




#include "nsInputStreamChannel.h"




nsresult
nsInputStreamChannel::OpenContentStream(bool async, nsIInputStream **result,
                                        nsIChannel** channel)
{
  NS_ENSURE_TRUE(mContentStream, NS_ERROR_NOT_INITIALIZED);

  
  

  if (mContentLength < 0) {
    uint64_t avail;
    nsresult rv = mContentStream->Available(&avail);
    if (rv == NS_BASE_STREAM_CLOSED) {
      
      avail = 0;
    } else if (NS_FAILED(rv)) {
      return rv;
    }
    mContentLength = avail;
  }

  EnableSynthesizedProgressEvents(true);
  
  NS_ADDREF(*result = mContentStream);
  return NS_OK;
}




NS_IMPL_ISUPPORTS_INHERITED(nsInputStreamChannel,
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

NS_IMETHODIMP
nsInputStreamChannel::GetSrcdocData(nsAString& aSrcdocData)
{
  aSrcdocData = mSrcdocData;
  return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::SetSrcdocData(const nsAString& aSrcdocData)
{
  mSrcdocData = aSrcdocData;
  mIsSrcdocChannel = true;
  return NS_OK;
}

NS_IMETHODIMP
nsInputStreamChannel::GetIsSrcdocChannel(bool *aIsSrcdocChannel)
{
  *aIsSrcdocChannel = mIsSrcdocChannel;
  return NS_OK;
}
