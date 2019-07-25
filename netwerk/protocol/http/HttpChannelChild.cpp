








































#include "nsHttp.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/net/HttpChannelChild.h"

#include "nsStringStream.h"
#include "nsHttpHandler.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace net {


HttpChannelChild::HttpChannelChild()
  : mState(HCC_NEW)
{
  LOG(("Creating HttpChannelChild @%x\n", this));
}

HttpChannelChild::~HttpChannelChild()
{
  LOG(("Destroying HttpChannelChild @%x\n", this));
}






NS_IMPL_ADDREF(HttpChannelChild)
NS_IMPL_RELEASE_WITH_DESTROY(HttpChannelChild, RefcountHitZero())

void
HttpChannelChild::RefcountHitZero()
{
  if (mWasOpened) {
    
    PHttpChannelChild::Send__delete__(this);
  } else {
    delete this;    
  }
}

NS_INTERFACE_MAP_BEGIN(HttpChannelChild)
  NS_INTERFACE_MAP_ENTRY(nsIRequest)
  NS_INTERFACE_MAP_ENTRY(nsIChannel)
  NS_INTERFACE_MAP_ENTRY(nsIHttpChannel)
  NS_INTERFACE_MAP_ENTRY(nsIHttpChannelInternal)
  NS_INTERFACE_MAP_ENTRY(nsICachingChannel)
  NS_INTERFACE_MAP_ENTRY(nsIEncodedChannel)
  NS_INTERFACE_MAP_ENTRY(nsIResumableChannel)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY(nsIProxiedChannel)
  NS_INTERFACE_MAP_ENTRY(nsITraceableChannel)
  NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheContainer)
  NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheChannel)
NS_INTERFACE_MAP_END_INHERITING(HttpBaseChannel)





bool 
HttpChannelChild::RecvOnStartRequest(const nsHttpResponseHead& responseHead)
{
  LOG(("HttpChannelChild::RecvOnStartRequest [this=%x]\n", this));

  mState = HCC_ONSTART;

  mResponseHead = new nsHttpResponseHead(responseHead);

  nsresult rv = mListener->OnStartRequest(this, mListenerContext);
  if (!NS_SUCCEEDED(rv)) {
    
    
    
    
    
    return false;  
  }
  return true;
}

bool 
HttpChannelChild::RecvOnDataAvailable(const nsCString& data,
                                      const PRUint32& offset,
                                      const PRUint32& count)
{
  LOG(("HttpChannelChild::RecvOnDataAvailable [this=%x]\n", this));

  mState = HCC_ONDATA;

  
  
  
  
  
  nsCOMPtr<nsIInputStream> stringStream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stringStream),
                                      data.get(),
                                      count,
                                      NS_ASSIGNMENT_DEPEND);
  if (!NS_SUCCEEDED(rv)) {
    
    return false;  
  }
  rv = mListener->OnDataAvailable(this, mListenerContext,
                                  stringStream, offset, count);
  stringStream->Close();
  if (!NS_SUCCEEDED(rv)) {
    
    return false; 
  }
  return true;
}

bool 
HttpChannelChild::RecvOnStopRequest(const nsresult& statusCode)
{
  LOG(("HttpChannelChild::RecvOnStopRequest [this=%x status=%u]\n", 
           this, statusCode));

  mState = HCC_ONSTOP;

  mIsPending = PR_FALSE;
  mStatus = statusCode;
  nsresult rv = mListener->OnStopRequest(this, mListenerContext, statusCode);
  mListener = 0;
  mListenerContext = 0;

  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nsnull, statusCode);

  
  this->Release();
  
  if (!NS_SUCCEEDED(rv)) {
    
    return false;  
  }
  return true;
}

bool
HttpChannelChild::RecvOnProgress(const PRUint64& progress,
                                 const PRUint64& progressMax)
{
  LOG(("HttpChannelChild::RecvOnProgress [this=%p progress=%llu/%llu]\n",
       this, progress, progressMax));

  
  if (!mProgressSink)
    GetCallback(mProgressSink);

  
  if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending && 
      !(mLoadFlags & LOAD_BACKGROUND)) 
  {
     if (progress > 0) {
      NS_ASSERTION(progress <= progressMax, "unexpected progress values");
      mProgressSink->OnProgress(this, nsnull, progress, progressMax);
    }
  }

  return true;
}

bool
HttpChannelChild::RecvOnStatus(const nsresult& status,
                               const nsString& statusArg)
{
  LOG(("HttpChannelChild::RecvOnStatus [this=%p status=%x]\n", this, status));

  
  if (!mProgressSink)
    GetCallback(mProgressSink);

  
  if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending && 
      !(mLoadFlags & LOAD_BACKGROUND)) 
  {
    mProgressSink->OnStatus(this, nsnull, status, statusArg.get());
  }

  return true;
}





