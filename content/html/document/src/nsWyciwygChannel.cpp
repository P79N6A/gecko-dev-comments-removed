






































#include "nsWyciwygChannel.h"
#include "nsIServiceManager.h"
#include "nsILoadGroup.h"
#include "nsIScriptSecurityManager.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsICacheService.h"
#include "nsICacheSession.h"
#include "nsIParser.h"
#include "nsThreadUtils.h"

PRLogModuleInfo * gWyciwygLog = nsnull;

#define wyciwyg_TYPE "text/html"
#define LOG(args)  PR_LOG(gWyciwygLog, 4, args)


nsWyciwygChannel::nsWyciwygChannel()
  : mStatus(NS_OK),
    mIsPending(PR_FALSE),
    mNeedToWriteCharset(PR_FALSE),
    mCharsetSource(kCharsetUninitialized),
    mContentLength(-1),
    mLoadFlags(LOAD_NORMAL)
{
}

nsWyciwygChannel::~nsWyciwygChannel() 
{
}

NS_IMPL_ISUPPORTS6(nsWyciwygChannel,
                   nsIChannel,
                   nsIRequest,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsICacheListener, 
                   nsIWyciwygChannel)

nsresult
nsWyciwygChannel::Init(nsIURI* uri)
{
  NS_ENSURE_ARG_POINTER(uri);
  mURI = uri;
  mOriginalURI = uri;
  return NS_OK;
}





NS_IMETHODIMP
nsWyciwygChannel::GetName(nsACString &aName)
{
  return mURI->GetSpec(aName);
}
 
