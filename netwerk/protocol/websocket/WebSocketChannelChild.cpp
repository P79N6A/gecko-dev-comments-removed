






































#include "WebSocketLog.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/net/NeckoChild.h"
#include "WebSocketChannelChild.h"
#include "nsITabChild.h"

namespace mozilla {
namespace net {

NS_IMPL_ADDREF(WebSocketChannelChild)

NS_IMETHODIMP_(nsrefcnt) WebSocketChannelChild::Release()
{
  NS_PRECONDITION(0 != mRefCnt, "dup release");
  NS_ASSERT_OWNINGTHREAD(WebSocketChannelChild);
  --mRefCnt;
  NS_LOG_RELEASE(this, mRefCnt, "WebSocketChannelChild");

  if (mRefCnt == 1 && mIPCOpen) {
    SendDeleteSelf();
    return mRefCnt;
  }

  if (mRefCnt == 0) {
    mRefCnt = 1; 
    delete this;
    return 0;
  }
  return mRefCnt;
}

NS_INTERFACE_MAP_BEGIN(WebSocketChannelChild)
  NS_INTERFACE_MAP_ENTRY(nsIWebSocketProtocol)
  NS_INTERFACE_MAP_ENTRY(nsIProtocolHandler)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebSocketProtocol)
NS_INTERFACE_MAP_END

WebSocketChannelChild::WebSocketChannelChild(bool aSecure)
: mEventQ(static_cast<nsIWebSocketProtocol*>(this))
, mIPCOpen(false)
, mCancelled(false)
{
  LOG(("WebSocketChannelChild::WebSocketChannelChild() %p\n", this));
  BaseWebSocketChannel::mEncrypted = aSecure;
}

WebSocketChannelChild::~WebSocketChannelChild()
{
  LOG(("WebSocketChannelChild::~WebSocketChannelChild() %p\n", this));
}

void
WebSocketChannelChild::AddIPDLReference()
{
  NS_ABORT_IF_FALSE(!mIPCOpen, "Attempt to retain more than one IPDL reference");
  mIPCOpen = true;
  AddRef();
}

void
WebSocketChannelChild::ReleaseIPDLReference()
{
  NS_ABORT_IF_FALSE(mIPCOpen, "Attempt to release nonexistent IPDL reference");
  mIPCOpen = false;
  Release();
}

class StartEvent : public ChannelEvent
{
 public:
  StartEvent(WebSocketChannelChild* aChild,
             const nsCString& aProtocol)
  : mChild(aChild)
  , mProtocol(aProtocol)
  {}

  void Run()
  {
    mChild->OnStart(mProtocol);
  }
 private:
  WebSocketChannelChild* mChild;
  nsCString mProtocol;
};

bool
WebSocketChannelChild::RecvOnStart(const nsCString& aProtocol)
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new StartEvent(this, aProtocol));
  } else {
    OnStart(aProtocol);
  }
  return true;
}

void
WebSocketChannelChild::OnStart(const nsCString& aProtocol)
{
  LOG(("WebSocketChannelChild::RecvOnStart() %p\n", this));
  SetProtocol(aProtocol);
  if (mListener) {
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);;
    mListener->OnStart(mContext);
  }
}

class StopEvent : public ChannelEvent
{
 public:
  StopEvent(WebSocketChannelChild* aChild,
            const nsresult& aStatusCode)
  : mChild(aChild)
  , mStatusCode(aStatusCode)
  {}

  void Run()
  {
    mChild->OnStop(mStatusCode);
  }
 private:
  WebSocketChannelChild* mChild;
  nsresult mStatusCode;
};

bool
WebSocketChannelChild::RecvOnStop(const nsresult& aStatusCode)
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new StopEvent(this, aStatusCode));
  } else {
    OnStop(aStatusCode);
  }
  return true;
}

void
WebSocketChannelChild::OnStop(const nsresult& aStatusCode)
{
  LOG(("WebSocketChannelChild::RecvOnStop() %p\n", this));
  if (mListener) {
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);;
    mListener->OnStop(mContext, aStatusCode);
  }
}

class MessageEvent : public ChannelEvent
{
 public:
  MessageEvent(WebSocketChannelChild* aChild,
               const nsCString& aMessage,
               bool aBinary)
  : mChild(aChild)
  , mMessage(aMessage)
  , mBinary(aBinary)
  {}

  void Run()
  {
    if (!mBinary) {
      mChild->OnMessageAvailable(mMessage);
    } else {
      mChild->OnBinaryMessageAvailable(mMessage);
    }
  }
 private:
  WebSocketChannelChild* mChild;
  nsCString mMessage;
  bool mBinary;
};

bool
WebSocketChannelChild::RecvOnMessageAvailable(const nsCString& aMsg)
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new MessageEvent(this, aMsg, false));
  } else {
    OnMessageAvailable(aMsg);
  }
  return true;
}

void
WebSocketChannelChild::OnMessageAvailable(const nsCString& aMsg)
{
  LOG(("WebSocketChannelChild::RecvOnMessageAvailable() %p\n", this));
  if (mListener) {
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);;
    mListener->OnMessageAvailable(mContext, aMsg);
  }
}

bool
WebSocketChannelChild::RecvOnBinaryMessageAvailable(const nsCString& aMsg)
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new MessageEvent(this, aMsg, true));
  } else {
    OnBinaryMessageAvailable(aMsg);
  }
  return true;
}

