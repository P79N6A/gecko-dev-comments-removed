






#include "mozilla/net/NeckoChild.h"
#include "mozilla/net/ChannelDiverterChild.h"
#include "mozilla/net/FTPChannelChild.h"
#include "mozilla/dom/TabChild.h"
#include "nsFtpProtocolHandler.h"
#include "nsITabChild.h"
#include "nsStringStream.h"
#include "nsNetUtil.h"
#include "base/compiler_specific.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/URIUtils.h"
#include "SerializedLoadContext.h"
#include "mozilla/ipc/BackgroundUtils.h"

using namespace mozilla::ipc;

#undef LOG
#define LOG(args) PR_LOG(gFTPLog, PR_LOG_DEBUG, args)

namespace mozilla {
namespace net {

FTPChannelChild::FTPChannelChild(nsIURI* uri)
: mIPCOpen(false)
, mCanceled(false)
, mSuspendCount(0)
, mIsPending(false)
, mLastModifiedTime(0)
, mStartPos(0)
, mDivertingToParent(false)
, mFlushedForDiversion(false)
, mSuspendSent(false)
{
  LOG(("Creating FTPChannelChild @%x\n", this));
  
  NS_ADDREF(gFtpHandler);
  SetURI(uri);
  mEventQ = new ChannelEventQueue(static_cast<nsIFTPChannel*>(this));

  
  
  DisallowThreadRetargeting();
}

FTPChannelChild::~FTPChannelChild()
{
  LOG(("Destroying FTPChannelChild @%x\n", this));
  gFtpHandler->Release();
}

void
FTPChannelChild::AddIPDLReference()
{
  MOZ_ASSERT(!mIPCOpen, "Attempt to retain more than one IPDL reference");
  mIPCOpen = true;
  AddRef();
}

void
FTPChannelChild::ReleaseIPDLReference()
{
  MOZ_ASSERT(mIPCOpen, "Attempt to release nonexistent IPDL reference");
  mIPCOpen = false;
  Release();
}





NS_IMPL_ISUPPORTS_INHERITED(FTPChannelChild,
                            nsBaseChannel,
                            nsIFTPChannel,
                            nsIUploadChannel,
                            nsIResumableChannel,
                            nsIProxiedChannel,
                            nsIChildChannel,
                            nsIDivertableChannel)



NS_IMETHODIMP
FTPChannelChild::GetLastModifiedTime(PRTime* lastModifiedTime)
{
  *lastModifiedTime = mLastModifiedTime;
  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::SetLastModifiedTime(PRTime lastModifiedTime)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
FTPChannelChild::ResumeAt(uint64_t aStartPos, const nsACString& aEntityID)
{
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  mStartPos = aStartPos;
  mEntityID = aEntityID;
  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::GetEntityID(nsACString& entityID)
{
  entityID = mEntityID;
  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::GetProxyInfo(nsIProxyInfo** aProxyInfo)
{
  DROP_DEAD();
}

NS_IMETHODIMP
FTPChannelChild::SetUploadStream(nsIInputStream* stream,
                                 const nsACString& contentType,
                                 int64_t contentLength)
{
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  mUploadStream = stream;
  
  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::GetUploadStream(nsIInputStream** stream)
{
  NS_ENSURE_ARG_POINTER(stream);
  *stream = mUploadStream;
  NS_IF_ADDREF(*stream);
  return NS_OK;
}




void
propagateLoadInfo(nsILoadInfo *aLoadInfo,
                  FTPChannelOpenArgs& openArgs)
{
  mozilla::ipc::PrincipalInfo requestingPrincipalInfo;
  mozilla::ipc::PrincipalInfo triggeringPrincipalInfo;

  if (aLoadInfo) {
    mozilla::ipc::PrincipalToPrincipalInfo(aLoadInfo->LoadingPrincipal(),
                                           &requestingPrincipalInfo);
    openArgs.requestingPrincipalInfo() = requestingPrincipalInfo;

    mozilla::ipc::PrincipalToPrincipalInfo(aLoadInfo->TriggeringPrincipal(),
                                           &triggeringPrincipalInfo);
    openArgs.triggeringPrincipalInfo() = triggeringPrincipalInfo;

    openArgs.securityFlags() = aLoadInfo->GetSecurityFlags();
    openArgs.contentPolicyType() = aLoadInfo->GetContentPolicyType();
    openArgs.innerWindowID() = aLoadInfo->GetInnerWindowID();
    return;
  }

  
  mozilla::ipc::PrincipalToPrincipalInfo(nsContentUtils::GetSystemPrincipal(),
                                         &requestingPrincipalInfo);
  openArgs.requestingPrincipalInfo() = requestingPrincipalInfo;
  openArgs.triggeringPrincipalInfo() = requestingPrincipalInfo;
  openArgs.securityFlags() = nsILoadInfo::SEC_NORMAL;
  openArgs.contentPolicyType() = nsIContentPolicy::TYPE_OTHER;
  openArgs.innerWindowID() = 0;
}

NS_IMETHODIMP
FTPChannelChild::AsyncOpen(::nsIStreamListener* listener, nsISupports* aContext)
{
  LOG(("FTPChannelChild::AsyncOpen [this=%p]\n", this));

  NS_ENSURE_TRUE((gNeckoChild), NS_ERROR_FAILURE);
  NS_ENSURE_ARG_POINTER(listener);
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  
  
  nsresult rv;
  rv = NS_CheckPortSafety(nsBaseChannel::URI()); 
                                                 
                                                 
  if (NS_FAILED(rv))
    return rv;

  mozilla::dom::TabChild* tabChild = nullptr;
  nsCOMPtr<nsITabChild> iTabChild;
  NS_QueryNotificationCallbacks(mCallbacks, mLoadGroup,
                                NS_GET_IID(nsITabChild),
                                getter_AddRefs(iTabChild));
  GetCallback(iTabChild);
  if (iTabChild) {
    tabChild = static_cast<mozilla::dom::TabChild*>(iTabChild.get());
  }
  if (MissingRequiredTabChild(tabChild, "ftp")) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  mListener = listener;
  mListenerContext = aContext;

  
  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nullptr);

  OptionalInputStreamParams uploadStream;
  nsTArray<mozilla::ipc::FileDescriptor> fds;
  SerializeInputStream(mUploadStream, uploadStream, fds);

  MOZ_ASSERT(fds.IsEmpty());

  FTPChannelOpenArgs openArgs;
  SerializeURI(nsBaseChannel::URI(), openArgs.uri());
  openArgs.startPos() = mStartPos;
  openArgs.entityID() = mEntityID;
  openArgs.uploadStream() = uploadStream;

  nsCOMPtr<nsILoadInfo> loadInfo;
  GetLoadInfo(getter_AddRefs(loadInfo));
  propagateLoadInfo(loadInfo, openArgs);

  gNeckoChild->
    SendPFTPChannelConstructor(this, tabChild, IPC::SerializedLoadContext(this),
                               openArgs);

  
  
  AddIPDLReference();

  mIsPending = true;
  mWasOpened = true;

  return rv;
}

NS_IMETHODIMP
FTPChannelChild::IsPending(bool* result)
{
  *result = mIsPending;
  return NS_OK;
}

nsresult
FTPChannelChild::OpenContentStream(bool async,
                                   nsIInputStream** stream,
                                   nsIChannel** channel)
{
  NS_RUNTIMEABORT("FTPChannel*Child* should never have OpenContentStream called!");
  return NS_OK;
}
  




class FTPStartRequestEvent : public ChannelEvent
{
public:
  FTPStartRequestEvent(FTPChannelChild* aChild,
                       const nsresult& aChannelStatus,
                       const int64_t& aContentLength,
                       const nsCString& aContentType,
                       const PRTime& aLastModified,
                       const nsCString& aEntityID,
                       const URIParams& aURI)
    : mChild(aChild)
    , mChannelStatus(aChannelStatus)
    , mContentLength(aContentLength)
    , mContentType(aContentType)
    , mLastModified(aLastModified)
    , mEntityID(aEntityID)
    , mURI(aURI)
  {
  }
  void Run()
  {
    mChild->DoOnStartRequest(mChannelStatus, mContentLength, mContentType,
                             mLastModified, mEntityID, mURI);
  }

private:
  FTPChannelChild* mChild;
  nsresult mChannelStatus;
  int64_t mContentLength;
  nsCString mContentType;
  PRTime mLastModified;
  nsCString mEntityID;
  URIParams mURI;
};

bool
FTPChannelChild::RecvOnStartRequest(const nsresult& aChannelStatus,
                                    const int64_t& aContentLength,
                                    const nsCString& aContentType,
                                    const PRTime& aLastModified,
                                    const nsCString& aEntityID,
                                    const URIParams& aURI)
{
  
  
  MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
    "mFlushedForDiversion should be unset before OnStartRequest!");
  MOZ_RELEASE_ASSERT(!mDivertingToParent,
    "mDivertingToParent should be unset before OnStartRequest!");

  LOG(("FTPChannelChild::RecvOnStartRequest [this=%p]\n", this));

  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new FTPStartRequestEvent(this, aChannelStatus,
                                              aContentLength, aContentType,
                                              aLastModified, aEntityID, aURI));
  } else {
    DoOnStartRequest(aChannelStatus, aContentLength, aContentType,
                     aLastModified, aEntityID, aURI);
  }
  return true;
}

void
FTPChannelChild::DoOnStartRequest(const nsresult& aChannelStatus,
                                  const int64_t& aContentLength,
                                  const nsCString& aContentType,
                                  const PRTime& aLastModified,
                                  const nsCString& aEntityID,
                                  const URIParams& aURI)
{
  LOG(("FTPChannelChild::DoOnStartRequest [this=%p]\n", this));

  
  
  MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
    "mFlushedForDiversion should be unset before OnStartRequest!");
  MOZ_RELEASE_ASSERT(!mDivertingToParent,
    "mDivertingToParent should be unset before OnStartRequest!");

