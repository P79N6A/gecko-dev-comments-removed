









































#include "nsHttp.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/net/HttpChannelChild.h"

#include "nsStringStream.h"
#include "nsHttpHandler.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsSerializationHelper.h"

namespace mozilla {
namespace net {





HttpChannelChild::HttpChannelChild()
  : ChannelEventQueue<HttpChannelChild>(this)
  , mIsFromCache(PR_FALSE)
  , mCacheEntryAvailable(PR_FALSE)
  , mCacheExpirationTime(nsICache::NO_EXPIRATION_TIME)
  , mSendResumeAt(false)
  , mSuspendCount(0)
  , mIPCOpen(false)
  , mKeptAlive(false)
{
  LOG(("Creating HttpChannelChild @%x\n", this));
}

HttpChannelChild::~HttpChannelChild()
{
  LOG(("Destroying HttpChannelChild @%x\n", this));
}






NS_IMPL_ADDREF(HttpChannelChild)

NS_IMETHODIMP_(nsrefcnt) HttpChannelChild::Release()
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  NS_ASSERT_OWNINGTHREAD(HttpChannelChild);
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "HttpChannelChild");

  if (mRefCnt == 1 && mKeptAlive && mIPCOpen) {
    mKeptAlive = false;
    
    
    PHttpChannelChild::Send__delete__(this);
    return 0;
  }

  if (mRefCnt == 0) {
    mRefCnt = 1; 
    delete this;
    return 0;
  }
  return mRefCnt;
}

NS_INTERFACE_MAP_BEGIN(HttpChannelChild)
  NS_INTERFACE_MAP_ENTRY(nsIRequest)
  NS_INTERFACE_MAP_ENTRY(nsIChannel)
  NS_INTERFACE_MAP_ENTRY(nsIHttpChannel)
  NS_INTERFACE_MAP_ENTRY(nsIHttpChannelInternal)
  NS_INTERFACE_MAP_ENTRY(nsICacheInfoChannel)
  NS_INTERFACE_MAP_ENTRY(nsIResumableChannel)
  NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
  NS_INTERFACE_MAP_ENTRY(nsIProxiedChannel)
  NS_INTERFACE_MAP_ENTRY(nsITraceableChannel)
  NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheContainer)
  NS_INTERFACE_MAP_ENTRY(nsIApplicationCacheChannel)
  NS_INTERFACE_MAP_ENTRY(nsIAsyncVerifyRedirectCallback)
  NS_INTERFACE_MAP_ENTRY(nsIChildChannel)
  NS_INTERFACE_MAP_ENTRY(nsIHttpChannelChild)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIAssociatedContentSecurity, GetAssociatedContentSecurity())
NS_INTERFACE_MAP_END_INHERITING(HttpBaseChannel)





void
HttpChannelChild::AddIPDLReference()
{
  NS_ABORT_IF_FALSE(!mIPCOpen, "Attempt to retain more than one IPDL reference");
  mIPCOpen = true;
  AddRef();
}

void
HttpChannelChild::ReleaseIPDLReference()
{
  NS_ABORT_IF_FALSE(mIPCOpen, "Attempt to release nonexistent IPDL reference");
  mIPCOpen = false;
  Release();
}

class StartRequestEvent : public ChannelEvent
{
 public:
  StartRequestEvent(HttpChannelChild* child,
                    const nsHttpResponseHead& responseHead,
                    const PRBool& useResponseHead,
                    const RequestHeaderTuples& requestHeaders,
                    const PRBool& isFromCache,
                    const PRBool& cacheEntryAvailable,
                    const PRUint32& cacheExpirationTime,
                    const nsCString& cachedCharset,
                    const nsCString& securityInfoSerialization)
  : mChild(child)
  , mResponseHead(responseHead)
  , mRequestHeaders(requestHeaders)
  , mUseResponseHead(useResponseHead)
  , mIsFromCache(isFromCache)
  , mCacheEntryAvailable(cacheEntryAvailable)
  , mCacheExpirationTime(cacheExpirationTime)
  , mCachedCharset(cachedCharset)
  , mSecurityInfoSerialization(securityInfoSerialization)
  {}

