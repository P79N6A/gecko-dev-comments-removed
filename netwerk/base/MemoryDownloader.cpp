




#include "MemoryDownloader.h"

#include "mozilla/Assertions.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS(MemoryDownloader,
		  nsIStreamListener,
		  nsIRequestObserver)

MemoryDownloader::MemoryDownloader(IObserver* aObserver)
: mObserver(aObserver)
{
}

MemoryDownloader::~MemoryDownloader()
{
}

NS_IMETHODIMP
MemoryDownloader::OnStartRequest(nsIRequest* aRequest, nsISupports* aCtxt)
{
  MOZ_ASSERT(!mData);
  mData.reset(new FallibleTArray<uint8_t>());
  mStatus = NS_OK;
  return NS_OK;
}

NS_IMETHODIMP
MemoryDownloader::OnStopRequest(nsIRequest* aRequest,
                                nsISupports* aCtxt,
                                nsresult aStatus)
{
  MOZ_ASSERT_IF(NS_FAILED(mStatus), NS_FAILED(aStatus));
  MOZ_ASSERT(!mData == NS_FAILED(mStatus));
  Data data;
  data.swap(mData);
  nsRefPtr<IObserver> observer;
  observer.swap(mObserver);
  observer->OnDownloadComplete(this, aRequest, aCtxt, aStatus,
                               mozilla::Move(data));
  return NS_OK;
}

NS_METHOD
MemoryDownloader::ConsumeData(nsIInputStream* aIn,
                              void* aClosure,
                              const char* aFromRawSegment,
                              uint32_t aToOffset,
                              uint32_t aCount,
                              uint32_t* aWriteCount)
{
  MemoryDownloader* self = static_cast<MemoryDownloader*>(aClosure);
  if (!self->mData->AppendElements(aFromRawSegment, aCount, fallible)) {
    
    
    self->mStatus = NS_ERROR_OUT_OF_MEMORY;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  *aWriteCount = aCount;
  return NS_OK;
}

NS_IMETHODIMP
MemoryDownloader::OnDataAvailable(nsIRequest* aRequest,
                                  nsISupports* aCtxt,
                                  nsIInputStream* aInStr,
                                  uint64_t aSourceOffset,
                                  uint32_t aCount)
{
  uint32_t n;
  MOZ_ASSERT(mData);
  nsresult rv = aInStr->ReadSegments(ConsumeData, this, aCount, &n);
  if (NS_SUCCEEDED(mStatus) && NS_FAILED(rv)) {
    mStatus = rv;
  }
  if (NS_WARN_IF(NS_FAILED(mStatus))) {
    mData.reset(nullptr);
    return mStatus;
  }
  return NS_OK;
}

} 
} 