  if (!mCanceled && NS_SUCCEEDED(mStatus)) {
    mStatus = aChannelStatus;
  }

  mContentLength = aContentLength;
  SetContentType(aContentType);
  mLastModifiedTime = aLastModified;
  mEntityID = aEntityID;

  nsCString spec;
  nsCOMPtr<nsIURI> uri = DeserializeURI(aURI);
  uri->GetSpec(spec);
  nsBaseChannel::URI()->SetSpec(spec);

  AutoEventEnqueuer ensureSerialDispatch(mEventQ);
  nsresult rv = mListener->OnStartRequest(this, mListenerContext);
  if (NS_FAILED(rv))
    Cancel(rv);

  if (mDivertingToParent) {
    mListener = nullptr;
    mListenerContext = nullptr;
    if (mLoadGroup) {
      mLoadGroup->RemoveRequest(this, nullptr, mStatus);
    }
  }
}

class FTPDataAvailableEvent : public ChannelEvent
{
public:
  FTPDataAvailableEvent(FTPChannelChild* aChild,
                        const nsresult& aChannelStatus,
                        const nsCString& aData,
                        const uint64_t& aOffset,
                        const uint32_t& aCount)
    : mChild(aChild)
    , mChannelStatus(aChannelStatus)
    , mData(aData)
    , mOffset(aOffset)
    , mCount(aCount)
  {
  }
  void Run()
  {
    mChild->DoOnDataAvailable(mChannelStatus, mData, mOffset, mCount);
  }

private:
  FTPChannelChild* mChild;
  nsresult mChannelStatus;
  nsCString mData;
  uint64_t mOffset;
  uint32_t mCount;
};

bool
FTPChannelChild::RecvOnDataAvailable(const nsresult& channelStatus,
                                     const nsCString& data,
                                     const uint64_t& offset,
                                     const uint32_t& count)
{
  MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
                     "Should not be receiving any more callbacks from parent!");

