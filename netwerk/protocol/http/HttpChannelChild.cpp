







#include "HttpLog.h"

#include "nsHttp.h"
#include "nsICacheEntry.h"
#include "mozilla/unused.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/ipc/FileDescriptorSetChild.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/net/HttpChannelChild.h"

#include "nsStringStream.h"
#include "nsHttpHandler.h"
#include "nsNetUtil.h"
#include "nsSerializationHelper.h"
#include "mozilla/Attributes.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/URIUtils.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "mozilla/net/ChannelDiverterChild.h"
#include "mozilla/net/DNS.h"
#include "SerializedLoadContext.h"

using namespace mozilla::dom;
using namespace mozilla::ipc;

namespace mozilla {
namespace net {





HttpChannelChild::HttpChannelChild()
  : HttpAsyncAborter<HttpChannelChild>(MOZ_THIS_IN_INITIALIZER_LIST())
  , mIsFromCache(false)
  , mCacheEntryAvailable(false)
  , mCacheExpirationTime(nsICacheEntry::NO_EXPIRATION_TIME)
  , mSendResumeAt(false)
  , mIPCOpen(false)
  , mKeptAlive(false)
  , mDivertingToParent(false)
  , mFlushedForDiversion(false)
  , mSuspendSent(false)
{
  LOG(("Creating HttpChannelChild @%x\n", this));

  mChannelCreationTime = PR_Now();
  mChannelCreationTimestamp = TimeStamp::Now();
  mAsyncOpenTime = TimeStamp::Now();
  mEventQ = new ChannelEventQueue(static_cast<nsIHttpChannel*>(this));
}

HttpChannelChild::~HttpChannelChild()
{
  LOG(("Destroying HttpChannelChild @%x\n", this));
}






NS_IMPL_ADDREF(HttpChannelChild)

NS_IMETHODIMP_(MozExternalRefCountType) HttpChannelChild::Release()
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  NS_ASSERT_OWNINGTHREAD(HttpChannelChild);
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "HttpChannelChild");

  
  
  
  
  if (mKeptAlive && mRefCnt == 1 && mIPCOpen) {
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
  NS_INTERFACE_MAP_ENTRY(nsIDivertableChannel)
NS_INTERFACE_MAP_END_INHERITING(HttpBaseChannel)





void
HttpChannelChild::AddIPDLReference()
{
  MOZ_ASSERT(!mIPCOpen, "Attempt to retain more than one IPDL reference");
  mIPCOpen = true;
  AddRef();
}

void
HttpChannelChild::ReleaseIPDLReference()
{
  MOZ_ASSERT(mIPCOpen, "Attempt to release nonexistent IPDL reference");
  mIPCOpen = false;
  Release();
}

class AssociateApplicationCacheEvent : public ChannelEvent
{
  public:
    AssociateApplicationCacheEvent(HttpChannelChild* child,
                                   const nsCString &groupID,
                                   const nsCString &clientID)
    : mChild(child)
    , groupID(groupID)
    , clientID(clientID) {}

    void Run() { mChild->AssociateApplicationCache(groupID, clientID); }
  private:
    HttpChannelChild* mChild;
    nsCString groupID;
    nsCString clientID;
};

bool
HttpChannelChild::RecvAssociateApplicationCache(const nsCString &groupID,
                                                const nsCString &clientID)
{
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new AssociateApplicationCacheEvent(this, groupID, clientID));
  } else {
    AssociateApplicationCache(groupID, clientID);
  }
  return true;
}

