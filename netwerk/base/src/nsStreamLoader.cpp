




#include "nsStreamLoader.h"
#include "nsIInputStream.h"
#include "nsIChannel.h"
#include "nsError.h"
#include "GeckoProfiler.h"

#include <limits>

nsStreamLoader::nsStreamLoader()
  : mData()
{
}

nsStreamLoader::~nsStreamLoader()
{
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

NS_IMPL_ISUPPORTS(nsStreamLoader, nsIStreamLoader,
                  nsIRequestObserver, nsIStreamListener)

NS_IMETHODIMP 
nsStreamLoader::GetNumBytesRead(uint32_t* aNumBytes)
{
  *aNumBytes = mData.length();
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
      if (uint64_t(contentLength) > std::numeric_limits<size_t>::max()) {
        
        return NS_ERROR_OUT_OF_MEMORY;
      }
      
      if (!mData.initCapacity(contentLength)) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }
  mContext = ctxt;
  return NS_OK;
}

NS_IMETHODIMP
nsStreamLoader::OnStopRequest(nsIRequest* request, nsISupports *ctxt,
                              nsresult aStatus)
{
  PROFILER_LABEL("nsStreamLoader", "OnStopRequest",
    js::ProfileEntry::Category::NETWORK);

  if (mObserver) {
    
    mRequest = request;
    size_t length = mData.length();
    uint8_t* elems = mData.extractRawBuffer();
    nsresult rv = mObserver->OnStreamComplete(this, mContext, aStatus,
                                              length, elems);
    if (rv != NS_SUCCESS_ADOPTED_DATA) {
      
      
      mData.replaceRawBuffer(elems, length);
    }
    
    ReleaseData();
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

  if (!self->mData.append(fromSegment, count)) {
    self->mData.clearAndFree();
    return NS_ERROR_OUT_OF_MEMORY;
  }

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

void
nsStreamLoader::ReleaseData()
{
  mData.clearAndFree();
}
