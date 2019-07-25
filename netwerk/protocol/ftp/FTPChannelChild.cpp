








































#include "mozilla/net/NeckoChild.h"
#include "mozilla/net/FTPChannelChild.h"
#include "nsFtpProtocolHandler.h"

#include "nsStringStream.h"
#include "nsMimeTypes.h"
#include "nsNetUtil.h"
#include "nsIURIFixup.h"
#include "nsCDefaultURIFixup.h"

#undef LOG
#define LOG(args) PR_LOG(gFTPLog, PR_LOG_DEBUG, args)

namespace mozilla {
namespace net {

FTPChannelChild::FTPChannelChild(nsIURI* uri)
: mIPCOpen(false)
, mEventQ(this)
, mCanceled(false)
, mSuspendCount(0)
, mIsPending(PR_FALSE)
, mWasOpened(PR_FALSE)
, mLastModifiedTime(0)
, mStartPos(0)
{
  LOG(("Creating FTPChannelChild @%x\n", this));
  
  NS_ADDREF(gFtpHandler);
  SetURI(uri);
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
FTPChannelChild::ResumeAt(PRUint64 aStartPos, const nsACString& aEntityID)
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
                                 PRInt32 contentLength)
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
  LOG(("FTPChannelChild::AsyncOpen [this=%x]\n", this));

  NS_ENSURE_TRUE((gNeckoChild), NS_ERROR_FAILURE);
  NS_ENSURE_ARG_POINTER(listener);
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

  
  
  nsresult rv;
  rv = NS_CheckPortSafety(nsBaseChannel::URI()); 
                                                 
                                                 
  if (NS_FAILED(rv))
    return rv;

  
  gNeckoChild->SendPFTPChannelConstructor(this);
  mListener = listener;
  mListenerContext = aContext;

  
  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nsnull);

  SendAsyncOpen(nsBaseChannel::URI(), mStartPos, mEntityID,
                IPC::InputStream(mUploadStream));

  
  
  AddIPDLReference();

  mIsPending = PR_TRUE;
  mWasOpened = PR_TRUE;

  return rv;
}

NS_IMETHODIMP
FTPChannelChild::IsPending(PRBool* result)
{
  *result = mIsPending;
  return NS_OK;
}

nsresult
FTPChannelChild::OpenContentStream(PRBool async,
                                   nsIInputStream** stream,
                                   nsIChannel** channel)
{
  NS_RUNTIMEABORT("FTPChannel*Child* should never have OpenContentStream called!");
  return NS_OK;
}
  




class FTPStartRequestEvent : public ChannelEvent
{
 public:
  FTPStartRequestEvent(FTPChannelChild* aChild, const PRInt32& aContentLength,
                       const nsCString& aContentType, const PRTime& aLastModified,
                       const nsCString& aEntityID, const IPC::URI& aURI)
  : mChild(aChild), mContentLength(aContentLength), mContentType(aContentType),
    mLastModified(aLastModified), mEntityID(aEntityID), mURI(aURI) {}
  void Run() { mChild->DoOnStartRequest(mContentLength, mContentType,
                                       mLastModified, mEntityID, mURI); }
 private:
  FTPChannelChild* mChild;
  PRInt32 mContentLength;
  nsCString mContentType;
  PRTime mLastModified;
  nsCString mEntityID;
  IPC::URI mURI;
};

bool
FTPChannelChild::RecvOnStartRequest(const PRInt32& aContentLength,
                                    const nsCString& aContentType,
                                    const PRTime& aLastModified,
                                    const nsCString& aEntityID,
                                    const IPC::URI& aURI)
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new FTPStartRequestEvent(this, aContentLength, aContentType,
                                             aLastModified, aEntityID, aURI));
  } else {
    DoOnStartRequest(aContentLength, aContentType, aLastModified,
                     aEntityID, aURI);
  }
  return true;
}