void
HttpChannelChild::AssociateApplicationCache(const nsCString &groupID,
                                            const nsCString &clientID)
{
  nsresult rv;
  mApplicationCache = do_CreateInstance(NS_APPLICATIONCACHE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return;

  mLoadedFromApplicationCache = true;
  mApplicationCache->InitAsHandle(groupID, clientID);
}

class StartRequestEvent : public ChannelEvent
{
 public:
  StartRequestEvent(HttpChannelChild* child,
                    const nsresult& channelStatus,
                    const nsHttpResponseHead& responseHead,
                    const bool& useResponseHead,
                    const nsHttpHeaderArray& requestHeaders,
                    const bool& isFromCache,
                    const bool& cacheEntryAvailable,
                    const uint32_t& cacheExpirationTime,
                    const nsCString& cachedCharset,
                    const nsCString& securityInfoSerialization,
                    const NetAddr& selfAddr,
                    const NetAddr& peerAddr)
  : mChild(child)
  , mChannelStatus(channelStatus)
  , mResponseHead(responseHead)
  , mRequestHeaders(requestHeaders)
  , mUseResponseHead(useResponseHead)
  , mIsFromCache(isFromCache)
  , mCacheEntryAvailable(cacheEntryAvailable)
  , mCacheExpirationTime(cacheExpirationTime)
  , mCachedCharset(cachedCharset)
  , mSecurityInfoSerialization(securityInfoSerialization)
  , mSelfAddr(selfAddr)
  , mPeerAddr(peerAddr)
  {}

  void Run()
  {
    mChild->OnStartRequest(mChannelStatus, mResponseHead, mUseResponseHead,
                           mRequestHeaders, mIsFromCache, mCacheEntryAvailable,
                           mCacheExpirationTime, mCachedCharset,
                           mSecurityInfoSerialization, mSelfAddr, mPeerAddr);
  }
 private:
  HttpChannelChild* mChild;
  nsresult mChannelStatus;
  nsHttpResponseHead mResponseHead;
  nsHttpHeaderArray mRequestHeaders;
  bool mUseResponseHead;
  bool mIsFromCache;
  bool mCacheEntryAvailable;
  uint32_t mCacheExpirationTime;
  nsCString mCachedCharset;
  nsCString mSecurityInfoSerialization;
  NetAddr mSelfAddr;
  NetAddr mPeerAddr;
};

bool
HttpChannelChild::RecvOnStartRequest(const nsresult& channelStatus,
                                     const nsHttpResponseHead& responseHead,
                                     const bool& useResponseHead,
                                     const nsHttpHeaderArray& requestHeaders,
                                     const bool& isFromCache,
                                     const bool& cacheEntryAvailable,
                                     const uint32_t& cacheExpirationTime,
                                     const nsCString& cachedCharset,
                                     const nsCString& securityInfoSerialization,
                                     const NetAddr& selfAddr,
                                     const NetAddr& peerAddr,
                                     const int16_t& redirectCount)
{
  
  
  MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
    "mFlushedForDiversion should be unset before OnStartRequest!");
  MOZ_RELEASE_ASSERT(!mDivertingToParent,
    "mDivertingToParent should be unset before OnStartRequest!");


  mRedirectCount = redirectCount;

  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new StartRequestEvent(this, channelStatus, responseHead,
                                           useResponseHead, requestHeaders,
                                           isFromCache, cacheEntryAvailable,
                                           cacheExpirationTime, cachedCharset,
                                           securityInfoSerialization, selfAddr,
                                           peerAddr));
  } else {
    OnStartRequest(channelStatus, responseHead, useResponseHead, requestHeaders,
                   isFromCache, cacheEntryAvailable, cacheExpirationTime,
                   cachedCharset, securityInfoSerialization, selfAddr,
                   peerAddr);
  }
  return true;
}

void
HttpChannelChild::OnStartRequest(const nsresult& channelStatus,
                                 const nsHttpResponseHead& responseHead,
                                 const bool& useResponseHead,
                                 const nsHttpHeaderArray& requestHeaders,
                                 const bool& isFromCache,
                                 const bool& cacheEntryAvailable,
                                 const uint32_t& cacheExpirationTime,
                                 const nsCString& cachedCharset,
                                 const nsCString& securityInfoSerialization,
                                 const NetAddr& selfAddr,
                                 const NetAddr& peerAddr)
{
  LOG(("HttpChannelChild::RecvOnStartRequest [this=%p]\n", this));

  
  
  MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
    "mFlushedForDiversion should be unset before OnStartRequest!");
  MOZ_RELEASE_ASSERT(!mDivertingToParent,
    "mDivertingToParent should be unset before OnStartRequest!");

  if (!mCanceled && NS_SUCCEEDED(mStatus)) {
    mStatus = channelStatus;
  }

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

  AutoEventEnqueuer ensureSerialDispatch(mEventQ);

  
  mRequestHead.Headers() = requestHeaders;

  
  
  
  

  mTracingEnabled = false;

  nsresult rv = mListener->OnStartRequest(this, mListenerContext);
  if (NS_FAILED(rv)) {
    Cancel(rv);
    return;
  }

  if (mResponseHead)
    SetCookie(mResponseHead->PeekHeader(nsHttp::Set_Cookie));

  mSelfAddr = selfAddr;
  mPeerAddr = peerAddr;

  if (mDivertingToParent) {
    mListener = nullptr;
    mListenerContext = nullptr;
    if (mLoadGroup) {
      mLoadGroup->RemoveRequest(this, nullptr, mStatus);
    }
    return;
  }

  nsCOMPtr<nsIStreamListener> listener;
  rv = DoApplyContentConversions(mListener, getter_AddRefs(listener),
                                 mListenerContext);
  if (NS_FAILED(rv)) {
    Cancel(rv);
  } else if (listener) {
    mListener = listener;
  }
}