NS_IMETHODIMP
HttpChannelChild::Cancel(nsresult status)
{
  
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::Suspend()
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::Resume()
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetSecurityInfo(nsISupports **aSecurityInfo)
{
  
  NS_ENSURE_ARG_POINTER(aSecurityInfo);
  *aSecurityInfo = 0;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::AsyncOpen(nsIStreamListener *listener, nsISupports *aContext)
{
  LOG(("HttpChannelChild::AsyncOpen [this=%x uri=%s]\n", this, mSpec.get()));

  NS_ENSURE_TRUE(gNeckoChild != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_ARG_POINTER(listener);
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  
  
  nsresult rv;
  rv = NS_CheckPortSafety(mURI);
  if (NS_FAILED(rv))
    return rv;

  
  nsCAutoString uploadStreamData;
  PRInt32 uploadStreamInfo;

  if (mUploadStream) {
    
    
    
    
    PRUint32 bytes;
    mUploadStream->Available(&bytes);
    if (bytes > 0) {
      rv = NS_ReadInputStreamToString(mUploadStream, uploadStreamData, bytes);
      if (!NS_SUCCEEDED(rv))
        return rv;
    }

    uploadStreamInfo = mUploadStreamHasHeaders ? 
      eUploadStream_hasHeaders : eUploadStream_hasNoHeaders;
  } else {
    uploadStreamInfo = eUploadStream_null;
  }

  

  
  
  
  

  
  gHttpHandler->OnModifyRequest(this);

  mIsPending = PR_TRUE;
  mWasOpened = PR_TRUE;
  mListener = listener;
  mListenerContext = aContext;

  
  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nsnull);

  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  

  
  gNeckoChild->SendPHttpChannelConstructor(this);

  SendAsyncOpen(IPC::URI(mURI), IPC::URI(mOriginalURI), IPC::URI(mDocumentURI),
                IPC::URI(mReferrer), mLoadFlags, mRequestHeaders, 
                mRequestHead.Method(), uploadStreamData, 
                uploadStreamInfo, mPriority, mRedirectionLimit, 
                mAllowPipelining, mForceAllowThirdPartyCookie);

  
  
  this->AddRef();

  mState = HCC_OPENED;
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::SetRequestHeader(const nsACString& aHeader, 
                                   const nsACString& aValue, 
                                   PRBool aMerge)
{
  nsresult rv = HttpBaseChannel::SetRequestHeader(aHeader, aValue, aMerge);
  if (NS_FAILED(rv))
    return rv;

  RequestHeaderTuple* tuple = mRequestHeaders.AppendElement();
  if (!tuple)
    return NS_ERROR_OUT_OF_MEMORY;

  tuple->mHeader = aHeader;
  tuple->mValue = aValue;
  tuple->mMerge = aMerge;
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::SetupFallbackChannel(const char *aFallbackKey)
{
  DROP_DEAD();
}






NS_IMETHODIMP
HttpChannelChild::GetCacheToken(nsISupports **aCacheToken)
{
  
  return NS_ERROR_NOT_AVAILABLE;
}
NS_IMETHODIMP
HttpChannelChild::SetCacheToken(nsISupports *aCacheToken)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetOfflineCacheToken(nsISupports **aOfflineCacheToken)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetOfflineCacheToken(nsISupports *aOfflineCacheToken)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetCacheKey(nsISupports **aCacheKey)
{
  
  NS_ENSURE_ARG_POINTER(aCacheKey);
  *aCacheKey = 0;
  return NS_OK;
}
NS_IMETHODIMP
HttpChannelChild::SetCacheKey(nsISupports *aCacheKey)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetCacheAsFile(PRBool *aCacheAsFile)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetCacheAsFile(PRBool aCacheAsFile)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetCacheForOfflineUse(PRBool *aCacheForOfflineUse)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetCacheForOfflineUse(PRBool aCacheForOfflineUse)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetOfflineCacheClientID(nsACString& id)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetOfflineCacheClientID(const nsACString& id)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetCacheFile(nsIFile **aCacheFile)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::IsFromCache(PRBool *value)
{
  if (!mIsPending)
    return NS_ERROR_NOT_AVAILABLE;

  
  *value = false;
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::GetContentEncodings(nsIUTF8StringEnumerator **result)
{
  DROP_DEAD();
}


NS_IMETHODIMP
HttpChannelChild::GetApplyConversion(PRBool *aApplyConversion)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::SetApplyConversion(PRBool aApplyConversion)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::ResumeAt(PRUint64 startPos, const nsACString& entityID)
{
  DROP_DEAD();
}

NS_IMETHODIMP
HttpChannelChild::GetEntityID(nsACString& aEntityID)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::SetPriority(PRInt32 aPriority)
{
  PRInt16 newValue = CLAMP(aPriority, PR_INT16_MIN, PR_INT16_MAX);
  if (mPriority == newValue)
    return NS_OK;
  mPriority = newValue;
  if (mWasOpened) 
    SendSetPriority(mPriority);
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::GetProxyInfo(nsIProxyInfo **aProxyInfo)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::SetNewListener(nsIStreamListener *listener, 
                                 nsIStreamListener **oldListener)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetApplicationCache(nsIApplicationCache **aApplicationCache)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetApplicationCache(nsIApplicationCache *aApplicationCache)
{
  DROP_DEAD();
}





NS_IMETHODIMP
HttpChannelChild::GetLoadedFromApplicationCache(PRBool *retval)
{
  
  *retval = 0;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetInheritApplicationCache(PRBool *aInheritApplicationCache)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetInheritApplicationCache(PRBool aInheritApplicationCache)
{
  
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetChooseApplicationCache(PRBool *aChooseApplicationCache)
{
  DROP_DEAD();
}
NS_IMETHODIMP
HttpChannelChild::SetChooseApplicationCache(PRBool aChooseApplicationCache)
{
  
  return NS_OK;
}



}} 

