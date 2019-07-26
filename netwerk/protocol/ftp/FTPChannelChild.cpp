






#include "mozilla/net/NeckoChild.h"
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
, mWasOpened(false)
, mLastModifiedTime(0)
, mStartPos(0)
{
  LOG(("Creating FTPChannelChild @%x\n", this));
  
  NS_ADDREF(gFtpHandler);
  SetURI(uri);
  mEventQ = new ChannelEventQueue(static_cast<nsIFTPChannel*>(this));
}

FTPChannelChild::~FTPChannelChild()
{
  LOG(("Destroying FTPChannelChild @%x\n", this));
  gFtpHandler->Release();
}

void
FTPChannelChild::AddIPDLReference()
{
  NS_ABORT_IF_FALSE(!mIPCOpen, "Attempt to retain more than one IPDL reference");
  mIPCOpen = true;
  AddRef();
}

void
FTPChannelChild::ReleaseIPDLReference()
{
  NS_ABORT_IF_FALSE(mIPCOpen, "Attempt to release nonexistent IPDL reference");
  mIPCOpen = false;
  Release();
}





NS_IMPL_ISUPPORTS_INHERITED5(FTPChannelChild,
                             nsBaseChannel,
                             nsIFTPChannel,
                             nsIUploadChannel,
                             nsIResumableChannel,
                             nsIProxiedChannel,
                             nsIChildChannel)



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
  SerializeInputStream(mUploadStream, uploadStream);

  FTPChannelOpenArgs openArgs;
  SerializeURI(nsBaseChannel::URI(), openArgs.uri());
  openArgs.startPos() = mStartPos;
  openArgs.entityID() = mEntityID;
  openArgs.uploadStream() = uploadStream;

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
  FTPStartRequestEvent(FTPChannelChild* aChild, const int64_t& aContentLength,
                       const nsCString& aContentType, const PRTime& aLastModified,
                       const nsCString& aEntityID, const URIParams& aURI)
  : mChild(aChild), mContentLength(aContentLength), mContentType(aContentType),
    mLastModified(aLastModified), mEntityID(aEntityID), mURI(aURI) {}
  void Run() { mChild->DoOnStartRequest(mContentLength, mContentType,
                                       mLastModified, mEntityID, mURI); }
 private:
  FTPChannelChild* mChild;
  int64_t mContentLength;
  nsCString mContentType;
  PRTime mLastModified;
  nsCString mEntityID;
  URIParams mURI;
};

bool
FTPChannelChild::RecvOnStartRequest(const int64_t& aContentLength,
                                    const nsCString& aContentType,
                                    const PRTime& aLastModified,
                                    const nsCString& aEntityID,
                                    const URIParams& aURI)
{
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new FTPStartRequestEvent(this, aContentLength, aContentType,
                                              aLastModified, aEntityID, aURI));
  } else {
    DoOnStartRequest(aContentLength, aContentType, aLastModified,
                     aEntityID, aURI);
  }
  return true;
}

void
FTPChannelChild::DoOnStartRequest(const int64_t& aContentLength,
                                  const nsCString& aContentType,
                                  const PRTime& aLastModified,
                                  const nsCString& aEntityID,
                                  const URIParams& aURI)
{
  LOG(("FTPChannelChild::RecvOnStartRequest [this=%p]\n", this));

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
}

class FTPDataAvailableEvent : public ChannelEvent
{
 public:
  FTPDataAvailableEvent(FTPChannelChild* aChild, const nsCString& aData,
                        const uint64_t& aOffset, const uint32_t& aCount)
  : mChild(aChild), mData(aData), mOffset(aOffset), mCount(aCount) {}
  void Run() { mChild->DoOnDataAvailable(mData, mOffset, mCount); }
 private:
  FTPChannelChild* mChild;
  nsCString mData;
  uint64_t mOffset;
  uint32_t mCount;
};

bool
FTPChannelChild::RecvOnDataAvailable(const nsCString& data,
                                     const uint64_t& offset,
                                     const uint32_t& count)
{
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new FTPDataAvailableEvent(this, data, offset, count));
  } else {
    DoOnDataAvailable(data, offset, count);
  }
  return true;
}

void
FTPChannelChild::DoOnDataAvailable(const nsCString& data,
                                   const uint64_t& offset,
                                   const uint32_t& count)
{
  LOG(("FTPChannelChild::RecvOnDataAvailable [this=%p]\n", this));

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
  FTPStopRequestEvent(FTPChannelChild* aChild, const nsresult& aStatusCode)
  : mChild(aChild), mStatusCode(aStatusCode) {}
  void Run() { mChild->DoOnStopRequest(mStatusCode); }
 private:
  FTPChannelChild* mChild;
  nsresult mStatusCode;
};

bool
FTPChannelChild::RecvOnStopRequest(const nsresult& statusCode)
{
  if (mEventQ->ShouldEnqueue()) {
    mEventQ->Enqueue(new FTPStopRequestEvent(this, statusCode));
  } else {
    DoOnStopRequest(statusCode);
  }
  return true;
}

void
FTPChannelChild::DoOnStopRequest(const nsresult& statusCode)
{
  LOG(("FTPChannelChild::RecvOnStopRequest [this=%p status=%u]\n",
           this, statusCode));

  if (!mCanceled)
    mStatus = statusCode;

  { 
    
    mIsPending = false;
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);
    (void)mListener->OnStopRequest(this, mListenerContext, statusCode);
    mListener = nullptr;
    mListenerContext = nullptr;

    if (mLoadGroup)
      mLoadGroup->RemoveRequest(this, nullptr, statusCode);
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

bool
FTPChannelChild::RecvFlushedForDiversion()
{
  return false;
}

bool
FTPChannelChild::RecvDivertMessages()
{
  return false;
}

class FTPDeleteSelfEvent : public ChannelEvent
{
 public:
  FTPDeleteSelfEvent(FTPChannelChild* aChild)
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
  if (!mSuspendCount++) {
    SendSuspend();
  }
  mEventQ->Suspend();

  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::Resume()
{
  NS_ENSURE_TRUE(mIPCOpen, NS_ERROR_NOT_AVAILABLE);

  if (!--mSuspendCount) {
    SendResume();
  }
  mEventQ->Resume();

  return NS_OK;
}





NS_IMETHODIMP
FTPChannelChild::ConnectParent(uint32_t id)
{
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

} 
} 