class TransportAndDataEvent : public ChannelEvent
{
 public:
  TransportAndDataEvent(HttpChannelChild* child,
                        const nsresult& channelStatus,
                        const nsresult& transportStatus,
                        const uint64_t& progress,
                        const uint64_t& progressMax,
                        const nsCString& data,
                        const uint64_t& offset,
                        const uint32_t& count)
  : mChild(child)
  , mChannelStatus(channelStatus)
  , mTransportStatus(transportStatus)
  , mProgress(progress)
  , mProgressMax(progressMax)
  , mData(data)
  , mOffset(offset)
  , mCount(count) {}

  void Run()
  {
    mChild->OnTransportAndData(mChannelStatus, mTransportStatus, mProgress,
                               mProgressMax, mData, mOffset, mCount);
  }
 private:
  HttpChannelChild* mChild;
  nsresult mChannelStatus;
  nsresult mTransportStatus;
  uint64_t mProgress;
  uint64_t mProgressMax;
  nsCString mData;
  uint64_t mOffset;
  uint32_t mCount;
};

bool
HttpChannelChild::RecvOnTransportAndData(const nsresult& channelStatus,
                                         const nsresult& transportStatus,
                                         const uint64_t& progress,
                                         const uint64_t& progressMax,
                                         const nsCString& data,
                                         const uint64_t& offset,
                                         const uint32_t& count)
{
  MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
                     "Should not be receiving any more callbacks from parent!");

  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new TransportAndDataEvent(this, channelStatus,
                                               transportStatus, progress,
                                               progressMax, data, offset,
                                               count));
  } else {
    MOZ_RELEASE_ASSERT(!mDivertingToParent,
                       "ShouldEnqueue when diverting to parent!");

    OnTransportAndData(channelStatus, transportStatus, progress, progressMax,
                       data, offset, count);
  }
  return true;
}

void
HttpChannelChild::OnTransportAndData(const nsresult& channelStatus,
                                     const nsresult& transportStatus,
                                     const uint64_t progress,
                                     const uint64_t& progressMax,
                                     const nsCString& data,
                                     const uint64_t& offset,
                                     const uint32_t& count)
{
  LOG(("HttpChannelChild::OnTransportAndData [this=%p]\n", this));

  if (!mCanceled && NS_SUCCEEDED(mStatus)) {
    mStatus = channelStatus;
  }

  
  if (mDivertingToParent) {
    MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
      "Should not be processing any more callbacks from parent!");

    SendDivertOnDataAvailable(data, offset, count);
    return;
  }

  if (mCanceled)
    return;

  
  if (!mProgressSink) {
    GetCallback(mProgressSink);
  }

  
  
  AutoEventEnqueuer ensureSerialDispatch(mEventQ);

  
  
  
  
  
  
  
  if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending &&
      !(mLoadFlags & LOAD_BACKGROUND))
  {
    
    
    MOZ_ASSERT(transportStatus == NS_NET_STATUS_RECEIVING_FROM ||
               transportStatus == NS_NET_STATUS_READING);

    nsAutoCString host;
    mURI->GetHost(host);
    mProgressSink->OnStatus(this, nullptr, transportStatus,
                            NS_ConvertUTF8toUTF16(host).get());
    
    
    if (progress > 0) {
      MOZ_ASSERT(progress <= progressMax, "unexpected progress values");
      mProgressSink->OnProgress(this, nullptr, progress, progressMax);
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
                   const nsresult& channelStatus)
  : mChild(child)
  , mChannelStatus(channelStatus) {}

  void Run() { mChild->OnStopRequest(mChannelStatus); }
 private:
  HttpChannelChild* mChild;
  nsresult mChannelStatus;
};

bool
HttpChannelChild::RecvOnStopRequest(const nsresult& channelStatus)
{
  MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
    "Should not be receiving any more callbacks from parent!");

  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new StopRequestEvent(this, channelStatus));
  } else {
    MOZ_ASSERT(!mDivertingToParent, "ShouldEnqueue when diverting to parent!");

    OnStopRequest(channelStatus);
  }
  return true;
}

void
HttpChannelChild::OnStopRequest(const nsresult& channelStatus)
{
  LOG(("HttpChannelChild::OnStopRequest [this=%p status=%x]\n",
           this, channelStatus));

  if (mDivertingToParent) {
    MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
      "Should not be processing any more callbacks from parent!");

    SendDivertOnStopRequest(channelStatus);
    return;
  }

  mIsPending = false;

  if (!mCanceled && NS_SUCCEEDED(mStatus)) {
    mStatus = channelStatus;
  }

  { 
    
    
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);

    mListener->OnStopRequest(this, mListenerContext, mStatus);

    mListener = 0;
    mListenerContext = 0;
    mCacheEntryAvailable = false;
    if (mLoadGroup)
      mLoadGroup->RemoveRequest(this, nullptr, mStatus);
  }

  if (mLoadFlags & LOAD_DOCUMENT_URI) {
    
    mKeptAlive = true;
    SendDocumentChannelCleanup();
  } else {
    
    
    PHttpChannelChild::Send__delete__(this);
  }
}