  void Run() 
  { 
    mChild->OnStartRequest(mResponseHead, mUseResponseHead, mRequestHeaders,
                           mIsFromCache, mCacheEntryAvailable,
                           mCacheExpirationTime, mCachedCharset,
                           mSecurityInfoSerialization);
  }
 private:
  HttpChannelChild* mChild;
  nsHttpResponseHead mResponseHead;
  RequestHeaderTuples mRequestHeaders;
  PRPackedBool mUseResponseHead;
  PRPackedBool mIsFromCache;
  PRPackedBool mCacheEntryAvailable;
  PRUint32 mCacheExpirationTime;
  nsCString mCachedCharset;
  nsCString mSecurityInfoSerialization;
};

bool
HttpChannelChild::RecvAssociateApplicationCache(const nsCString &groupID,
                                                const nsCString &clientID)
{
  nsresult rv;
  mApplicationCache = do_CreateInstance(
    NS_APPLICATIONCACHE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return true;

  mLoadedFromApplicationCache = PR_TRUE;
  mApplicationCache->InitAsHandle(groupID, clientID);
  return true;
}

bool 
HttpChannelChild::RecvOnStartRequest(const nsHttpResponseHead& responseHead,
                                     const PRBool& useResponseHead,
                                     const RequestHeaderTuples& requestHeaders,
                                     const PRBool& isFromCache,
                                     const PRBool& cacheEntryAvailable,
                                     const PRUint32& cacheExpirationTime,
                                     const nsCString& cachedCharset,
                                     const nsCString& securityInfoSerialization)
{
  if (ShouldEnqueue()) {
    EnqueueEvent(new StartRequestEvent(this, responseHead, useResponseHead,
                                       requestHeaders,
                                       isFromCache, cacheEntryAvailable,
                                       cacheExpirationTime, cachedCharset,
                                       securityInfoSerialization));
  } else {
    OnStartRequest(responseHead, useResponseHead, requestHeaders, isFromCache,
                   cacheEntryAvailable, cacheExpirationTime, cachedCharset,
                   securityInfoSerialization);
  }
  return true;
}

void 
HttpChannelChild::OnStartRequest(const nsHttpResponseHead& responseHead,
                                 const PRBool& useResponseHead,
                                 const RequestHeaderTuples& requestHeaders,
                                 const PRBool& isFromCache,
                                 const PRBool& cacheEntryAvailable,
                                 const PRUint32& cacheExpirationTime,
                                 const nsCString& cachedCharset,
                                 const nsCString& securityInfoSerialization)
{
  LOG(("HttpChannelChild::RecvOnStartRequest [this=%x]\n", this));

  if (useResponseHead && !mCanceled)
    mResponseHead = new nsHttpResponseHead(responseHead);

  if (!securityInfoSerialization.IsEmpty()) {
    NS_DeserializeObject(securityInfoSerialization, 
                         getter_AddRefs(mSecurityInfo));
  }
 
  mIsFromCache = isFromCache;
  mCacheEntryAvailable = cacheEntryAvailable;
  mCacheExpirationTime = cacheExpirationTime;
  mCachedCharset = cachedCharset;

  AutoEventEnqueuer ensureSerialDispatch(this);

  
  mRequestHead.ClearHeaders();
  for (PRUint32 i = 0; i < requestHeaders.Length(); i++) {
    mRequestHead.Headers().SetHeader(nsHttp::ResolveAtom(requestHeaders[i].mHeader),
                                     requestHeaders[i].mValue);
  }

  nsresult rv = mListener->OnStartRequest(this, mListenerContext);
  if (NS_FAILED(rv)) {
    Cancel(rv);
    return;
  }

  if (mResponseHead)
    SetCookie(mResponseHead->PeekHeader(nsHttp::Set_Cookie));

  rv = ApplyContentConversions();
  if (NS_FAILED(rv))
    Cancel(rv);
}

class TransportAndDataEvent : public ChannelEvent
{
 public:
  TransportAndDataEvent(HttpChannelChild* child,
                        const nsresult& status,
                        const PRUint64& progress,
                        const PRUint64& progressMax,
                        const nsCString& data,
                        const PRUint32& offset,
                        const PRUint32& count)
  : mChild(child)
  , mStatus(status)
  , mProgress(progress)
  , mProgressMax(progressMax)
  , mData(data)
  , mOffset(offset)
  , mCount(count) {}

  void Run() { mChild->OnTransportAndData(mStatus, mProgress, mProgressMax,
                                          mData, mOffset, mCount); }
 private:
  HttpChannelChild* mChild;
  nsresult mStatus;
  PRUint64 mProgress;
  PRUint64 mProgressMax;
  nsCString mData;
  PRUint32 mOffset;
  PRUint32 mCount;
};

bool
HttpChannelChild::RecvOnTransportAndData(const nsresult& status,
                                         const PRUint64& progress,
                                         const PRUint64& progressMax,
                                         const nsCString& data,
                                         const PRUint32& offset,
                                         const PRUint32& count)
{
  if (ShouldEnqueue()) {
    EnqueueEvent(new TransportAndDataEvent(this, status, progress, progressMax,
                                           data, offset, count));
  } else {
    OnTransportAndData(status, progress, progressMax, data, offset, count);
  }
  return true;
}

void
HttpChannelChild::OnTransportAndData(const nsresult& status,
                                     const PRUint64 progress,
                                     const PRUint64& progressMax,
                                     const nsCString& data,
                                     const PRUint32& offset,
                                     const PRUint32& count)
{
  LOG(("HttpChannelChild::OnTransportAndData [this=%x]\n", this));

  if (mCanceled)
    return;

  
  if (!mProgressSink)
    GetCallback(mProgressSink);

  
  
  AutoEventEnqueuer ensureSerialDispatch(this);

  
  
  
  
  
  if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending &&
      !(mLoadFlags & LOAD_BACKGROUND))
  {
    
    
    NS_ASSERTION(status == nsISocketTransport::STATUS_RECEIVING_FROM ||
                 status == nsITransport::STATUS_READING,
                 "unexpected status code");

    nsCAutoString host;
    mURI->GetHost(host);
    mProgressSink->OnStatus(this, nsnull, status,
                            NS_ConvertUTF8toUTF16(host).get());
    
    
    if (progress > 0) {
      NS_ASSERTION(progress <= progressMax, "unexpected progress values");
      mProgressSink->OnProgress(this, nsnull, progress, progressMax);
    }
  }

  
  
  
  
  
  
  
  nsCOMPtr<nsIInputStream> stringStream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stringStream), data.get(),
                                      count, NS_ASSIGNMENT_DEPEND);
  if (NS_FAILED(rv)) {
    Cancel(rv);
    return;
  }

  rv = mListener->OnDataAvailable(this, mListenerContext,
                                  stringStream, offset, count);
  stringStream->Close();
  if (NS_FAILED(rv)) {
    Cancel(rv);
  }
}