  LOG(("FTPChannelChild::RecvOnDataAvailable [this=%p]\n", this));

  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(
      new FTPDataAvailableEvent(this, channelStatus, data, offset, count));
  } else {
    MOZ_RELEASE_ASSERT(!mDivertingToParent,
                       "ShouldEnqueue when diverting to parent!");

    DoOnDataAvailable(channelStatus, data, offset, count);
  }
  return true;
}

void
FTPChannelChild::DoOnDataAvailable(const nsresult& channelStatus,
                                   const nsCString& data,
                                   const uint64_t& offset,
                                   const uint32_t& count)
{
  LOG(("FTPChannelChild::DoOnDataAvailable [this=%p]\n", this));

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

  
  
  
  
  
  nsCOMPtr<nsIInputStream> stringStream;
  nsresult rv = NS_NewByteInputStream(getter_AddRefs(stringStream),
                                      data.get(),
                                      count,
                                      NS_ASSIGNMENT_DEPEND);
  if (NS_FAILED(rv)) {
    Cancel(rv);
    return;
  }

  AutoEventEnqueuer ensureSerialDispatch(mEventQ);
  rv = mListener->OnDataAvailable(this, mListenerContext,
                                  stringStream, offset, count);
  if (NS_FAILED(rv))
    Cancel(rv);
  stringStream->Close();
}