class ProgressEvent : public ChannelEvent
{
 public:
  ProgressEvent(HttpChannelChild* child,
                const uint64_t& progress,
                const uint64_t& progressMax)
  : mChild(child)
  , mProgress(progress)
  , mProgressMax(progressMax) {}

  void Run() { mChild->OnProgress(mProgress, mProgressMax); }
 private:
  HttpChannelChild* mChild;
  uint64_t mProgress, mProgressMax;
};

bool
HttpChannelChild::RecvOnProgress(const uint64_t& progress,
                                 const uint64_t& progressMax)
{
  if (mEventQ->ShouldEnqueue())  {
    mEventQ->Enqueue(new ProgressEvent(this, progress, progressMax));
  } else {
    OnProgress(progress, progressMax);
  }
  return true;
}

void
HttpChannelChild::OnProgress(const uint64_t& progress,
                             const uint64_t& progressMax)
{
  LOG(("HttpChannelChild::OnProgress [this=%p progress=%llu/%llu]\n",
       this, progress, progressMax));

  if (mCanceled)
    return;

  
  if (!mProgressSink) {
    GetCallback(mProgressSink);
  }

  AutoEventEnqueuer ensureSerialDispatch(mEventQ);

  
  if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending)
  {
    if (progress > 0) {
      MOZ_ASSERT(progress <= progressMax, "unexpected progress values");
      mProgressSink->OnProgress(this, nullptr, progress, progressMax);
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
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new StatusEvent(this, status));
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

  AutoEventEnqueuer ensureSerialDispatch(mEventQ);

  
  
  if (mProgressSink && NS_SUCCEEDED(mStatus) && mIsPending &&
      !(mLoadFlags & LOAD_BACKGROUND))
  {
    nsAutoCString host;
    mURI->GetHost(host);
    mProgressSink->OnStatus(this, nullptr, status,
                            NS_ConvertUTF8toUTF16(host).get());
  }
}

class FailedAsyncOpenEvent : public ChannelEvent
{
 public:
  FailedAsyncOpenEvent(HttpChannelChild* child, const nsresult& status)
  : mChild(child)
  , mStatus(status) {}

  void Run() { mChild->FailedAsyncOpen(mStatus); }
 private:
  HttpChannelChild* mChild;
  nsresult mStatus;
};

bool
HttpChannelChild::RecvFailedAsyncOpen(const nsresult& status)
{
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new FailedAsyncOpenEvent(this, status));
  } else {
    FailedAsyncOpen(status);
  }
  return true;
}




void
HttpChannelChild::HandleAsyncAbort()
{
  HttpAsyncAborter<HttpChannelChild>::HandleAsyncAbort();
}

void
HttpChannelChild::FailedAsyncOpen(const nsresult& status)
{
  LOG(("HttpChannelChild::FailedAsyncOpen [this=%p status=%x]\n", this, status));

  mStatus = status;

  
  HandleAsyncAbort();
}

void
HttpChannelChild::DoNotifyListenerCleanup()
{
  if (mIPCOpen)
    PHttpChannelChild::Send__delete__(this);
}

class DeleteSelfEvent : public ChannelEvent
{
 public:
  explicit DeleteSelfEvent(HttpChannelChild* child) : mChild(child) {}
  void Run() { mChild->DeleteSelf(); }
 private:
  HttpChannelChild* mChild;
};

bool
HttpChannelChild::RecvDeleteSelf()
{
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new DeleteSelfEvent(this));
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
                 const uint32_t& newChannelId,
                 const URIParams& newURI,
                 const uint32_t& redirectFlags,
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
  uint32_t            mNewChannelId;
  URIParams           mNewURI;
  uint32_t            mRedirectFlags;
  nsHttpResponseHead  mResponseHead;
};

bool
HttpChannelChild::RecvRedirect1Begin(const uint32_t& newChannelId,
                                     const URIParams& newUri,
                                     const uint32_t& redirectFlags,
                                     const nsHttpResponseHead& responseHead)
{
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new Redirect1Event(this, newChannelId, newUri,
                                       redirectFlags, responseHead));
  } else {
    Redirect1Begin(newChannelId, newUri, redirectFlags, responseHead);
  }
  return true;
}

