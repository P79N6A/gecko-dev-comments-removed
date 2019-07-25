




#include "nsUnicharStreamLoader.h"
#include "nsIInputStream.h"
#include "nsICharsetConverterManager.h"
#include "nsIServiceManager.h"

#define SNIFFING_BUFFER_SIZE 512 // specified in draft-abarth-mime-sniff-06

using mozilla::fallible_t;

NS_IMETHODIMP
nsUnicharStreamLoader::Init(nsIUnicharStreamLoaderObserver *aObserver)
{
  NS_ENSURE_ARG_POINTER(aObserver);

  mObserver = aObserver;

  if (!mRawData.SetCapacity(SNIFFING_BUFFER_SIZE, fallible_t()))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsresult
nsUnicharStreamLoader::Create(nsISupports *aOuter,
                              REFNSIID aIID,
                              void **aResult)
{
  if (aOuter) return NS_ERROR_NO_AGGREGATION;

  nsUnicharStreamLoader* it = new nsUnicharStreamLoader();
  NS_ADDREF(it);
  nsresult rv = it->QueryInterface(aIID, aResult);
  NS_RELEASE(it);
  return rv;
}

NS_IMPL_ISUPPORTS3(nsUnicharStreamLoader, nsIUnicharStreamLoader,
                   nsIRequestObserver, nsIStreamListener)


NS_IMETHODIMP
nsUnicharStreamLoader::GetChannel(nsIChannel **aChannel)
{
  NS_IF_ADDREF(*aChannel = mChannel);
  return NS_OK;
}


NS_IMETHODIMP
nsUnicharStreamLoader::GetCharset(nsACString& aCharset)
{
  aCharset = mCharset;
  return NS_OK;
}


NS_IMETHODIMP
nsUnicharStreamLoader::OnStartRequest(nsIRequest*, nsISupports*)
{
  return NS_OK;
}

NS_IMETHODIMP
nsUnicharStreamLoader::OnStopRequest(nsIRequest *aRequest,
                                     nsISupports *aContext,
                                     nsresult aStatus)
{
  if (!mObserver) {
    NS_ERROR("nsUnicharStreamLoader::OnStopRequest called before ::Init");
    return NS_ERROR_UNEXPECTED;
  }

  mContext = aContext;
  mChannel = do_QueryInterface(aRequest);

  nsresult rv = NS_OK;
  if (mRawData.Length() > 0 && NS_SUCCEEDED(aStatus)) {
    NS_ABORT_IF_FALSE(mBuffer.Length() == 0,
                      "should not have both decoded and raw data");
    rv = DetermineCharset();
  }

  if (NS_FAILED(rv)) {
    
    mObserver->OnStreamComplete(this, mContext, rv, EmptyString());
  } else {
    mObserver->OnStreamComplete(this, mContext, aStatus, mBuffer);
  }

  mObserver = nullptr;
  mDecoder = nullptr;
  mContext = nullptr;
  mChannel = nullptr;
  mCharset.Truncate();
  mBuffer.Truncate();
  return rv;
}


NS_IMETHODIMP
nsUnicharStreamLoader::OnDataAvailable(nsIRequest *aRequest,
                                       nsISupports *aContext,
                                       nsIInputStream *aInputStream,
                                       uint64_t aSourceOffset,
                                       uint32_t aCount)
{
  if (!mObserver) {
    NS_ERROR("nsUnicharStreamLoader::OnDataAvailable called before ::Init");
    return NS_ERROR_UNEXPECTED;
  }

  mContext = aContext;
  mChannel = do_QueryInterface(aRequest);

  nsresult rv = NS_OK;
  if (mDecoder) {
    
    uint32_t dummy;
    aInputStream->ReadSegments(WriteSegmentFun, this, aCount, &dummy);
  } else {
    
    
    
    
    

    uint32_t haveRead = mRawData.Length();
    uint32_t toRead = NS_MIN(SNIFFING_BUFFER_SIZE - haveRead, aCount);
    uint32_t n;
    char *here = mRawData.BeginWriting() + haveRead;

    rv = aInputStream->Read(here, toRead, &n);
    if (NS_SUCCEEDED(rv)) {
      mRawData.SetLength(haveRead + n);
      if (mRawData.Length() == SNIFFING_BUFFER_SIZE) {
        rv = DetermineCharset();
        if (NS_SUCCEEDED(rv)) {
          
          uint32_t dummy;
          aInputStream->ReadSegments(WriteSegmentFun, this, aCount - n, &dummy);
        }
      } else {
        NS_ABORT_IF_FALSE(n == aCount, "didn't read as much as was available");
      }
    }
  }

  mContext = nullptr;
  mChannel = nullptr;
  return rv;
}


static NS_DEFINE_CID(kCharsetConverterManagerCID,
                     NS_ICHARSETCONVERTERMANAGER_CID);

nsresult
nsUnicharStreamLoader::DetermineCharset()
{
  nsresult rv = mObserver->OnDetermineCharset(this, mContext,
                                              mRawData, mCharset);
  if (NS_FAILED(rv) || mCharset.IsEmpty()) {
    
    mCharset.AssignLiteral("UTF-8");
  }

  
  nsCOMPtr<nsICharsetConverterManager> ccm =
    do_GetService(kCharsetConverterManagerCID, &rv);
  if (NS_FAILED(rv)) return rv;

  rv = ccm->GetUnicodeDecoder(mCharset.get(), getter_AddRefs(mDecoder));
  if (NS_FAILED(rv)) return rv;

  
  uint32_t dummy;
  rv = WriteSegmentFun(nullptr, this,
                       mRawData.BeginReading(),
                       0, mRawData.Length(),
                       &dummy);
  mRawData.Truncate();
  return rv;
}

NS_METHOD
nsUnicharStreamLoader::WriteSegmentFun(nsIInputStream *,
                                       void *aClosure,
                                       const char *aSegment,
                                       uint32_t,
                                       uint32_t aCount,
                                       uint32_t *aWriteCount)
{
  nsUnicharStreamLoader* self = static_cast<nsUnicharStreamLoader*>(aClosure);

  uint32_t haveRead = self->mBuffer.Length();
  uint32_t consumed = 0;
  nsresult rv;
  do {
    int32_t srcLen = aCount - consumed;
    int32_t dstLen;
    self->mDecoder->GetMaxLength(aSegment + consumed, srcLen, &dstLen);

    uint32_t capacity = haveRead + dstLen;
    if (!self->mBuffer.SetCapacity(capacity, fallible_t())) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    rv = self->mDecoder->Convert(aSegment + consumed,
                                 &srcLen,
                                 self->mBuffer.BeginWriting() + haveRead,
                                 &dstLen);
    haveRead += dstLen;
    
    
    
    consumed += srcLen;
    if (NS_FAILED(rv)) {
      if (haveRead >= capacity) {
        
        if (!self->mBuffer.SetCapacity(haveRead + 1, fallible_t())) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }
      self->mBuffer.BeginWriting()[haveRead++] = 0xFFFD;
      ++consumed;
      
      
      consumed = NS_MAX<uint32_t>(consumed, 0);
      self->mDecoder->Reset();
    }
  } while (consumed < aCount);

  self->mBuffer.SetLength(haveRead);
  *aWriteCount = aCount;
  return NS_OK;
}
