




































#include "nsStreamLoader.h"
#include "nsIInputStream.h"
#include "nsIChannel.h"
#include "nsNetError.h"

#include <stdlib.h>

nsStreamLoader::nsStreamLoader()
  : mData(nsnull),
    mAllocated(0),
    mLength(0)
{
}

nsStreamLoader::~nsStreamLoader()
{
  if (mData) {
    ::free(mData);
  }
}

NS_IMETHODIMP
nsStreamLoader::Init(nsIStreamLoaderObserver* observer)
{
  NS_ENSURE_ARG_POINTER(observer);
  mObserver = observer;
  return NS_OK;
}

NS_METHOD
nsStreamLoader::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter) return NS_ERROR_NO_AGGREGATION;

  nsStreamLoader* it = new nsStreamLoader();
  if (it == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(it);
  nsresult rv = it->QueryInterface(aIID, aResult);
  NS_RELEASE(it);
  return rv;
}

NS_IMPL_ISUPPORTS3(nsStreamLoader, nsIStreamLoader,
                   nsIRequestObserver, nsIStreamListener)

NS_IMETHODIMP 
nsStreamLoader::GetNumBytesRead(PRUint32* aNumBytes)
{
  *aNumBytes = mLength;
  return NS_OK;
}


NS_IMETHODIMP 
nsStreamLoader::GetRequest(nsIRequest **aRequest)
{
  NS_IF_ADDREF(*aRequest = mRequest);
  return NS_OK;
}

NS_IMETHODIMP 
nsStreamLoader::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
  nsCOMPtr<nsIChannel> chan( do_QueryInterface(request) );
  if (chan) {
    PRInt32 contentLength = -1;
    chan->GetContentLength(&contentLength);
    if (contentLength >= 0) {
      
      mData = static_cast<PRUint8*>(::malloc(contentLength));
      if (!mData) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mAllocated = contentLength;
    }
  }
  mContext = ctxt;
  return NS_OK;
}

NS_IMETHODIMP 
nsStreamLoader::OnStopRequest(nsIRequest* request, nsISupports *ctxt,
                              nsresult aStatus)
{
  if (mObserver) {
    
    mRequest = request;
    nsresult rv = mObserver->OnStreamComplete(this, mContext, aStatus,
                                              mLength, mData);
    if (rv == NS_SUCCESS_ADOPTED_DATA) {
      
      
      mData = nsnull;
      mLength = 0;
      mAllocated = 0;
    }
    
    mRequest = 0;
    mObserver = 0;
    mContext = 0;
  }
  return NS_OK;
}

NS_METHOD
nsStreamLoader::WriteSegmentFun(nsIInputStream *inStr,
                                void *closure,
                                const char *fromSegment,
                                PRUint32 toOffset,
                                PRUint32 count,
                                PRUint32 *writeCount)
{
  nsStreamLoader *self = (nsStreamLoader *) closure;

  if (count > 0xffffffffU - self->mLength) {
    return NS_ERROR_ILLEGAL_VALUE; 
  }

  if (self->mLength + count > self->mAllocated) {
    self->mData = static_cast<PRUint8*>(::realloc(self->mData,
                                                  self->mLength + count));
    if (!self->mData) {
      self->mLength = 0;
      self->mAllocated = 0;
      return NS_ERROR_OUT_OF_MEMORY;
    }
    self->mAllocated = self->mLength + count;
  }

  ::memcpy(self->mData + self->mLength, fromSegment, count);
  self->mLength += count;

  *writeCount = count;

  return NS_OK;
}

NS_IMETHODIMP 
nsStreamLoader::OnDataAvailable(nsIRequest* request, nsISupports *ctxt, 
                                nsIInputStream *inStr, 
                                PRUint32 sourceOffset, PRUint32 count)
{
  PRUint32 countRead;
  return inStr->ReadSegments(WriteSegmentFun, this, count, &countRead);
}