class StopRequestEvent : public ChannelEvent
{
 public:
  StopRequestEvent(HttpChannelChild* child,
                   const nsresult& statusCode)
  : mChild(child)
  , mStatusCode(statusCode) {}

  void Run() { mChild->OnStopRequest(mStatusCode); }
 private:
  HttpChannelChild* mChild;
  nsresult mStatusCode;
};

bool 
HttpChannelChild::RecvOnStopRequest(const nsresult& statusCode)
{
  if (ShouldEnqueue()) {
    EnqueueEvent(new StopRequestEvent(this, statusCode));
  } else {
    OnStopRequest(statusCode);
  }
  return true;
}

void 
HttpChannelChild::OnStopRequest(const nsresult& statusCode)
{
  LOG(("HttpChannelChild::OnStopRequest [this=%x status=%u]\n", 
           this, statusCode));

  mIsPending = PR_FALSE;

  if (!mCanceled && NS_SUCCEEDED(mStatus))
    mStatus = statusCode;

  { 
    
    
    AutoEventEnqueuer ensureSerialDispatch(this);

    mListener->OnStopRequest(this, mListenerContext, mStatus);

    mListener = 0;
    mListenerContext = 0;
    mCacheEntryAvailable = PR_FALSE;
    if (mLoadGroup)
      mLoadGroup->RemoveRequest(this, nsnull, mStatus);
  }

  if (!(mLoadFlags & LOAD_DOCUMENT_URI)) {
    
    
    PHttpChannelChild::Send__delete__(this);
  } else {
    
    
    mKeptAlive = true;
    SendDocumentChannelCleanup();
  }
}