void
FTPChannelChild::DoOnStartRequest(const PRInt32& aContentLength,
                                  const nsCString& aContentType,
                                  const PRTime& aLastModified,
                                  const nsCString& aEntityID,
                                  const IPC::URI& aURI)
{
  LOG(("FTPChannelChild::RecvOnStartRequest [this=%x]\n", this));

  SetContentLength(aContentLength);
  SetContentType(aContentType);
  mLastModifiedTime = aLastModified;
  mEntityID = aEntityID;

  nsCString spec;
  nsCOMPtr<nsIURI> uri(aURI);
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
                        const PRUint32& aOffset, const PRUint32& aCount)
  : mChild(aChild), mData(aData), mOffset(aOffset), mCount(aCount) {}
  void Run() { mChild->DoOnDataAvailable(mData, mOffset, mCount); }
 private:
  FTPChannelChild* mChild;
  nsCString mData;
  PRUint32 mOffset, mCount;
};

bool
FTPChannelChild::RecvOnDataAvailable(const nsCString& data,
                                     const PRUint32& offset,
                                     const PRUint32& count)
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new FTPDataAvailableEvent(this, data, offset, count));
  } else {
    DoOnDataAvailable(data, offset, count);
  }
  return true;
}

void
FTPChannelChild::DoOnDataAvailable(const nsCString& data,
                                   const PRUint32& offset,
                                   const PRUint32& count)
{
  LOG(("FTPChannelChild::RecvOnDataAvailable [this=%x]\n", this));

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
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new FTPStopRequestEvent(this, statusCode));
  } else {
    DoOnStopRequest(statusCode);
  }
  return true;
}

void
FTPChannelChild::DoOnStopRequest(const nsresult& statusCode)
{
  LOG(("FTPChannelChild::RecvOnStopRequest [this=%x status=%u]\n",
           this, statusCode));

  if (!mCanceled)
    mStatus = statusCode;

  { 
    
    mIsPending = PR_FALSE;
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);
    (void)mListener->OnStopRequest(this, mListenerContext, statusCode);
    mListener = nsnull;
    mListenerContext = nsnull;

    if (mLoadGroup)
      mLoadGroup->RemoveRequest(this, nsnull, statusCode);
  }

  
  
  Send__delete__(this);
}

class FTPCancelEarlyEvent : public ChannelEvent
{
 public:
  FTPCancelEarlyEvent(FTPChannelChild* aChild, nsresult aStatus)
  : mChild(aChild), mStatus(aStatus) {}
  void Run() { mChild->DoCancelEarly(mStatus); }
 private:
  FTPChannelChild* mChild;
  nsresult mStatus;
};

bool
FTPChannelChild::RecvCancelEarly(const nsresult& statusCode)
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new FTPCancelEarlyEvent(this, statusCode));
  } else {
    DoCancelEarly(statusCode);
  }
  return true;
}

void
FTPChannelChild::DoCancelEarly(const nsresult& statusCode)
{
  if (mCanceled)
    return;

  mCanceled = true;
  mStatus = statusCode;
  mIsPending = PR_FALSE;
  
  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nsnull, statusCode);

  if (mListener) {
    mListener->OnStartRequest(this, mListenerContext);
    mListener->OnStopRequest(this, mListenerContext, statusCode);
  }

  mListener = nsnull;
  mListenerContext = nsnull;

  if (mIPCOpen)
    Send__delete__(this);
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
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new FTPDeleteSelfEvent(this));
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
    mEventQ.Suspend();
  }
  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::Resume()
{
  NS_ENSURE_TRUE(mIPCOpen, NS_ERROR_NOT_AVAILABLE);

  if (!--mSuspendCount) {
    SendResume();
    mEventQ.Resume();    
  }
  return NS_OK;
}





NS_IMETHODIMP
FTPChannelChild::ConnectParent(PRUint32 id)
{
  
  
  AddIPDLReference();

  if (!gNeckoChild->SendPFTPChannelConstructor(this))
    return NS_ERROR_FAILURE;

  if (!SendConnectChannel(id))
    return NS_ERROR_FAILURE;

  return NS_OK;
}

NS_IMETHODIMP
FTPChannelChild::CompleteRedirectSetup(nsIStreamListener *listener,
                                       nsISupports *aContext)
{
  LOG(("FTPChannelChild::CompleteRedirectSetup [this=%x]\n", this));

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

} 
} 