NS_IMETHODIMP
nsWyciwygChannel::IsPending(PRBool *aIsPending)
{
  *aIsPending = mIsPending;
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::GetStatus(nsresult *aStatus)
{
  if (NS_SUCCEEDED(mStatus) && mPump)
    mPump->GetStatus(aStatus);
  else
    *aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::Cancel(nsresult status)
{
  mStatus = status;
  if (mPump)
    mPump->Cancel(status);
  
  return NS_OK;
}
 
NS_IMETHODIMP
nsWyciwygChannel::Suspend()
{
  if (mPump)
    mPump->Suspend();
  
  return NS_OK;
}
 
NS_IMETHODIMP
nsWyciwygChannel::Resume()
{
  if (mPump)
    mPump->Resume();
  
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::GetLoadGroup(nsILoadGroup* *aLoadGroup)
{
  *aLoadGroup = mLoadGroup;
  NS_IF_ADDREF(*aLoadGroup);
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::SetLoadGroup(nsILoadGroup* aLoadGroup)
{
  mLoadGroup = aLoadGroup;
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::SetLoadFlags(PRUint32 aLoadFlags)
{
  mLoadFlags = aLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::GetLoadFlags(PRUint32 * aLoadFlags)
{
  *aLoadFlags = mLoadFlags;
  return NS_OK;
}





NS_IMETHODIMP
nsWyciwygChannel::GetOriginalURI(nsIURI* *aURI)
{
  *aURI = mOriginalURI;
  NS_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::SetOriginalURI(nsIURI* aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  mOriginalURI = aURI;
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::GetURI(nsIURI* *aURI)
{
  *aURI = mURI;
  NS_IF_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::GetOwner(nsISupports **aOwner)
{
  NS_PRECONDITION(mOwner, "Must have a principal!");
  NS_ENSURE_STATE(mOwner);

  NS_ADDREF(*aOwner = mOwner);
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::SetOwner(nsISupports* aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::GetNotificationCallbacks(nsIInterfaceRequestor* *aCallbacks)
{
  *aCallbacks = mCallbacks.get();
  NS_IF_ADDREF(*aCallbacks);
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::SetNotificationCallbacks(nsIInterfaceRequestor* aNotificationCallbacks)
{
  mCallbacks = aNotificationCallbacks;
  mProgressSink = do_GetInterface(mCallbacks);
  return NS_OK;
}

NS_IMETHODIMP 
nsWyciwygChannel::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
  NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);

  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::GetContentType(nsACString &aContentType)
{
  aContentType.AssignLiteral(wyciwyg_TYPE);
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::SetContentType(const nsACString &aContentType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWyciwygChannel::GetContentCharset(nsACString &aContentCharset)
{
  aContentCharset.Assign("UTF-16");
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::SetContentCharset(const nsACString &aContentCharset)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWyciwygChannel::GetContentLength(PRInt32 *aContentLength)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWyciwygChannel::SetContentLength(PRInt32 aContentLength)
{
  mContentLength = aContentLength;

  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::Open(nsIInputStream ** aReturn)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWyciwygChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *ctx)
{
  
  
  
  NS_PRECONDITION(mOwner, "Must have a principal");
  
  LOG(("nsWyciwygChannel::AsyncOpen [this=%x]\n", this));

  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_STATE(mOwner);
  NS_ENSURE_ARG_POINTER(listener);

  nsCAutoString spec;
  mURI->GetSpec(spec);

  
  PRBool delayed = PR_FALSE;
  nsresult rv = OpenCacheEntry(spec, nsICache::ACCESS_READ, &delayed);
  if (rv == NS_ERROR_CACHE_KEY_NOT_FOUND) {
    nsCOMPtr<nsIRunnable> ev =
      new nsRunnableMethod<nsWyciwygChannel>(this,
                                             &nsWyciwygChannel::NotifyListener);
    
    
    rv = NS_DispatchToCurrentThread(ev);
    delayed = PR_TRUE;
  }

  if (NS_FAILED(rv)) {
    LOG(("nsWyciwygChannel::OpenCacheEntry failed [rv=%x]\n", rv));
    return rv;
  }

  if (!delayed) {
    rv = ReadFromCache();
    if (NS_FAILED(rv)) {
      LOG(("nsWyciwygChannel::ReadFromCache failed [rv=%x]\n", rv));
      return rv;
    }
  }

  mIsPending = PR_TRUE;
  mListener = listener;
  mListenerContext = ctx;

  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nsnull);

  return NS_OK;
}





NS_IMETHODIMP
nsWyciwygChannel::WriteToCacheEntry(const nsAString &aData)
{
  nsresult rv;

  if (!mCacheEntry) {
    nsCAutoString spec;
    rv = mURI->GetAsciiSpec(spec);
    if (NS_FAILED(rv)) return rv;
    rv = OpenCacheEntry(spec, nsICache::ACCESS_WRITE);
    if (NS_FAILED(rv)) return rv;
  }

  if (mSecurityInfo) {
    mCacheEntry->SetSecurityInfo(mSecurityInfo);
  }

  if (mNeedToWriteCharset) {
    WriteCharsetAndSourceToCache(mCharsetSource, mCharset);
    mNeedToWriteCharset = PR_FALSE;
  }
  
  PRUint32 out;
  if (!mCacheOutputStream) {
    
    rv = mCacheEntry->OpenOutputStream(0, getter_AddRefs(mCacheOutputStream));    
    if (NS_FAILED(rv)) return rv;

    
    
    PRUnichar bom = 0xFEFF;
    rv = mCacheOutputStream->Write((char *)&bom, sizeof(bom), &out);
    if (NS_FAILED(rv)) return rv;
  }

  return mCacheOutputStream->Write((char *)PromiseFlatString(aData).get(),
                                   aData.Length() * sizeof(PRUnichar), &out);
}


NS_IMETHODIMP
nsWyciwygChannel::CloseCacheEntry(nsresult reason)
{
  if (mCacheEntry) {
    LOG(("nsWyciwygChannel::CloseCacheEntry [this=%x ]", this));
    mCacheOutputStream = 0;
    mCacheInputStream = 0;

    if (NS_FAILED(reason))
      mCacheEntry->Doom();

    mCacheEntry = 0;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::SetSecurityInfo(nsISupports *aSecurityInfo)
{
  mSecurityInfo = aSecurityInfo;

  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::SetCharsetAndSource(PRInt32 aSource,
                                      const nsACString& aCharset)
{
  NS_ENSURE_ARG(!aCharset.IsEmpty());

  if (mCacheEntry) {
    WriteCharsetAndSourceToCache(aSource, PromiseFlatCString(aCharset));
  } else {
    mNeedToWriteCharset = PR_TRUE;
    mCharsetSource = aSource;
    mCharset = aCharset;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsWyciwygChannel::GetCharsetAndSource(PRInt32* aSource, nsACString& aCharset)
{
  if (!mCacheEntry) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsXPIDLCString data;
  mCacheEntry->GetMetaDataElement("charset", getter_Copies(data));

  if (data.IsEmpty()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsXPIDLCString sourceStr;
  mCacheEntry->GetMetaDataElement("charset-source", getter_Copies(sourceStr));

  PRInt32 source;
  
  PRInt32 err;
  source = sourceStr.ToInteger(&err);
  if (NS_FAILED(err) || source == 0) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  *aSource = source;
  aCharset = data;
  return NS_OK;
}




NS_IMETHODIMP
nsWyciwygChannel::OnCacheEntryAvailable(nsICacheEntryDescriptor * aCacheEntry, nsCacheAccessMode aMode, nsresult aStatus)
{
  LOG(("nsWyciwygChannel::OnCacheEntryAvailable [this=%x entry=%x "
       "access=%x status=%x]\n", this, aCacheEntry, aMode, aStatus));

  
  
  if (!mIsPending)
    return NS_OK;

  
  if (NS_SUCCEEDED(aStatus))
    mCacheEntry = aCacheEntry;
  else if (NS_SUCCEEDED(mStatus))
    mStatus = aStatus;

  nsresult rv;
  if (NS_FAILED(mStatus)) {
    LOG(("channel was canceled [this=%x status=%x]\n", this, mStatus));
    rv = mStatus;
  }
  else { 
    rv = ReadFromCache();
  }

  
  if (NS_FAILED(rv)) {
    CloseCacheEntry(rv);

    NotifyListener();
  }

  return NS_OK;
}





NS_IMETHODIMP
nsWyciwygChannel::OnDataAvailable(nsIRequest *request, nsISupports *ctx,
                                  nsIInputStream *input,
                                  PRUint32 offset, PRUint32 count)
{
  LOG(("nsWyciwygChannel::OnDataAvailable [this=%x request=%x offset=%u count=%u]\n",
      this, request, offset, count));

  nsresult rv;
  
  rv = mListener->OnDataAvailable(this, mListenerContext, input, offset, count);

  
  if (mProgressSink && NS_SUCCEEDED(rv) && !(mLoadFlags & LOAD_BACKGROUND))
    mProgressSink->OnProgress(this, nsnull, PRUint64(offset + count),
                              PRUint64(mContentLength));

  return rv; 
}





NS_IMETHODIMP
nsWyciwygChannel::OnStartRequest(nsIRequest *request, nsISupports *ctx)
{
  LOG(("nsWyciwygChannel::OnStartRequest [this=%x request=%x\n",
      this, request));

  return mListener->OnStartRequest(this, mListenerContext);
}


NS_IMETHODIMP
nsWyciwygChannel::OnStopRequest(nsIRequest *request, nsISupports *ctx, nsresult status)
{
  LOG(("nsWyciwygChannel::OnStopRequest [this=%x request=%x status=%d\n",
      this, request, status));

  if (NS_SUCCEEDED(mStatus))
    mStatus = status;

  mListener->OnStopRequest(this, mListenerContext, mStatus);
  mListener = 0;
  mListenerContext = 0;

  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nsnull, mStatus);

  CloseCacheEntry(mStatus);
  mPump = 0;
  mIsPending = PR_FALSE;

  
  mCallbacks = 0;
  mProgressSink = 0;

  return NS_OK;
}





nsresult
nsWyciwygChannel::OpenCacheEntry(const nsACString & aCacheKey,
                                 nsCacheAccessMode aAccessMode,
                                 PRBool * aDelayFlag)
{
  nsresult rv = NS_ERROR_FAILURE;
  
  nsCOMPtr<nsICacheService> cacheService =
    do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString spec;    
  nsAutoString newURIString;    
  nsCOMPtr<nsICacheSession> cacheSession;

  
  nsCacheStoragePolicy storagePolicy;
  if (mLoadFlags & INHIBIT_PERSISTENT_CACHING)
    storagePolicy = nsICache::STORE_IN_MEMORY;
  else
    storagePolicy = nsICache::STORE_ANYWHERE;
 
  
  rv = cacheService->CreateSession("wyciwyg", storagePolicy, PR_TRUE,
                                   getter_AddRefs(cacheSession));
  if (!cacheSession) 
    return NS_ERROR_FAILURE;

  



    
  rv = cacheSession->OpenCacheEntry(aCacheKey, aAccessMode, PR_FALSE,
                                    getter_AddRefs(mCacheEntry));

  if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) {
    
    
    rv = cacheSession->AsyncOpenCacheEntry(aCacheKey, aAccessMode, this);
    if (NS_FAILED(rv)) 
      return rv;
    if (aDelayFlag)
      *aDelayFlag = PR_TRUE;
  }
  else if (rv == NS_OK) {
    LOG(("nsWyciwygChannel::OpenCacheEntry got cache entry \n"));
  }

  return rv;
}

nsresult
nsWyciwygChannel::ReadFromCache()
{
  LOG(("nsWyciwygChannel::ReadFromCache [this=%x] ", this));

  NS_ENSURE_TRUE(mCacheEntry, NS_ERROR_FAILURE);
  nsresult rv;

  
  mCacheEntry->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

  
  rv = mCacheEntry->OpenInputStream(0, getter_AddRefs(mCacheInputStream));
  if (NS_FAILED(rv))
    return rv;
  NS_ENSURE_TRUE(mCacheInputStream, NS_ERROR_UNEXPECTED);

  rv = NS_NewInputStreamPump(getter_AddRefs(mPump), mCacheInputStream);
  if (NS_FAILED(rv)) return rv;

  
  return mPump->AsyncRead(this, nsnull);
}

void
nsWyciwygChannel::WriteCharsetAndSourceToCache(PRInt32 aSource,
                                               const nsCString& aCharset)
{
  NS_PRECONDITION(mCacheEntry, "Better have cache entry!");
  
  mCacheEntry->SetMetaDataElement("charset", aCharset.get());

  nsCAutoString source;
  source.AppendInt(aSource);
  mCacheEntry->SetMetaDataElement("charset-source", source.get());
}

void
nsWyciwygChannel::NotifyListener()
{    
  if (mListener) {
    mListener->OnStartRequest(this, mListenerContext);
    mListener->OnStopRequest(this, mListenerContext, mStatus);
    mListener = 0;
    mListenerContext = 0;
  }

  mIsPending = PR_FALSE;

  
  if (mLoadGroup) {
    mLoadGroup->RemoveRequest(this, nsnull, mStatus);
  }
}


