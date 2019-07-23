





































#include "nsUnicharStreamLoader.h"
#include "nsNetUtil.h"
#include "nsProxiedService.h"
#include "nsIChannel.h"
#include "nsIUnicharInputStream.h"
#include "nsIConverterInputStream.h"
#include "nsIPipe.h"

#ifdef DEBUG 
#include "nsReadableUtils.h"
#endif 

NS_IMETHODIMP
nsUnicharStreamLoader::Init(nsIUnicharStreamLoaderObserver *aObserver,
                            PRUint32 aSegmentSize)
{
  NS_ENSURE_ARG_POINTER(aObserver);

  if (aSegmentSize <= 0) {
    aSegmentSize = nsIUnicharStreamLoader::DEFAULT_SEGMENT_SIZE;
  }
  
  mObserver = aObserver;
  mCharset.Truncate();
  mChannel = nsnull; 
  mSegmentSize = aSegmentSize;
  return NS_OK;
}

NS_METHOD
nsUnicharStreamLoader::Create(nsISupports *aOuter,
                              REFNSIID aIID,
                              void **aResult)
{
  if (aOuter) return NS_ERROR_NO_AGGREGATION;

  nsUnicharStreamLoader* it = new nsUnicharStreamLoader();
  if (it == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
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
nsUnicharStreamLoader::OnStartRequest(nsIRequest* request,
                                      nsISupports *ctxt)
{
  mContext = ctxt;
  return NS_OK;
}

NS_IMETHODIMP 
nsUnicharStreamLoader::OnStopRequest(nsIRequest *request,
                                     nsISupports *ctxt,
                                     nsresult aStatus)
{
  
  
  
  if (!mObserver) {
    NS_ERROR("No way we should not have an mObserver here!");
    return NS_ERROR_UNEXPECTED;
  }

  
  mChannel = do_QueryInterface(request);

  if (mInputStream) {
    nsresult rv;
    
    

    
    PRUint32 readCount = 0;
    rv = mInputStream->ReadSegments(WriteSegmentFun,
                                    this,
                                    mSegmentSize, 
                                    &readCount);
    if (NS_FAILED(rv)) {
      rv = mObserver->OnStreamComplete(this, mContext, rv, nsnull);
      goto cleanup;
    }

    nsCOMPtr<nsIConverterInputStream> uin =
      do_CreateInstance("@mozilla.org/intl/converter-input-stream;1",
                        &rv);
    if (NS_FAILED(rv)) {
      rv = mObserver->OnStreamComplete(this, mContext, rv, nsnull);
      goto cleanup;
    }

    rv = uin->Init(mInputStream,
                   mCharset.get(),
                   mSegmentSize,
                   nsIConverterInputStream::DEFAULT_REPLACEMENT_CHARACTER);
    
    if (NS_FAILED(rv)) {
      rv = mObserver->OnStreamComplete(this, mContext, rv, nsnull);
      goto cleanup;
    }
    
    mObserver->OnStreamComplete(this, mContext, aStatus, uin);

  } else {
    
    
    mObserver->OnStreamComplete(this, mContext, aStatus, nsnull);
  }
  
  
 cleanup:
  mObserver = nsnull;
  mChannel = nsnull;
  mContext = nsnull;
  mInputStream = nsnull;
  mOutputStream = nsnull;
  return NS_OK;
}


NS_METHOD
nsUnicharStreamLoader::WriteSegmentFun(nsIInputStream *aInputStream,
                                       void *aClosure,
                                       const char *aSegment,
                                       PRUint32 aToOffset,
                                       PRUint32 aCount,
                                       PRUint32 *aWriteCount)
{
  nsUnicharStreamLoader *self = (nsUnicharStreamLoader *) aClosure;
  if (self->mCharset.IsEmpty()) {
    
    NS_ASSERTION(self->mObserver, "This should never be possible");

    nsresult rv = self->mObserver->OnDetermineCharset(self,
                                                      self->mContext,
                                                      aSegment,
                                                      aCount,
                                                      self->mCharset);
    
    if (NS_FAILED(rv) || self->mCharset.IsEmpty()) {
      
      self->mCharset.AssignLiteral("ISO-8859-1");
    }

    NS_ASSERTION(IsASCII(self->mCharset),
                 "Why is the charset name non-ascii?  Whose bright idea was that?");
  }
  
  *aWriteCount = 0;
  return NS_BASE_STREAM_WOULD_BLOCK;
}


NS_IMETHODIMP
nsUnicharStreamLoader::OnDataAvailable(nsIRequest *aRequest,
                                       nsISupports *aContext,
                                       nsIInputStream *aInputStream, 
                                       PRUint32 aSourceOffset,
                                       PRUint32 aCount)
{
  nsresult rv = NS_OK;
  if (!mInputStream) {
    
    NS_ASSERTION(!mOutputStream, "Why are we sorta-initialized?");
    rv = NS_NewPipe(getter_AddRefs(mInputStream),
                    getter_AddRefs(mOutputStream),
                    mSegmentSize,
                    PRUint32(-1),  
                    PR_TRUE,  
                    PR_TRUE); 
    if (NS_FAILED(rv))
      return rv;
  }

  PRUint32 writeCount = 0;
  do {
    rv = mOutputStream->WriteFrom(aInputStream, aCount, &writeCount);
    if (NS_FAILED(rv)) return rv;
    aCount -= writeCount;
  } while (aCount > 0);

  return NS_OK;
}