class FTPStopRequestEvent : public ChannelEvent
{
public:
  FTPStopRequestEvent(FTPChannelChild* aChild,
                      const nsresult& aChannelStatus)
    : mChild(aChild)
    , mChannelStatus(aChannelStatus)
  {
  }
  void Run()
  {
    mChild->DoOnStopRequest(mChannelStatus);
  }

private:
  FTPChannelChild* mChild;
  nsresult mChannelStatus;
};

bool
FTPChannelChild::RecvOnStopRequest(const nsresult& aChannelStatus)
{
  MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
    "Should not be receiving any more callbacks from parent!");

  LOG(("FTPChannelChild::RecvOnStopRequest [this=%p status=%x]\n",
       this, aChannelStatus));

  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new FTPStopRequestEvent(this, aChannelStatus));
  } else {
    DoOnStopRequest(aChannelStatus);
  }
  return true;
}

void
FTPChannelChild::DoOnStopRequest(const nsresult& aChannelStatus)
{
  LOG(("FTPChannelChild::DoOnStopRequest [this=%p status=%x]\n",
       this, aChannelStatus));

  if (mDivertingToParent) {
    MOZ_RELEASE_ASSERT(!mFlushedForDiversion,
      "Should not be processing any more callbacks from parent!");

    SendDivertOnStopRequest(aChannelStatus);
    return;
  }

  if (!mCanceled)
    mStatus = aChannelStatus;

  { 
    
    mIsPending = false;
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);
    (void)mListener->OnStopRequest(this, mListenerContext, aChannelStatus);
    mListener = nullptr;
    mListenerContext = nullptr;

    if (mLoadGroup)
      mLoadGroup->RemoveRequest(this, nullptr, aChannelStatus);
  }

  
  
  Send__delete__(this);
}

class FTPFailedAsyncOpenEvent : public ChannelEvent
{
 public:
  FTPFailedAsyncOpenEvent(FTPChannelChild* aChild, nsresult aStatus)
  : mChild(aChild), mStatus(aStatus) {}
  void Run() { mChild->DoFailedAsyncOpen(mStatus); }
 private:
  FTPChannelChild* mChild;
  nsresult mStatus;
};

bool
FTPChannelChild::RecvFailedAsyncOpen(const nsresult& statusCode)
{
  LOG(("FTPChannelChild::RecvFailedAsyncOpen [this=%p status=%x]\n",
       this, statusCode));
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new FTPFailedAsyncOpenEvent(this, statusCode));
  } else {
    DoFailedAsyncOpen(statusCode);
  }
  return true;
}

void
FTPChannelChild::DoFailedAsyncOpen(const nsresult& statusCode)
{
  LOG(("FTPChannelChild::DoFailedAsyncOpen [this=%p status=%x]\n",
       this, statusCode));
  mStatus = statusCode;

  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nullptr, statusCode);

  if (mListener) {
    mListener->OnStartRequest(this, mListenerContext);
    mIsPending = false;
    mListener->OnStopRequest(this, mListenerContext, statusCode);
  } else {
    mIsPending = false;
  }

  mListener = nullptr;
  mListenerContext = nullptr;

  if (mIPCOpen)
    Send__delete__(this);
}

class FTPFlushedForDiversionEvent : public ChannelEvent
{
 public:
  explicit FTPFlushedForDiversionEvent(FTPChannelChild* aChild)
  : mChild(aChild)
  {
    MOZ_RELEASE_ASSERT(aChild);
  }

  void Run()
  {
    mChild->FlushedForDiversion();
  }
 private:
  FTPChannelChild* mChild;
};