void
HttpChannelChild::Redirect1Begin(const uint32_t& newChannelId,
                                 const URIParams& newUri,
                                 const uint32_t& redirectFlags,
                                 const nsHttpResponseHead& responseHead)
{
  nsresult rv;

  nsCOMPtr<nsIIOService> ioService;
  rv = gHttpHandler->GetIOService(getter_AddRefs(ioService));
  if (NS_FAILED(rv)) {
    
    OnRedirectVerifyCallback(rv);
    return;
  }

  nsCOMPtr<nsIURI> uri = DeserializeURI(newUri);

  nsCOMPtr<nsIChannel> newChannel;
  rv = ioService->NewChannelFromURI(uri, getter_AddRefs(newChannel));
  if (NS_FAILED(rv)) {
    
    OnRedirectVerifyCallback(rv);
    return;
  }

  
  mResponseHead = new nsHttpResponseHead(responseHead);
  SetCookie(mResponseHead->PeekHeader(nsHttp::Set_Cookie));

  bool rewriteToGET = HttpBaseChannel::ShouldRewriteRedirectToGET(mResponseHead->Status(),
                                                                  mRequestHead.ParsedMethod());

  rv = SetupReplacementChannel(uri, newChannel, !rewriteToGET);
  if (NS_FAILED(rv)) {
    
    OnRedirectVerifyCallback(rv);
    return;
  }

  mRedirectChannelChild = do_QueryInterface(newChannel);
  if (mRedirectChannelChild) {
    mRedirectChannelChild->ConnectParent(newChannelId);
    rv = gHttpHandler->AsyncOnChannelRedirect(this,
                                              newChannel,
                                              redirectFlags);
  } else {
    LOG(("  redirecting to a protocol that doesn't implement"
         " nsIChildChannel"));
    rv = NS_ERROR_FAILURE;
  }

  if (NS_FAILED(rv))
    OnRedirectVerifyCallback(rv);
}

class Redirect3Event : public ChannelEvent
{
 public:
  explicit Redirect3Event(HttpChannelChild* child) : mChild(child) {}
  void Run() { mChild->Redirect3Complete(); }
 private:
  HttpChannelChild* mChild;
};

bool
HttpChannelChild::RecvRedirect3Complete()
{
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new Redirect3Event(this));
  } else {
    Redirect3Complete();
  }
  return true;
}

class HttpFlushedForDiversionEvent : public ChannelEvent
{
 public:
  explicit HttpFlushedForDiversionEvent(HttpChannelChild* aChild)
  : mChild(aChild)
  {
    MOZ_RELEASE_ASSERT(aChild);
  }

  void Run()
  {
    mChild->FlushedForDiversion();
  }
 private:
  HttpChannelChild* mChild;
};

bool
HttpChannelChild::RecvFlushedForDiversion()
{
  MOZ_RELEASE_ASSERT(mDivertingToParent);
  MOZ_RELEASE_ASSERT(mEventQ->ShouldEnqueue());

  mEventQ->Enqueue(new HttpFlushedForDiversionEvent(this));

  return true;
}

void
HttpChannelChild::FlushedForDiversion()
{
  MOZ_RELEASE_ASSERT(mDivertingToParent);

  
  
  
  mFlushedForDiversion = true;

  SendDivertComplete();
}

bool
HttpChannelChild::RecvDivertMessages()
{
  MOZ_RELEASE_ASSERT(mDivertingToParent);
  MOZ_RELEASE_ASSERT(mSuspendCount > 0);

  
  
  MOZ_RELEASE_ASSERT(NS_SUCCEEDED(Resume()));

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
    mLoadGroup->RemoveRequest(this, nullptr, NS_BINDING_ABORTED);

  if (NS_FAILED(rv))
    NS_WARNING("CompleteRedirectSetup failed, HttpChannelChild already open?");

  
  mRedirectChannelChild = nullptr;
}