void
WebSocketChannelChild::OnBinaryMessageAvailable(const nsCString& aMsg)
{
  LOG(("WebSocketChannelChild::RecvOnBinaryMessageAvailable() %p\n", this));
  if (mListener) {
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);;
    mListener->OnBinaryMessageAvailable(mContext, aMsg);
  }
}

class AcknowledgeEvent : public ChannelEvent
{
 public:
  AcknowledgeEvent(WebSocketChannelChild* aChild,
                   const PRUint32& aSize)
  : mChild(aChild)
  , mSize(aSize)
  {}

  void Run()
  {
    mChild->OnAcknowledge(mSize);
  }
 private:
  WebSocketChannelChild* mChild;
  PRUint32 mSize;
};

bool
WebSocketChannelChild::RecvOnAcknowledge(const PRUint32& aSize)
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new AcknowledgeEvent(this, aSize));
  } else {
    OnAcknowledge(aSize);
  }
  return true;
}

void
WebSocketChannelChild::OnAcknowledge(const PRUint32& aSize)
{
  LOG(("WebSocketChannelChild::RecvOnAcknowledge() %p\n", this));
  if (mListener) {
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);;
    mListener->OnAcknowledge(mContext, aSize);
  }
}

class ServerCloseEvent : public ChannelEvent
{
 public:
  ServerCloseEvent(WebSocketChannelChild* aChild)
  : mChild(aChild)
  {}

  void Run()
  {
    mChild->OnServerClose();
  }
 private:
  WebSocketChannelChild* mChild;
};

bool
WebSocketChannelChild::RecvOnServerClose()
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new ServerCloseEvent(this));
  } else {
    OnServerClose();
  }
  return true;
}

void
WebSocketChannelChild::OnServerClose()
{
  LOG(("WebSocketChannelChild::RecvOnServerClose() %p\n", this));
  if (mListener) {
    AutoEventEnqueuer ensureSerialDispatch(mEventQ);;
    mListener->OnServerClose(mContext);
  }
}

class AsyncOpenFailedEvent : public ChannelEvent
{
 public:
  AsyncOpenFailedEvent(WebSocketChannelChild* aChild)
  : mChild(aChild)
  {}

  void Run()
  {
    mChild->AsyncOpenFailed();
  }
 private:
  WebSocketChannelChild* mChild;
};

bool
WebSocketChannelChild::RecvAsyncOpenFailed()
{
  if (mEventQ.ShouldEnqueue()) {
    mEventQ.Enqueue(new AsyncOpenFailedEvent(this));
  } else {
    AsyncOpenFailed();
  }
  return true;
}

void
WebSocketChannelChild::AsyncOpenFailed()
{
  LOG(("WebSocketChannelChild::RecvAsyncOpenFailed() %p\n", this));
  mCancelled = true;
  if (mIPCOpen)
    SendDeleteSelf();
}

NS_IMETHODIMP
WebSocketChannelChild::AsyncOpen(nsIURI *aURI,
                                 const nsACString &aOrigin,
                                 nsIWebSocketListener *aListener,
                                 nsISupports *aContext)
{
  LOG(("WebSocketChannelChild::AsyncOpen() %p\n", this));

  NS_ABORT_IF_FALSE(aURI && aListener && !mListener, 
                    "Invalid state for WebSocketChannelChild::AsyncOpen");

  mozilla::dom::TabChild* tabChild = nsnull;
  nsCOMPtr<nsITabChild> iTabChild;
  NS_QueryNotificationCallbacks(mCallbacks, mLoadGroup,
                                NS_GET_IID(nsITabChild),
                                getter_AddRefs(iTabChild));
  if (iTabChild) {
    tabChild = static_cast<mozilla::dom::TabChild*>(iTabChild.get());
  }

  
  AddIPDLReference();

  gNeckoChild->SendPWebSocketConstructor(this, tabChild);
  if (!SendAsyncOpen(aURI, nsCString(aOrigin), mProtocol, mEncrypted))
    return NS_ERROR_UNEXPECTED;

  mOriginalURI = aURI;
  mURI = mOriginalURI;
  mListener = aListener;
  mContext = aContext;
  mOrigin = aOrigin;

  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannelChild::Close()
{
  LOG(("WebSocketChannelChild::Close() %p\n", this));

  if (mCancelled)
    return NS_ERROR_UNEXPECTED;

  if (!mIPCOpen || !SendClose())
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannelChild::SendMsg(const nsACString &aMsg)
{
  LOG(("WebSocketChannelChild::SendMsg() %p\n", this));

  if (mCancelled)
    return NS_ERROR_UNEXPECTED;

  if (!mIPCOpen || !SendSendMsg(nsCString(aMsg)))
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannelChild::SendBinaryMsg(const nsACString &aMsg)
{
  LOG(("WebSocketChannelChild::SendBinaryMsg() %p\n", this));

  if (mCancelled)
    return NS_ERROR_UNEXPECTED;

  if (!mIPCOpen || !SendSendBinaryMsg(nsCString(aMsg)))
    return NS_ERROR_UNEXPECTED;
  return NS_OK;
}

NS_IMETHODIMP
WebSocketChannelChild::GetSecurityInfo(nsISupports **aSecurityInfo)
{
  LOG(("WebSocketChannelChild::GetSecurityInfo() %p\n", this));
  return NS_ERROR_NOT_AVAILABLE;
}

} 
} 