class ProgressEvent : public ChannelEvent
{
 public:
  ProgressEvent(HttpChannelChild* child,
                const PRUint64& progress,
                const PRUint64& progressMax)
  : mChild(child)
  , mProgress(progress)
  , mProgressMax(progressMax) {}

  void Run() { mChild->OnProgress(mProgress, mProgressMax); }
 private:
  HttpChannelChild* mChild;
  PRUint64 mProgress, mProgressMax;
};

bool
HttpChannelChild::RecvOnProgress(const PRUint64& progress,
                                 const PRUint64& progressMax)
{
  if (ShouldEnqueue())  {
    EnqueueEvent(new ProgressEvent(this, progress, progressMax));
  } else {
    OnProgress(progress, progressMax);
  }
  return true;
}

void
HttpChannelChild::OnProgress(const PRUint64& progress,
                             const PRUint64& progressMax)
{
  LOG(("HttpChannelChild::OnProgress [this=%p progress=%llu/%llu]\n",
       this, progress, progressMax));

  if (mCanceled)
    return;

  
  if (!mProgressSink)
    GetCallback(mProgressSink);

  AutoEventEnqueuer ensureSerialDispatch(this);

  
  
  if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending && 
      !(mLoadFlags & LOAD_BACKGROUND)) 
  {
    if (progress > 0) {
      NS_ASSERTION(progress <= progressMax, "unexpected progress values");
      mProgressSink->OnProgress(this, nsnull, progress, progressMax);
    }
  }
}

class StatusEvent : public ChannelEvent
{
 public:
  StatusEvent(HttpChannelChild* child,
              const nsresult& status)
  : mChild(child)
  , mStatus(status) {}

  void Run() { mChild->OnStatus(mStatus); }
 private:
  HttpChannelChild* mChild;
  nsresult mStatus;
};

bool
HttpChannelChild::RecvOnStatus(const nsresult& status)
{
  if (ShouldEnqueue()) {
    EnqueueEvent(new StatusEvent(this, status));
  } else {
    OnStatus(status);
  }
  return true;
}

void
HttpChannelChild::OnStatus(const nsresult& status)
{
  LOG(("HttpChannelChild::OnStatus [this=%p status=%x]\n", this, status));

  if (mCanceled)
    return;

  
  if (!mProgressSink)
    GetCallback(mProgressSink);

  AutoEventEnqueuer ensureSerialDispatch(this);

  
  
  if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending && 
      !(mLoadFlags & LOAD_BACKGROUND)) 
  {
    nsCAutoString host;
    mURI->GetHost(host);
    mProgressSink->OnStatus(this, nsnull, status,
                            NS_ConvertUTF8toUTF16(host).get());
  }
}

class CancelEvent : public ChannelEvent
{
 public:
  CancelEvent(HttpChannelChild* child, const nsresult& status)
  : mChild(child)
  , mStatus(status) {}

  void Run() { mChild->OnCancel(mStatus); }
 private:
  HttpChannelChild* mChild;
  nsresult mStatus;
};

bool
HttpChannelChild::RecvCancelEarly(const nsresult& status)
{
  if (ShouldEnqueue()) {
    EnqueueEvent(new CancelEvent(this, status));
  } else {
    OnCancel(status);
  }
  return true;
}

void
HttpChannelChild::OnCancel(const nsresult& status)
{
  LOG(("HttpChannelChild::OnCancel [this=%p status=%x]\n", this, status));

  if (mCanceled)
    return;

  mCanceled = true;
  mStatus = status;

  mIsPending = false;
  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nsnull, mStatus);

  if (mListener) {
    mListener->OnStartRequest(this, mListenerContext);
    mListener->OnStopRequest(this, mListenerContext, mStatus);
  }

  mListener = NULL;
  mListenerContext = NULL;

  if (mIPCOpen)
    PHttpChannelChild::Send__delete__(this);
}

