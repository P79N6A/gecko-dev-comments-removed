




#include "nsStreamLoader.h"
#include "nsIInputStream.h"
#include "nsIChannel.h"
#include "nsError.h"
#include "sampler.h"

nsStreamLoader::nsStreamLoader()
  : mData(nullptr),
    mAllocated(0),
    mLength(0)
{
}

nsStreamLoader::~nsStreamLoader()
{
  if (mData) {
    NS_Free(mData);
  }
}

NS_IMETHODIMP
nsStreamLoader::Init(nsIStreamLoaderObserver* observer)
{
  NS_ENSURE_ARG_POINTER(observer);
  mObserver = observer;
  return NS_OK;
}

nsresult
nsStreamLoader::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (aOuter) return NS_ERROR_NO_AGGREGATION;

  nsStreamLoader* it = new nsStreamLoader();
  if (it == nullptr)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(it);
  nsresult rv = it->QueryInterface(aIID, aResult);
  NS_RELEASE(it);
  return rv;
}

NS_IMPL_ISUPPORTS3(nsStreamLoader, nsIStreamLoader,
                   nsIRequestObserver, nsIStreamListener)

NS_IMETHODIMP 
nsStreamLoader::GetNumBytesRead(uint32_t* aNumBytes)
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
    int64_t contentLength = -1;
    chan->GetContentLength(&contentLength);
    if (contentLength >= 0) {
      if (contentLength > UINT32_MAX) {
        
        
        return NS_ERROR_OUT_OF_MEMORY;
      }
      uint32_t contentLength32 = uint32_t(contentLength);
      
      mData = static_cast<uint8_t*>(NS_Alloc(contentLength32));
      if (!mData) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      mAllocated = contentLength32;
    }
  }
  mContext = ctxt;
  return NS_OK;
}

NS_IMETHODIMP 
nsStreamLoader::OnStopRequest(nsIRequest* request, nsISupports *ctxt,
                              nsresult aStatus)
{
  SAMPLE_LABEL("network", "nsStreamLoader::OnStopRequest");
  if (mObserver) {
    
    mRequest = request;
    nsresult rv = mObserver->OnStreamComplete(this, mContext, aStatus,
                                              mLength, mData);
    if (rv == NS_SUCCESS_ADOPTED_DATA) {
      
      
      mData = nullptr;
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
                                uint32_t toOffset,
                                uint32_t count,
                                uint32_t *writeCount)
{
  nsStreamLoader *self = (nsStreamLoader *) closure;

  if (count > UINT32_MAX - self->mLength) {
    return NS_ERROR_ILLEGAL_VALUE; 
  }

  if (self->mLength + count > self->mAllocated) {
    self->mData = static_cast<uint8_t*>(NS_Realloc(self->mData,
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
                                uint64_t sourceOffset, uint32_t count)
{
  uint32_t countRead;
  return inStr->ReadSegments(WriteSegmentFun, this, count, &countRead);
}