NS_IMETHODIMP
HttpChannelChild::ConnectParent(uint32_t id)
{
  mozilla::dom::TabChild* tabChild = nullptr;
  nsCOMPtr<nsITabChild> iTabChild;
  GetCallback(iTabChild);
  if (iTabChild) {
    tabChild = static_cast<mozilla::dom::TabChild*>(iTabChild.get());
  }
  if (MissingRequiredTabChild(tabChild, "http")) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  
  
  AddIPDLReference();

  HttpChannelConnectArgs connectArgs(id);
  PBrowserOrId browser;
  if (!tabChild ||
      static_cast<ContentChild*>(gNeckoChild->Manager()) == tabChild->Manager()) {
    browser = tabChild;
  } else {
    browser = TabChild::GetTabChildId(tabChild);
  }
  if (!gNeckoChild->
        SendPHttpChannelConstructor(this, browser,
                                    IPC::SerializedLoadContext(this),
                                    connectArgs)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::CompleteRedirectSetup(nsIStreamListener *listener,
                                        nsISupports *aContext)
{
  LOG(("HttpChannelChild::FinishRedirectSetup [this=%p]\n", this));

  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  






  mIsPending = true;
  mWasOpened = true;
  mListener = listener;
  mListenerContext = aContext;

  
  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nullptr);

  
  
  
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::OnRedirectVerifyCallback(nsresult result)
{
  OptionalURIParams redirectURI;
  nsCOMPtr<nsIHttpChannel> newHttpChannel =
      do_QueryInterface(mRedirectChannelChild);

  if (newHttpChannel) {
    
    newHttpChannel->SetOriginalURI(mOriginalURI);
  }

  RequestHeaderTuples emptyHeaders;
  RequestHeaderTuples* headerTuples = &emptyHeaders;

  nsCOMPtr<nsIHttpChannelChild> newHttpChannelChild =
      do_QueryInterface(mRedirectChannelChild);
  if (newHttpChannelChild && NS_SUCCEEDED(result)) {
    newHttpChannelChild->AddCookiesToRequest();
    newHttpChannelChild->GetClientSetRequestHeaders(&headerTuples);
  }

  

  SerializeURI(nullptr, redirectURI);

  if (NS_SUCCEEDED(result)) {
    
    
    
    
    
    

    nsCOMPtr<nsIHttpChannelInternal> newHttpChannelInternal =
      do_QueryInterface(mRedirectChannelChild);
    if (newHttpChannelInternal) {
      nsCOMPtr<nsIURI> apiRedirectURI;
      nsresult rv = newHttpChannelInternal->GetApiRedirectToURI(
        getter_AddRefs(apiRedirectURI));
      if (NS_SUCCEEDED(rv) && apiRedirectURI) {
        

        SerializeURI(apiRedirectURI, redirectURI);
      }
    }
  }

  if (mIPCOpen)
    SendRedirect2Verify(result, *headerTuples, redirectURI);

  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::Cancel(nsresult status)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mCanceled) {
    
    
    mCanceled = true;
    mStatus = status;
    if (RemoteChannelExists())
      SendCancel(status);
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::Suspend()
{
  NS_ENSURE_TRUE(RemoteChannelExists(), NS_ERROR_NOT_AVAILABLE);

  
  
  
  if (!mSuspendCount++ && !mDivertingToParent) {
    SendSuspend();
    mSuspendSent = true;
  }
  mEventQ->Suspend();

  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::Resume()
{
  NS_ENSURE_TRUE(RemoteChannelExists(), NS_ERROR_NOT_AVAILABLE);
  NS_ENSURE_TRUE(mSuspendCount > 0, NS_ERROR_UNEXPECTED);

  nsresult rv = NS_OK;

  
  
  
  
  if (!--mSuspendCount && (!mDivertingToParent || mSuspendSent)) {
    SendResume();
    if (mCallOnResume) {
      AsyncCall(mCallOnResume);
      mCallOnResume = nullptr;
    }
  }
  mEventQ->Resume();

  return rv;
}






void
propagateLoadInfo(nsILoadInfo *aLoadInfo,
                  HttpChannelOpenArgs& openArgs)
{
  mozilla::ipc::PrincipalInfo principalInfo;

  if (aLoadInfo) {
    mozilla::ipc::PrincipalToPrincipalInfo(aLoadInfo->LoadingPrincipal(),
                                           &principalInfo);
    openArgs.requestingPrincipalInfo() = principalInfo;
    openArgs.securityFlags() = aLoadInfo->GetSecurityFlags();
    openArgs.contentPolicyType() = aLoadInfo->GetContentPolicyType();
    return;
  }

  
  mozilla::ipc::PrincipalToPrincipalInfo(nsContentUtils::GetSystemPrincipal(),
                                         &principalInfo);
  openArgs.requestingPrincipalInfo() = principalInfo;
  openArgs.securityFlags() = nsILoadInfo::SEC_NORMAL;
  openArgs.contentPolicyType() = nsIContentPolicy::TYPE_OTHER;
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
  LOG(("HttpChannelChild::AsyncOpen [this=%p uri=%s]\n", this, mSpec.get()));

  if (mCanceled)
    return mStatus;

  NS_ENSURE_TRUE(gNeckoChild != nullptr, NS_ERROR_FAILURE);
  NS_ENSURE_ARG_POINTER(listener);
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  mAsyncOpenTime = TimeStamp::Now();

  
  
  nsresult rv;
  rv = NS_CheckPortSafety(mURI);
  if (NS_FAILED(rv))
    return rv;

  const char *cookieHeader = mRequestHead.PeekHeader(nsHttp::Cookie);
  if (cookieHeader) {
    mUserSetCookieHeader = cookieHeader;
  }

  AddCookiesToRequest();

  
  
  
  

  
  
  
  
  

  mIsPending = true;
  mWasOpened = true;
  mListener = listener;
  mListenerContext = aContext;

  
  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nullptr);

  if (mCanceled) {
    
    
    
    AsyncAbort(mStatus);
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

  
  
  

  mozilla::dom::TabChild* tabChild = nullptr;
  nsCOMPtr<nsITabChild> iTabChild;
  GetCallback(iTabChild);
  if (iTabChild) {
    tabChild = static_cast<mozilla::dom::TabChild*>(iTabChild.get());
  }
  if (MissingRequiredTabChild(tabChild, "http")) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  HttpChannelOpenArgs openArgs;
  
  
  SerializeURI(mURI, openArgs.uri());
  SerializeURI(mOriginalURI, openArgs.original());
  SerializeURI(mDocumentURI, openArgs.doc());
  SerializeURI(mReferrer, openArgs.referrer());
  SerializeURI(mAPIRedirectToURI, openArgs.apiRedirectTo());
  openArgs.loadFlags() = mLoadFlags;
  openArgs.requestHeaders() = mClientSetRequestHeaders;
  openArgs.requestMethod() = mRequestHead.Method();

  nsTArray<mozilla::ipc::FileDescriptor> fds;
  SerializeInputStream(mUploadStream, openArgs.uploadStream(), fds);

  PFileDescriptorSetChild* fdSet = nullptr;
  if (!fds.IsEmpty()) {
    MOZ_ASSERT(gNeckoChild->Manager());

    fdSet = gNeckoChild->Manager()->SendPFileDescriptorSetConstructor(fds[0]);
    for (uint32_t i = 1; i < fds.Length(); ++i) {
      unused << fdSet->SendAddFileDescriptor(fds[i]);
    }
  }

  OptionalFileDescriptorSet optionalFDs;
  if (fdSet) {
    optionalFDs = fdSet;
  } else {
    optionalFDs = mozilla::void_t();
  }

  openArgs.fds() = optionalFDs;

  openArgs.uploadStreamHasHeaders() = mUploadStreamHasHeaders;
  openArgs.priority() = mPriority;
  openArgs.redirectionLimit() = mRedirectionLimit;
  openArgs.allowPipelining() = mAllowPipelining;
  openArgs.allowSTS() = mAllowSTS;
  openArgs.forceAllowThirdPartyCookie() = mForceAllowThirdPartyCookie;
  openArgs.resumeAt() = mSendResumeAt;
  openArgs.startPos() = mStartPos;
  openArgs.entityID() = mEntityID;
  openArgs.chooseApplicationCache() = mChooseApplicationCache;
  openArgs.appCacheClientID() = appCacheClientId;
  openArgs.allowSpdy() = mAllowSpdy;

  propagateLoadInfo(mLoadInfo, openArgs);

  
  
  AddIPDLReference();

  PBrowserOrId browser;
  if (!tabChild ||
      static_cast<ContentChild*>(gNeckoChild->Manager()) == tabChild->Manager()) {
    browser = tabChild;
  } else {
    browser = TabChild::GetTabChildId(tabChild);
  }
  gNeckoChild->SendPHttpChannelConstructor(this, browser,
                                           IPC::SerializedLoadContext(this),
                                           openArgs);

  if (fdSet) {
    FileDescriptorSetChild* fdSetActor =
      static_cast<FileDescriptorSetChild*>(fdSet);

    fdSetActor->ForgetFileDescriptors(fds);
  }

  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::SetRequestHeader(const nsACString& aHeader,
                                   const nsACString& aValue,
                                   bool aMerge)
{
  nsresult rv = HttpBaseChannel::SetRequestHeader(aHeader, aValue, aMerge);
  if (NS_FAILED(rv))
    return rv;

  RequestHeaderTuple* tuple = mClientSetRequestHeaders.AppendElement();
  if (!tuple)
    return NS_ERROR_OUT_OF_MEMORY;

  tuple->mHeader = aHeader;
  tuple->mValue = aValue;
  tuple->mMerge = aMerge;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::RedirectTo(nsIURI *newURI)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
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
HttpChannelChild::GetRemotePort(int32_t * _result)
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
HttpChannelChild::GetLocalPort(int32_t * _result)
{
  NS_ENSURE_ARG_POINTER(_result);
  return NS_ERROR_NOT_AVAILABLE;
}






NS_IMETHODIMP
HttpChannelChild::GetCacheTokenExpirationTime(uint32_t *_retval)
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
  if (!mCacheEntryAvailable || !RemoteChannelExists())
    return NS_ERROR_NOT_AVAILABLE;

  mCachedCharset = aCharset;
  if (!SendSetCacheTokenCachedCharset(PromiseFlatCString(aCharset))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::IsFromCache(bool *value)
{
  if (!mIsPending)
    return NS_ERROR_NOT_AVAILABLE;

  *value = mIsFromCache;
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::ResumeAt(uint64_t startPos, const nsACString& entityID)
{
  ENSURE_CALLED_BEFORE_CONNECT();
  mStartPos = startPos;
  mEntityID = entityID;
  mSendResumeAt = true;
  return NS_OK;
}







NS_IMETHODIMP
HttpChannelChild::SetPriority(int32_t aPriority)
{
  int16_t newValue = clamped<int32_t>(aPriority, INT16_MIN, INT16_MAX);
  if (mPriority == newValue)
    return NS_OK;
  mPriority = newValue;
  if (RemoteChannelExists())
    SendSetPriority(mPriority);
  return NS_OK;
}





NS_IMETHODIMP
HttpChannelChild::GetProxyInfo(nsIProxyInfo **aProxyInfo)
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
HttpChannelChild::GetApplicationCacheForWrite(nsIApplicationCache **aApplicationCache)
{
  *aApplicationCache = nullptr;
  return NS_OK;
}
NS_IMETHODIMP
HttpChannelChild::SetApplicationCacheForWrite(nsIApplicationCache *aApplicationCache)
{
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
HttpChannelChild::GetLoadedFromApplicationCache(bool *aLoadedFromApplicationCache)
{
  *aLoadedFromApplicationCache = mLoadedFromApplicationCache;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetInheritApplicationCache(bool *aInherit)
{
  *aInherit = mInheritApplicationCache;
  return NS_OK;
}
NS_IMETHODIMP
HttpChannelChild::SetInheritApplicationCache(bool aInherit)
{
  mInheritApplicationCache = aInherit;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::GetChooseApplicationCache(bool *aChoose)
{
  *aChoose = mChooseApplicationCache;
  return NS_OK;
}

NS_IMETHODIMP
HttpChannelChild::SetChooseApplicationCache(bool aChoose)
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
HttpChannelChild::GetCountSubRequestsBrokenSecurity(
                    int32_t *aSubRequestsBrokenSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->GetCountSubRequestsBrokenSecurity(aSubRequestsBrokenSecurity);
}
NS_IMETHODIMP
HttpChannelChild::SetCountSubRequestsBrokenSecurity(
                    int32_t aSubRequestsBrokenSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->SetCountSubRequestsBrokenSecurity(aSubRequestsBrokenSecurity);
}


NS_IMETHODIMP
HttpChannelChild::GetCountSubRequestsNoSecurity(int32_t *aSubRequestsNoSecurity)
{
  nsCOMPtr<nsIAssociatedContentSecurity> assoc;
  if (!GetAssociatedContentSecurity(getter_AddRefs(assoc)))
    return NS_OK;

  return assoc->GetCountSubRequestsNoSecurity(aSubRequestsNoSecurity);
}
NS_IMETHODIMP
HttpChannelChild::SetCountSubRequestsNoSecurity(int32_t aSubRequestsNoSecurity)
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
  int32_t broken, no;

  rv = assoc->GetCountSubRequestsBrokenSecurity(&broken);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = assoc->GetCountSubRequestsNoSecurity(&no);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mIPCOpen)
    SendUpdateAssociatedContentSecurity(broken, no);

  return NS_OK;
}





NS_IMETHODIMP HttpChannelChild::AddCookiesToRequest()
{
  HttpBaseChannel::AddCookiesToRequest();
  return NS_OK;
}

NS_IMETHODIMP HttpChannelChild::GetClientSetRequestHeaders(RequestHeaderTuples **aRequestHeaders)
{
  *aRequestHeaders = &mClientSetRequestHeaders;
  return NS_OK;
}




NS_IMETHODIMP
HttpChannelChild::DivertToParent(ChannelDiverterChild **aChild)
{
  MOZ_RELEASE_ASSERT(aChild);
  MOZ_RELEASE_ASSERT(gNeckoChild);
  MOZ_RELEASE_ASSERT(!mDivertingToParent);

  
  
  if (NS_FAILED(mStatus) && !RemoteChannelExists()) {
    return mStatus;
  }

  nsresult rv = Suspend();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  
  mDivertingToParent = true;

  PChannelDiverterChild* diverter =
    gNeckoChild->SendPChannelDiverterConstructor(this);
  MOZ_RELEASE_ASSERT(diverter);

  *aChild = static_cast<ChannelDiverterChild*>(diverter);

  return NS_OK;
}

}} 