class DeleteSelfEvent : public ChannelEvent
{
 public:
  DeleteSelfEvent(HttpChannelChild* child) : mChild(child) {}
  void Run() { mChild->DeleteSelf(); }
 private:
  HttpChannelChild* mChild;
};

bool
HttpChannelChild::RecvDeleteSelf()
{
  if (ShouldEnqueue()) {
    EnqueueEvent(new DeleteSelfEvent(this));
  } else {
    DeleteSelf();
  }
  return true;
}

void
HttpChannelChild::DeleteSelf()
{
  Send__delete__(this);
}

class Redirect1Event : public ChannelEvent
{
 public:
  Redirect1Event(HttpChannelChild* child,
                 const PRUint32& newChannelId,
                 const IPC::URI& newURI,
                 const PRUint32& redirectFlags,
                 const nsHttpResponseHead& responseHead)
  : mChild(child)
  , mNewChannelId(newChannelId)
  , mNewURI(newURI)
  , mRedirectFlags(redirectFlags)
  , mResponseHead(responseHead) {}

  void Run() 
  { 
    mChild->Redirect1Begin(mNewChannelId, mNewURI, mRedirectFlags,
                           mResponseHead); 
  }
 private:
  HttpChannelChild*   mChild;
  PRUint32            mNewChannelId;
  IPC::URI            mNewURI;
  PRUint32            mRedirectFlags;
  nsHttpResponseHead  mResponseHead;
};

bool
HttpChannelChild::RecvRedirect1Begin(const PRUint32& newChannelId,
                                     const URI& newUri,
                                     const PRUint32& redirectFlags,
                                     const nsHttpResponseHead& responseHead)
{
  if (ShouldEnqueue()) {
    EnqueueEvent(new Redirect1Event(this, newChannelId, newUri, redirectFlags,
                                    responseHead)); 
  } else {
    Redirect1Begin(newChannelId, newUri, redirectFlags, responseHead);
  }
  return true;
}

void
HttpChannelChild::Redirect1Begin(const PRUint32& newChannelId,
                                 const IPC::URI& newURI,
                                 const PRUint32& redirectFlags,
                                 const nsHttpResponseHead& responseHead)
{
  nsresult rv;

  nsCOMPtr<nsIIOService> ioService;
  rv = gHttpHandler->GetIOService(getter_AddRefs(ioService));
  if (NS_FAILED(rv)) {
    
    OnRedirectVerifyCallback(rv);
    return;
  }

  nsCOMPtr<nsIURI> uri(newURI);

  nsCOMPtr<nsIChannel> newChannel;
  rv = ioService->NewChannelFromURI(uri, getter_AddRefs(newChannel));
  if (NS_FAILED(rv)) {
    
    OnRedirectVerifyCallback(rv);
    return;
  }

  
  mResponseHead = new nsHttpResponseHead(responseHead);
  SetCookie(mResponseHead->PeekHeader(nsHttp::Set_Cookie));

  PRBool preserveMethod = (mResponseHead->Status() == 307);
  rv = SetupReplacementChannel(uri, newChannel, preserveMethod);
  if (NS_FAILED(rv)) {
    
    OnRedirectVerifyCallback(rv);
    return;
  }

  mRedirectChannelChild = do_QueryInterface(newChannel);
  if (mRedirectChannelChild) {
    mRedirectChannelChild->ConnectParent(newChannelId);
  } else {
    NS_ERROR("Redirecting to a protocol that doesn't support universal protocol redirect");
  }

  rv = gHttpHandler->AsyncOnChannelRedirect(this, 
                                            newChannel,
                                            redirectFlags);
  if (NS_FAILED(rv))
    OnRedirectVerifyCallback(rv);
}

class Redirect3Event : public ChannelEvent
{
 public:
  Redirect3Event(HttpChannelChild* child) : mChild(child) {}
  void Run() { mChild->Redirect3Complete(); }
 private:
  HttpChannelChild* mChild;
};

bool
HttpChannelChild::RecvRedirect3Complete()
{
  if (ShouldEnqueue()) {
    EnqueueEvent(new Redirect3Event(this));
  } else {
    Redirect3Complete();
  }
  return true;
}