bool
FTPChannelChild::RecvFlushedForDiversion()
{
  LOG(("FTPChannelChild::RecvFlushedForDiversion [this=%p]\n", this));
  MOZ_ASSERT(mDivertingToParent);

  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new FTPFlushedForDiversionEvent(this));
  } else {
    MOZ_CRASH();
  }
  return true;
}

void
FTPChannelChild::FlushedForDiversion()
{
  LOG(("FTPChannelChild::FlushedForDiversion [this=%p]\n", this));
  MOZ_RELEASE_ASSERT(mDivertingToParent);

  
  
  
  mFlushedForDiversion = true;

  SendDivertComplete();
}

bool
FTPChannelChild::RecvDivertMessages()
{
  LOG(("FTPChannelChild::RecvDivertMessages [this=%p]\n", this));
  MOZ_RELEASE_ASSERT(mDivertingToParent);
  MOZ_RELEASE_ASSERT(mSuspendCount > 0);

  
  
  if (NS_WARN_IF(NS_FAILED(Resume()))) {
    return false;
  }
  return true;
}

class FTPDeleteSelfEvent : public ChannelEvent
{
 public:
  explicit FTPDeleteSelfEvent(FTPChannelChild* aChild)
  : mChild(aChild) {}
  void Run() { mChild->DoDeleteSelf(); }
 private:
  FTPChannelChild* mChild;
};

bool
FTPChannelChild::RecvDeleteSelf()
{
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new FTPDeleteSelfEvent(this));
  } else {
    DoDeleteSelf();
  }
  return true;
}

void
FTPChannelChild::DoDeleteSelf()
{
  if (mIPCOpen)
    Send__delete__(this);
}

NS_IMETHODIMP
FTPChannelChild::Cancel(nsresult status)
{
  LOG(("FTPChannelChild::Cancel [this=%p]\n", this));
  if (mCanceled)
    return NS_OK;

  mCanceled = true;
  mStatus = status;
  if (mIPCOpen)
    SendCancel(status);
  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::Suspend()
{
  NS_ENSURE_TRUE(mIPCOpen, NS_ERROR_NOT_AVAILABLE);

  LOG(("FTPChannelChild::Suspend [this=%p]\n", this));

  
  
  
  if (!mSuspendCount++ && !mDivertingToParent) {
    SendSuspend();
    mSuspendSent = true;
  }
  mEventQ->Suspend();

  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::Resume()
{
  NS_ENSURE_TRUE(mIPCOpen, NS_ERROR_NOT_AVAILABLE);

  LOG(("FTPChannelChild::Resume [this=%p]\n", this));

  
  
  
  
  if (!--mSuspendCount && (!mDivertingToParent || mSuspendSent)) {
    SendResume();
  }
  mEventQ->Resume();

  return NS_OK;
}





NS_IMETHODIMP
FTPChannelChild::ConnectParent(uint32_t id)
{
  LOG(("FTPChannelChild::ConnectParent [this=%p]\n", this));

  mozilla::dom::TabChild* tabChild = nullptr;
  nsCOMPtr<nsITabChild> iTabChild;
  NS_QueryNotificationCallbacks(mCallbacks, mLoadGroup,
                                NS_GET_IID(nsITabChild),
                                getter_AddRefs(iTabChild));
  GetCallback(iTabChild);
  if (iTabChild) {
    tabChild = static_cast<mozilla::dom::TabChild*>(iTabChild.get());
  }

  
  
  AddIPDLReference();

  FTPChannelConnectArgs connectArgs(id);

  if (!gNeckoChild->SendPFTPChannelConstructor(this, tabChild,
                                               IPC::SerializedLoadContext(this),
                                               connectArgs)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::CompleteRedirectSetup(nsIStreamListener *listener,
                                       nsISupports *aContext)
{
  LOG(("FTPChannelChild::CompleteRedirectSetup [this=%p]\n", this));

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
FTPChannelChild::DivertToParent(ChannelDiverterChild **aChild)
{
  MOZ_RELEASE_ASSERT(aChild);
  MOZ_RELEASE_ASSERT(gNeckoChild);
  MOZ_RELEASE_ASSERT(!mDivertingToParent);

  LOG(("FTPChannelChild::DivertToParent [this=%p]\n", this));

  
  
  if (NS_FAILED(mStatus) && !mIPCOpen) {
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

} 
} 