void
HttpChannelChild::Redirect3Complete()
{
  nsresult rv = NS_OK;

  
  if (mRedirectChannelChild)
    rv = mRedirectChannelChild->CompleteRedirectSetup(mListener,
                                                      mListenerContext);

  
  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nsnull, NS_BINDING_ABORTED);

  if (NS_FAILED(rv))
    NS_WARNING("CompleteRedirectSetup failed, HttpChannelChild already open?");

  
  mRedirectChannelChild = nsnull;
}





NS_IMETHODIMP
HttpChannelChild::ConnectParent(PRUint32 id)
{
  mozilla::dom::TabChild* tabChild = nsnull;
  nsCOMPtr<nsITabChild> iTabChild;
  GetCallback(iTabChild);
  if (iTabChild) {
    tabChild = static_cast<mozilla::dom::TabChild*>(iTabChild.get());
  }

  
  
  AddIPDLReference();

  if (!gNeckoChild->SendPHttpChannelConstructor(this, tabChild))
    return NS_ERROR_FAILURE;

  if (!SendConnectChannel(id))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::CompleteRedirectSetup(nsIStreamListener *listener, 
                                        nsISupports *aContext)
{
  LOG(("HttpChannelChild::FinishRedirectSetup [this=%x]\n", this));

  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  






  mIsPending = PR_TRUE;
  mWasOpened = PR_TRUE;
  mListener = listener;
  mListenerContext = aContext;

  
  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nsnull);

  
  
  
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::OnRedirectVerifyCallback(nsresult result)
{
  nsCOMPtr<nsIHttpChannel> newHttpChannel =
      do_QueryInterface(mRedirectChannelChild);

  if (newHttpChannel) {
    
    newHttpChannel->SetOriginalURI(mRedirectOriginalURI);
  }

  RequestHeaderTuples emptyHeaders;
  RequestHeaderTuples* headerTuples = &emptyHeaders;

  nsCOMPtr<nsIHttpChannelChild> newHttpChannelChild =
      do_QueryInterface(mRedirectChannelChild);
  if (newHttpChannelChild && NS_SUCCEEDED(result)) {
    newHttpChannelChild->AddCookiesToRequest();
    newHttpChannelChild->GetHeaderTuples(&headerTuples);
  }

  
  
  if (NS_SUCCEEDED(result))
    gHttpHandler->OnModifyRequest(newHttpChannel);

  return SendRedirect2Verify(result, *headerTuples);
}





NS_IMETHODIMP
HttpChannelChild::Cancel(nsresult status)
{
  if (!mCanceled) {
    
    
    mCanceled = true;
    mStatus = status;
    if (mIPCOpen)
      SendCancel(status);
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::Suspend()
{
  NS_ENSURE_TRUE(mIPCOpen, NS_ERROR_NOT_AVAILABLE);
  SendSuspend();
  mSuspendCount++;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::Resume()
{
  NS_ENSURE_TRUE(mIPCOpen, NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_TRUE(mSuspendCount > 0, NS_ERROR_UNEXPECTED);
  SendResume();
  mSuspendCount--;
  if (!mSuspendCount) {
    
    
    
    
    if (mQueuePhase == PHASE_UNQUEUED)
      mQueuePhase = PHASE_FINISHED_QUEUEING;
    FlushEventQueue();
  }
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::GetSecurityInfo(nsISupports **aSecurityInfo)
{
  NS_ENSURE_ARG_POINTER(aSecurityInfo);
  NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::AsyncOpen(nsIStreamListener *listener, nsISupports *aContext)
{
  LOG(("HttpChannelChild::AsyncOpen [this=%x uri=%s]\n", this, mSpec.get()));

  if (mCanceled)
    return mStatus;

  NS_ENSURE_TRUE(gNeckoChild != nsnull, NS_ERROR_FAILURE);
  NS_ENSURE_ARG_POINTER(listener);
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  
  
  nsresult rv;
  rv = NS_CheckPortSafety(mURI);
  if (NS_FAILED(rv))
    return rv;

  const char *cookieHeader = mRequestHead.PeekHeader(nsHttp::Cookie);
  if (cookieHeader) {
    mUserSetCookieHeader = cookieHeader;
  }

  AddCookiesToRequest();

  
  
  
  

  
  gHttpHandler->OnModifyRequest(this);

  mIsPending = PR_TRUE;
  mWasOpened = PR_TRUE;
  mListener = listener;
  mListenerContext = aContext;

  
  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nsnull);

  if (mCanceled) {
    
    
    

    
    mCanceled = false;
    OnCancel(mStatus);
    return NS_OK;
  }

  nsCString appCacheClientId;
  if (mInheritApplicationCache) {
    
    
    nsCOMPtr<nsIApplicationCacheContainer> appCacheContainer;
    GetCallback(appCacheContainer);

    if (appCacheContainer) {
      nsCOMPtr<nsIApplicationCache> appCache;
      rv = appCacheContainer->GetApplicationCache(getter_AddRefs(appCache));
      if (NS_SUCCEEDED(rv) && appCache) {
        appCache->GetClientID(appCacheClientId);
      }
    }
  }

  
  
  

  

  mozilla::dom::TabChild* tabChild = nsnull;
  nsCOMPtr<nsITabChild> iTabChild;
  GetCallback(iTabChild);
  if (iTabChild) {
    tabChild = static_cast<mozilla::dom::TabChild*>(iTabChild.get());
  }

  
  
  AddIPDLReference();

  gNeckoChild->SendPHttpChannelConstructor(this, tabChild);

  SendAsyncOpen(IPC::URI(mURI), IPC::URI(mOriginalURI),
                IPC::URI(mDocumentURI), IPC::URI(mReferrer), mLoadFlags,
                mRequestHeaders, mRequestHead.Method(),
                IPC::InputStream(mUploadStream), mUploadStreamHasHeaders,
                mPriority, mRedirectionLimit, mAllowPipelining,
                mForceAllowThirdPartyCookie, mSendResumeAt,
                mStartPos, mEntityID, mChooseApplicationCache, 
                appCacheClientId);

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
HttpChannelChild::GetRemoteAddress(nsACString & _result)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
HttpChannelChild::GetRemotePort(PRInt32 * _result)
{
  NS_ENSURE_ARG_POINTER(_result);
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
HttpChannelChild::GetLocalAddress(nsACString & _result)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
HttpChannelChild::GetLocalPort(PRInt32 * _result)
{
  NS_ENSURE_ARG_POINTER(_result);
  return NS_ERROR_NOT_AVAILABLE;
}






NS_IMETHODIMP
HttpChannelChild::GetCacheTokenExpirationTime(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if (!mCacheEntryAvailable)
    return NS_ERROR_NOT_AVAILABLE;

  *_retval = mCacheExpirationTime;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetCacheTokenCachedCharset(nsACString &_retval)
{
  if (!mCacheEntryAvailable)
    return NS_ERROR_NOT_AVAILABLE;

  _retval = mCachedCharset;
  return NS_OK;
}
NS_IMETHODIMP
HttpChannelChild::SetCacheTokenCachedCharset(const nsACString &aCharset)
{
  if (!mCacheEntryAvailable || !mIPCOpen)
    return NS_ERROR_NOT_AVAILABLE;

  mCachedCharset = aCharset;
  if (!SendSetCacheTokenCachedCharset(PromiseFlatCString(aCharset))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::IsFromCache(PRBool *value)
{
  if (!mIsPending)
    return NS_ERROR_NOT_AVAILABLE;

  *value = mIsFromCache;
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::ResumeAt(PRUint64 startPos, const nsACString& entityID)
{
  ENSURE_CALLED_BEFORE_ASYNC_OPEN();
  mStartPos = startPos;
  mEntityID = entityID;
  mSendResumeAt = true;
  return NS_OK;
}







NS_IMETHODIMP
HttpChannelChild::SetPriority(PRInt32 aPriority)
{
  PRInt16 newValue = CLAMP(aPriority, PR_INT16_MIN, PR_INT16_MAX);
  if (mPriority == newValue)
    return NS_OK;
  mPriority = newValue;
  if (mIPCOpen) 
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
  NS_IF_ADDREF(*aApplicationCache = mApplicationCache);
  return NS_OK;
}
NS_IMETHODIMP
HttpChannelChild::SetApplicationCache(nsIApplicationCache *aApplicationCache)
{
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  mApplicationCache = aApplicationCache;
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::GetLoadedFromApplicationCache(PRBool *aLoadedFromApplicationCache)
{
  *aLoadedFromApplicationCache = mLoadedFromApplicationCache;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetInheritApplicationCache(PRBool *aInherit)
{
  *aInherit = mInheritApplicationCache;
  return NS_OK;
}
NS_IMETHODIMP
HttpChannelChild::SetInheritApplicationCache(PRBool aInherit)
{
  mInheritApplicationCache = aInherit;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetChooseApplicationCache(PRBool *aChoose)
{
  *aChoose = mChooseApplicationCache;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::SetChooseApplicationCache(PRBool aChoose)
{
  mChooseApplicationCache = aChoose;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::MarkOfflineCacheEntryAsForeign()
{
  SendMarkOfflineCacheEntryAsForeign();
  return NS_OK;
}





bool
HttpChannelChild::GetAssociatedContentSecurity(
                    nsIAssociatedContentSecurity** _result)
{
  if (!mSecurityInfo)
    return false;

  nsCOMPtr<nsIAssociatedContentSecurity> assoc =
      do_QueryInterface(mSecurityInfo);
  if (!assoc)
    return false;

  if (_result)
    assoc.forget(_result);
  return true;
}


NS_IMETHODIMP
HttpChannelChild::GetCountSubRequestsHighSecurity(
                    PRInt32 *aSubRequestsHighSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->GetCountSubRequestsHighSecurity(aSubRequestsHighSecurity);
}
NS_IMETHODIMP
HttpChannelChild::SetCountSubRequestsHighSecurity(
                    PRInt32 aSubRequestsHighSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->SetCountSubRequestsHighSecurity(aSubRequestsHighSecurity);
}


NS_IMETHODIMP
HttpChannelChild::GetCountSubRequestsLowSecurity(
                    PRInt32 *aSubRequestsLowSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->GetCountSubRequestsLowSecurity(aSubRequestsLowSecurity);
}
NS_IMETHODIMP
HttpChannelChild::SetCountSubRequestsLowSecurity(
                    PRInt32 aSubRequestsLowSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->SetCountSubRequestsLowSecurity(aSubRequestsLowSecurity);
}


NS_IMETHODIMP 
HttpChannelChild::GetCountSubRequestsBrokenSecurity(
                    PRInt32 *aSubRequestsBrokenSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->GetCountSubRequestsBrokenSecurity(aSubRequestsBrokenSecurity);
}
NS_IMETHODIMP 
HttpChannelChild::SetCountSubRequestsBrokenSecurity(
                    PRInt32 aSubRequestsBrokenSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->SetCountSubRequestsBrokenSecurity(aSubRequestsBrokenSecurity);
}


NS_IMETHODIMP
HttpChannelChild::GetCountSubRequestsNoSecurity(PRInt32 *aSubRequestsNoSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->GetCountSubRequestsNoSecurity(aSubRequestsNoSecurity);
}
NS_IMETHODIMP
HttpChannelChild::SetCountSubRequestsNoSecurity(PRInt32 aSubRequestsNoSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->SetCountSubRequestsNoSecurity(aSubRequestsNoSecurity);
}

NS_IMETHODIMP
HttpChannelChild::Flush()
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  nsresult rv;
  PRInt32 hi, low, broken, no;

  rv = assoc->GetCountSubRequestsHighSecurity(&hi);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = assoc->GetCountSubRequestsLowSecurity(&low);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = assoc->GetCountSubRequestsBrokenSecurity(&broken);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = assoc->GetCountSubRequestsNoSecurity(&no);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mIPCOpen)
    SendUpdateAssociatedContentSecurity(hi, low, broken, no);

  return NS_OK;
}





NS_IMETHODIMP HttpChannelChild::AddCookiesToRequest()
{
  HttpBaseChannel::AddCookiesToRequest();
  return NS_OK;
}

NS_IMETHODIMP HttpChannelChild::GetHeaderTuples(RequestHeaderTuples **aHeaderTuples)
{
  *aHeaderTuples = &mRequestHeaders;
  return NS_OK;
}



}} 
